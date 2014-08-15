// pde.h -- Brad Slayter

#ifndef PDE_H
#define PDE_H

#include "lib/common.h"
#include "mm/physmem.h"

enum PAGE_PDE_FLAGS {
	PDE_PRESENT = 1,
	PDE_WRITABLE = 2,
	PDE_USER = 4,
	PDE_PWT = 8,
	PDE_PCD = 0x10,
	PDE_ACCESSED = 0x20,
	PDE_DIRTY = 0x40,
	PDE_4MB = 0x80,
	PDE_CPU_GLOBAL = 0x100,
	PDE_LV4_GLOBAL = 0x200,
	PDE_FRAME = 0x7FFFF000
};

typedef u32int pd_entry;

void pd_entry_add_attrib(pd_entry *e, u32int attrib);
void pd_entry_del_attrib(pd_entry *e, u32int attrib);
void pd_entry_set_frame(pd_entry *e, u32int addr);
u8int pd_entry_is_present(pd_entry e);
u8int pd_entry_is_user(pd_entry e);
u8int pd_entry_is_4mb(pd_entry e);
u8int pd_entry_is_writable(pd_entry e);
u32int pd_entry_pfn(pd_entry e);
void pd_entry_enable_global(pd_entry e);

#endif