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
pdirectory *kernel_directory = 0;
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
	cur_pdbr = (physical_addr)&dir->m_entries;
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

void virt_map_page(pdirectory *pageDirectory, void *virt, void *phys, u32int flags) {
	//pdirectory *pageDirectory = virt_get_directory();

	pd_entry *e = &pageDirectory->m_entries[PAGE_DIRECTORY_INDEX((u32int)virt)];
	if ((*e & PTE_PRESENT) != PTE_PRESENT) {
		ptable *table = (ptable *)mem_alloc_block();
		if (!table)
			return;

		memset(table, 0, sizeof(ptable));

		pd_entry *entry = &pageDirectory->m_entries[PAGE_DIRECTORY_INDEX((u32int)virt)];

		pd_entry_add_attrib(entry, PDE_PRESENT);
		pd_entry_add_attrib(entry, PDE_WRITABLE);
		pd_entry_add_attrib(entry, PDE_USER);
		pd_entry_set_frame(entry, (physical_addr)table);
	}

	ptable *table = (ptable *)PAGE_GET_PHYSICAL_ADDRESS(e);

	pt_entry *page = &table->m_entries[PAGE_TABLE_INDEX((u32int)virt)];

	pt_entry_set_frame(page, (physical_addr)phys);
	pt_entry_add_attrib(page, PTE_PRESENT);
	pt_entry_add_attrib(page, PTE_WRITABLE);
	pt_entry_add_attrib(page, PTE_USER);

}

void virt_check_address_present(pdirectory *dir, u32int virt, u32int phys) {
	pd_entry *e = &dir->m_entries[PAGE_DIRECTORY_INDEX((u32int)virt)];
	if ((*e & PDE_PRESENT) != PDE_PRESENT) {
		kprintf(K_ERROR, "Table not present.\n");
		return;
	}

	ptable *table = (ptable *)PAGE_GET_PHYSICAL_ADDRESS(e);
	pt_entry page = table->m_entries[PAGE_TABLE_INDEX((u32int)virt)];

	if (!pt_entry_is_present(page))
		kprintf(K_ERROR, "Page not present.\n");
	else
		kprintf(K_OK, "Good to go!\n");
}

int virt_create_page_table(pdirectory *dir, u32int virt, u32int flags) {
	kprintf(K_INFO, "Creating table\n");
	pd_entry *pagedir = dir->m_entries;
	if (pagedir[virt >> 22] == 0) {
		void *block = mem_alloc_block();
		if (!block)
			return 0; // out of memory

		pagedir[virt >> 22] = ((u32int)block) | flags;
		memset((u32int *)pagedir[virt >> 22], 0, 4096); // clearing the table

		virt_map_phys_addr(dir, (u32int)block, (u32int)block, flags);
	}

	return 1; // success
}

void virt_map_phys_addr(pdirectory *dir, u32int virt, u32int phys, u32int flags) {
	pd_entry *pagedir = dir->m_entries;
	if (pagedir[virt >> 22] == 0)
		virt_create_page_table(dir, virt, flags);
	((u32int *)(pagedir[virt >> 22] & ~0xFFF))[virt << 10 >> 10 >> 12] = phys | flags;

	kprintf(K_INFO, "Mapped %x to %x with %x flags. Value reads %x\n", virt, phys, flags, pagedir[virt >> 22]);
}

void virt_unmap_page_table(pdirectory *dir, u32int virt) {
	pd_entry *pagedir = dir->m_entries;
	if (pagedir[virt >> 22] != 0) {
		// get mapped frame
		void *frame = (void *)(pagedir[virt >> 22] & 0x7FFFF000);

		// unmap frame
		mem_free_block(frame);
		pagedir[virt >> 22] = 0;
	}
}

void virt_unmap_phys_addr(pdirectory *dir, u32int virt) {
	pd_entry *pagedir = dir->m_entries;
	if (pagedir[virt >> 22] != 0)
		virt_unmap_page_table(dir, virt);
}

void *virt_get_phys_addr(pdirectory *dir, u32int virt) {
	pd_entry *pagedir = dir->m_entries;
	if (pagedir[virt >> 22] == 0) {
		kprintf(K_INFO, "Returning 0\n");
		return 0;
	}
	return (void *)((u32int *)(pagedir[virt >> 22] & ~0xFFF))[virt << 10 >> 10 >> 12];
}

pdirectory *virt_create_addr_space() {
	pdirectory *dir = 0;

	// allocate dir
	dir = (pdirectory *)mem_alloc_block();
	if (!dir)
		return 0; // :(

	// clear dir (mark all tables as not present)
	memset(dir, 0, sizeof(pdirectory));
	memcpy(dir->m_entries, cur_directory->m_entries, sizeof(u32int)*1024);
	return dir;
}

ptable *virt_clone_table(pd_entry *src) {
	ptable *table = (ptable *)mem_alloc_block();
	ptable *srcTable = (ptable *)PAGE_GET_PHYSICAL_ADDRESS(src);

	memset(table, 0, sizeof(ptable));

	for (int i = 0; i < 1024; i++) {
		if (!srcTable->m_entries[i])
			continue;

		pt_entry page = 0;

		if (pt_entry_is_present(srcTable->m_entries[i])) pt_entry_add_attrib(&page, PTE_PRESENT);
		if (pt_entry_is_writable(srcTable->m_entries[i])) pt_entry_add_attrib(&page, PTE_WRITABLE);
		if (pt_entry_is_user(srcTable->m_entries[i])) pt_entry_add_attrib(&page, PTE_USER);
		if (pt_entry_is_accessed(srcTable->m_entries[i])) pt_entry_add_attrib(&page, PTE_ACCESSED);
		if (pt_entry_is_dirty(srcTable->m_entries[i])) pt_entry_add_attrib(&page, PTE_DIRTY);

		pt_entry_set_frame(&page, pt_entry_pfn(srcTable->m_entries[i]));
	}

	return table;
}

pdirectory *virt_clone_directory(pdirectory *src) {
	pdirectory *dir = (pdirectory *)mem_alloc_block();

	memset(dir, 0, sizeof(pdirectory));

	u32int phys = (physical_addr)&dir->m_entries;

	for (int i = 0; i < 1024; i++) {
		if (!src->m_entries[i])
			continue;

		if (kernel_directory->m_entries[i] == src->m_entries[i]) {
			dir->m_entries[i] = src->m_entries[i];
		} else {
			// copy the table
			ptable *table = virt_clone_table(&src->m_entries[i]);

			pd_entry *entry = (pd_entry *)&dir->m_entries[i];
			pd_entry *srcTable = (pd_entry *)&src->m_entries[i];

			if (pd_entry_is_present(src->m_entries[i]))  pd_entry_add_attrib(srcTable, PDE_PRESENT);
			if (pd_entry_is_writable(src->m_entries[i])) pd_entry_add_attrib(srcTable, PDE_WRITABLE);
			if (pd_entry_is_user(src->m_entries[i])) 	  pd_entry_add_attrib(srcTable, PDE_USER);
			if (pd_entry_is_accessed(src->m_entries[i])) pd_entry_add_attrib(srcTable, PDE_ACCESSED);
			if (pd_entry_is_dirty(src->m_entries[i]))    pd_entry_add_attrib(srcTable, PDE_DIRTY);

			//dir->m_entries[i] = src->m_entries[i];
			pd_entry_set_frame(entry, (physical_addr)table);
			kprintf(K_DEBUG, "Table at %x\n", (physical_addr)entry);
		}
	}

	return dir;
}

void page_fault(registers_t *regs) {
	u32int fault_addr;
	asm("mov %%cr2, %0" : "=r"(fault_addr));

	int present  = !(regs->err_code & 0x1);
	int rw 		 = regs->err_code & 0x2;
	int us 		 = regs->err_code & 0x4;
	int reserved = regs->err_code & 0x8;
	int id 		 = regs->err_code & 0x10;

	kprintf(K_ERROR, "Page Fault! ( ");
	if (present) {mon_write("present ");}
	if (rw) {mon_write("read-only ");}
	if (us) {mon_write("user-mode ");}
	if (reserved) {mon_write("reserved ");}
	mon_write(") at ");
	mon_write_hex(fault_addr);
	mon_write("\n");
	kprintf(K_NONE, "\t  eip: %x cs: %x esp: %x ss: %x\n", regs->eip, regs->cs, regs->esp, regs->ss);
	PANIC("Page Fault");
}

void virt_init() {
	ptable *table = (ptable *)mem_alloc_block();
	if (!table)
		return;

	ptable *table2 = (ptable *)mem_alloc_block();
	if (!table2)
		return;

	memset(table, 0, sizeof(ptable));

	for (int i = 0, frame = 0x0, virt = 0x00000000; i < 1024; i++, frame += 4096, virt += 4096) {
		pt_entry page = 0;
		pt_entry_add_attrib(&page, PTE_PRESENT);
		pt_entry_set_frame(&page, frame);

		table2->m_entries[PAGE_TABLE_INDEX(virt)] = page;
	}

	for (int i = 0, frame = 0x100000, virt = 0xC0000000; i < 1024; i++, frame += 4096, virt += 4096) {
		pt_entry page = 0;
		pt_entry_add_attrib(&page, PTE_PRESENT);
		pt_entry_set_frame(&page, frame);

		table->m_entries[PAGE_TABLE_INDEX(virt)] = page;
	}

	pdirectory *dir = (pdirectory *)mem_alloc_blocks(3);
	if (!dir)
		return;

	memset(dir, 0, sizeof(pdirectory));

	pd_entry *entry = &dir->m_entries[PAGE_DIRECTORY_INDEX(0xC0000000)];
	pd_entry_add_attrib(entry, PDE_PRESENT);
	pd_entry_add_attrib(entry, PDE_WRITABLE);
	pd_entry_set_frame(entry, (physical_addr)table);

	pd_entry *entry2 = &dir->m_entries[PAGE_DIRECTORY_INDEX(0x00000000)];
	pd_entry_add_attrib(entry2, PDE_PRESENT);
	pd_entry_add_attrib(entry2, PDE_WRITABLE);
	pd_entry_set_frame(entry2, (physical_addr)table2);

	cur_pdbr = (physical_addr)&dir->m_entries;

	// register page fault handler
	register_interrupt_handler(14, page_fault);

	kernel_directory = dir;
	dir = virt_clone_directory(kernel_directory);
	virt_switch_pdirectory(dir);

	kprintf(K_INFO, "Enable paging\n");
	mem_enable_paging(1);
}