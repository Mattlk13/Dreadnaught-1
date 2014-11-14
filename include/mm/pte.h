// pte.h -- Brad Slayter

#ifndef PTE_H
#define PTE_H

#include "lib/common.h"
#include "mm/physmem.h"

enum PAGE_PTE_FLAGS {
	PTE_PRESENT = 1,
	PTE_WRITABLE = 2,
	PTE_USER = 4,
	PTE_WRITETHROUGH = 8,
	PTE_NOT_CACHEABLE = 0x10,
	PTE_ACCESSED = 0x20,
	PTE_DIRTY = 0x40,
	PTE_PAT = 0x80,
	PTE_CPU_GLOBAL = 0x100,
	PTE_LV4_GLOBAL = 0x200,
	PTE_FRAME = 0x7FFFF000
};

typedef u32int pt_entry;

void pt_entry_add_attrib(pt_entry *e, u32int attrib);
void pt_entry_del_attrib(pt_entry *e, u32int attrib);
void pt_entry_set_frame(pt_entry *e, u32int addr);
u8int pt_entry_is_present(pt_entry e);
u8int pt_entry_is_writable(pt_entry e);
u8int pt_entry_is_user(pt_entry e);
u8int pt_entry_is_accessed(pt_entry e);
u8int pt_entry_is_dirty(pt_entry e);
u32int pt_entry_pfn(pt_entry e);

#endif