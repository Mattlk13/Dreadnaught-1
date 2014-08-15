// kheap.c -- Brad Slayter

#include "mm/kheap.h"

#include "io/monitor.h"

extern u32int end;
u32int placement_address = (u32int)&end;

u32int kmalloc_int(size_t sz, int align, u32int *phys) {
	if (align && (placement_address & 0xFFFFF000)) {// if not already aligned
		placement_address &= 0xFFFFF000;
		placement_address += 0x1000;
	}

	if (phys) {
		*phys = placement_address;
	}

	u32int tmp = placement_address;
	placement_address += sz;

	return tmp;
}

u32int kmalloc_ap(size_t sz, u32int *phys) {
	return kmalloc_int(sz, 1, phys);
}

u32int kmalloc_p(size_t sz, u32int *phys) {
	return kmalloc_int(sz, 0, phys);
}

u32int kmalloc_a(size_t sz) {
	return kmalloc_int(sz, 1, 0);
}

u32int kmalloc(size_t sz) {
	return kmalloc_int(sz, 0, 0);
}