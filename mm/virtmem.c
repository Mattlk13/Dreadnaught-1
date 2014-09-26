// virtmem.c -- Brad Slayter

#include "mm/virtmem.h"
#include "mm/physmem.h"

#include "kernel/isr.h"

#include "io/monitor.h"

#include "lib/stdio.h"

#define PTABLE_ADDR_SPACE_SIZE 0x400000    // page table is 4mb
#define DTABLE_ADDR_SPACE_SIZE 0x100000000 // page dir is 4gb
#define PAGE_SIZE 4096 // pages are 4k

pdirectory *cur_directory = 0;
physical_addr cur_pdbr = 0; // page dir base register

inline pt_entry *virt_ptable_lookup_entry(ptable *p, virtual_addr addr) {
	if (p)
		return &p->m_entries[PAGE_TABLE_INDEX(addr)];
	return 0;
}

inline pd_entry *virt_pdirectory_lookup_entry(pdirectory *p, virtual_addr addr) {
	if (p)
		return &p->m_entries[PAGE_DIRECTORY_INDEX(addr)];
	return 0;
}

inline void virt_ptable_clear(ptable *p) {
	if (p)
		memset(p, 0, sizeof(ptable));
}

inline void virt_pdirectory_clear(pdirectory *dir) {
	if (dir)
		memset(dir, 0, sizeof(pdirectory));
}

inline u32int virt_pdirectory_virt_to_index(virtual_addr addr) {
	return (addr >= DTABLE_ADDR_SPACE_SIZE) ? 0 : addr/PAGE_SIZE;
}

inline u32int virt_ptable_virt_to_index(virtual_addr addr) {
	return (addr >= PTABLE_ADDR_SPACE_SIZE) ? 0 : addr/PAGE_SIZE;
}

inline u8int virt_switch_pdirectory(pdirectory *dir) {
	if (!dir)
		return 0;

	cur_directory = dir;
	mem_load_PDBR(cur_pdbr);
	return 1;
}

void virt_flush_tlb_entry(virtual_addr addr) {
	asm volatile("cli");
	asm volatile("invlpg (%0)":: "r"((void *)addr));
	asm volatile("sti");
}

pdirectory *virt_get_directory() {
	return cur_directory;
}

u8int virt_alloc_page(pt_entry *e) {
	void *p = mem_alloc_block();
	if (!p)
		return 0;

	// map to page
	pt_entry_set_frame(e, (physical_addr)p);
	pt_entry_add_attrib(e, PTE_PRESENT);

	return 1;
}

void virt_free_page(pt_entry *e) {
	void *p = (void *)pt_entry_pfn(*e);
	if (p)
		mem_free_block(p);

	pt_entry_del_attrib(e, PTE_PRESENT);
}

void virt_map_page(void *phys, void *virt) {
	pdirectory *pageDirectory = virt_get_directory();

	pd_entry *e = &pageDirectory->m_entries[PAGE_DIRECTORY_INDEX((u32int)virt)];
	if ((*e & PTE_PRESENT) != PTE_PRESENT) {
		ptable *table = (ptable *)mem_alloc_block();
		if (!table)
			return;

		memset(table, 0, sizeof(ptable));

		pd_entry *entry = &pageDirectory->m_entries[PAGE_DIRECTORY_INDEX((u32int)virt)];

		pd_entry_add_attrib(entry, PDE_PRESENT);
		pd_entry_add_attrib(entry, PDE_WRITABLE);
		pd_entry_set_frame(entry, (physical_addr)table);
	}

	ptable *table = (ptable *)PAGE_GET_PHYSICAL_ADDRESS(e);

	pt_entry *page = &table->m_entries[PAGE_TABLE_INDEX((u32int)virt)];

	pt_entry_set_frame(page, (physical_addr)phys);
	pt_entry_add_attrib(page, PTE_PRESENT);

}

void page_fault(registers_t regs) {
	u32int fault_addr;
	asm("mov %%cr2, %0" : "=r"(fault_addr));

	int present  = !(regs.err_code & 0x1);
	int rw 		 = regs.err_code & 0x2;
	int us 		 = regs.err_code & 0x4;
	int reserved = regs.err_code & 0x8;
	int id 		 = regs.err_code & 0x10;

	kprintf(K_ERROR, "Page Fault! ( ");
	if (present) {mon_write("present ");}
	if (rw) {mon_write("read-only ");}
	if (us) {mon_write("user-mode ");}
	if (reserved) {mon_write("reserved ");}
	mon_write(") at ");
	mon_write_hex(fault_addr);
	mon_write("\n");
	PANIC("Page Fault");
}

void virt_init() {
	ptable *table = (ptable *)mem_alloc_block();
	if (!table) {
		kprintf(K_ERROR, "Could not allocate page table\n");
		return;
	}

	virt_ptable_clear(table);

	for (int i = 0, frame = 0; i < 1024; i++, frame += 4096) {
		pt_entry page = 0;
		pt_entry_add_attrib(&page, PTE_PRESENT);
		pt_entry_add_attrib(&page, PTE_USER);
		pt_entry_set_frame(&page, frame);

		table->m_entries[virt_ptable_virt_to_index(frame)] = page;
	}

	pdirectory *dir = (pdirectory *)mem_alloc_blocks(3);
	if (!dir) {
		kprintf(K_ERROR, "Could not allocate page directory\n");
		return;
	}

	virt_pdirectory_clear(dir);

	pd_entry *entry = virt_pdirectory_lookup_entry(dir, 0);
	pd_entry_add_attrib(entry, PDE_PRESENT);
	pd_entry_add_attrib(entry, PDE_WRITABLE);
	pd_entry_add_attrib(entry, PDE_USER);
	pd_entry_set_frame(entry, (physical_addr)table);

	cur_pdbr = (physical_addr)&dir->m_entries;

	// register page fault handler
	register_interrupt_handler(14, page_fault);

	virt_switch_pdirectory(dir);

	kprintf(K_INFO, "Enable paging\n");
	mem_enable_paging(1);
}