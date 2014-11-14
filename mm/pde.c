// pde.c -- Brad Slayter

#include "mm/pde.h"

inline void pd_entry_add_attrib(pd_entry *e, u32int attrib) {
	*e |= attrib;
}

inline void pd_entry_del_attrib(pd_entry *e, u32int attrib) {
	*e &= ~attrib;
}

inline void pd_entry_set_frame(pd_entry *e, u32int addr) {
	*e = (*e & ~PDE_FRAME) | addr;
}

inline u8int pd_entry_is_present(pd_entry e) {
	return e & PDE_PRESENT;
}

inline u8int pd_entry_is_writable(pd_entry e) {
	return e & PDE_WRITABLE;
}

inline u32int pd_entry_pfn(pd_entry e) {
	return e & PDE_FRAME;
}

inline u8int pd_entry_is_user(pd_entry e) {
	return e & PDE_USER;
}

inline u8int pd_entry_is_accessed(pd_entry e) {
	return e & PDE_ACCESSED;
}

inline u8int pd_entry_is_dirty(pd_entry e) {
	return e & PDE_DIRTY;
}

inline u8int pd_entry_is_4mb(pd_entry e) {
	return e & PDE_4MB;
}

inline void pd_entry_enable_global(pd_entry e) {
	
}