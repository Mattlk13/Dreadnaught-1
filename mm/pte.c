// pte.c -- Brad Slayter

#include "mm/pte.h"

inline void pt_entry_add_attrib(pt_entry *e, u32int attrib) {
	*e |= attrib;
}

inline void pt_entry_del_attrib(pt_entry *e, u32int attrib) {
	*e &= ~attrib;
}

inline void pt_entry_set_frame(pt_entry *e, u32int addr) {
	*e = (*e & ~PTE_FRAME) | addr;
}

inline u8int pt_entry_is_present(pt_entry e) {
	return e & PTE_PRESENT;
}

inline u8int pt_entry_is_writable(pt_entry e) {
	return e & PTE_WRITABLE;
}

inline u8int pt_entry_is_user(pt_entry e) {
	return e & PTE_USER;
}

inline u8int pt_entry_is_accessed(pt_entry e) {
	return e & PTE_ACCESSED;
}

inline u8int pt_entry_is_dirty(pt_entry e) {
	return e & PTE_DIRTY;
}

inline u32int pt_entry_pfn(pt_entry e) {
	return e & PTE_FRAME;
}