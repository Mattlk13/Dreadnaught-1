// paging.c -- Brad Slayter

#include "mm/paging.h"
#include "mm/kheap.h"

#include "io/monitor.h"

page_directory_t *kernel_directory = 0;
page_directory_t *current_directory = 0;

u8int firstSwitch = 0;

u32int *frames;
u32int nframes;

extern u32int placement_address;

#define INDEX_FROM_BIT(a) (a/(8*4))
#define OFFSET_FROM_BIT(a) (a%(8*4))

static void set_frame(u32int frame_addr) {
	u32int frame = frame_addr/0x1000;
	u32int idx = INDEX_FROM_BIT(frame);
	u32int off = OFFSET_FROM_BIT(frame);
	frames[idx] |= (0x1 << off);
}

static void clear_frame(u32int frame_addr) {
	u32int frame = frame_addr/0x1000;
	u32int idx = INDEX_FROM_BIT(frame);
	u32int off = OFFSET_FROM_BIT(frame);
	frames[idx] &= ~(0x01 << off);
}

static u32int test_frame(u32int frame_addr) {
	u32int frame = frame_addr/0x1000;
	u32int idx = INDEX_FROM_BIT(frame);
	u32int off = OFFSET_FROM_BIT(frame);
	return (frames[idx] & (0x1 << off));
}

static u32int first_frame() {
	u32int i, j;
	for (i = 0; i < INDEX_FROM_BIT(nframes); i++) {
		if (frames[i] != 0xFFFFFFFF) {// nothing free
			for (j = 0; j < 32; j++) {
				u32int toTest = 0x1 << j;
				if (!(frames[i]&toTest))
					return i*4*8+j;
			}
		}
	}
}

void alloc_frame(page_t *page, int is_kernel, int is_writeable) {
	if (page->frame != 0) {
		return; // page was already allocated
	} else {
		u32int idx = first_frame();
		if (idx == (u32int)-1) {
			PANIC("No Free Frames!");
		}

		set_frame(idx&0x1000);
		page->present = 1;
		page->rw = (is_writeable)?1:0;
		page->user = (is_kernel)?0:1;
		page->frame = idx;
	}
}

void free_frame(page_t *page) {
	u32int frame;
	if (!(frame=page->frame)) {
		return; // page was never allocated to begin with
	} else {
		clear_frame(frame); // frame is a free elf
		page->frame = 0x0;  // no frame for page
	}
}

void init_paging() {
	mon_write("Init paging\n");

	u32int mem_end_page = 0x10000000; // max of memory
									  // currently 16MB

	nframes = mem_end_page / 0x1000;
	frames = (u32int *)kmalloc(INDEX_FROM_BIT(nframes));
	memset(frames, 0, INDEX_FROM_BIT(nframes));

	// make page dir
	kernel_directory = (page_directory_t *)kmalloc_a(sizeof(page_directory_t));
	memset(kernel_directory, 0, sizeof(page_directory_t));
	current_directory = kernel_directory;

	// identity map from 0x0 to end memory
	int i = 0;
	while (i < placement_address) {
		alloc_frame(get_page(i, 1, kernel_directory), 0, 0);
		i += 0x1000;
	}

	// register fault handler
	register_interrupt_handler(14, page_fault);

	//firstSwitch = 1; // first time switching directory
	switch_page_directory(kernel_directory);
}

void switch_page_directory(page_directory_t *dir) {
	current_directory = dir;
    asm volatile("mov %0, %%cr3":: "r"(&dir->physAddr));

    mon_write("dir->physAddr: ");
    mon_write_hex(dir->physAddr);
    mon_write("\n");

    u32int cr0;
    asm volatile("mov %%cr0, %0": "=r"(cr0));
    cr0 |= 0x80000000; // Enable paging!
    asm volatile("mov %0, %%cr0":: "r"(cr0));
}

page_t *get_page(u32int address, int make, page_directory_t *dir) {
	// get index from address
	address /= 0x1000;
	// Find table containing address
	u32int table_idx = address / 1024;
	if (dir->tables[table_idx]) { // if table is already assigne
		return &dir->tables[table_idx]->pages[address%1024];
	} else if (make) {
		u32int tmp;
		dir->tables[table_idx] = (page_table_t *)kmalloc_ap(sizeof(page_table_t), &tmp);
		memset(dir->tables[table_idx], 0, 0x1000);
		dir->tablesPhysical[table_idx] = tmp | 0x7; // PRES, R/W, U/S
		return &dir->tables[table_idx]->pages[address%1024];
	} else {
		return 0;
	}
}

void page_fault(registers_t regs) {
	u32int fault_addr;
	asm("mov %%cr2, %0" : "=r" (fault_addr));

	int present  = !(regs.err_code & 0x1);
	int rw 		 = regs.err_code & 0x2;
	int us 		 = regs.err_code & 0x4;
	int reserved = regs.err_code & 0x8;
	int id 		 = regs.err_code & 0x10;

	mon_write("Page Fault! ( ");
	if (present) {mon_write("present ");}
	if (rw) {mon_write("read-only ");}
	if (us) {mon_write("user-mode ");}
	if (reserved) {mon_write("reserved ");}
	mon_write(") at ");
	mon_write_hex(fault_addr);
	mon_write("\n");
	PANIC("Page Fault");
}