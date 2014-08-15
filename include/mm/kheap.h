// kheap.h -- Brad Slayter

#ifndef KHEAP_H
#define KHEAP_H

#include "lib/common.h"

u32int kmalloc_a(size_t sz); 			   // page aligned
u32int kmalloc_p(size_t sz, u32int *phys);  // returns phys address
u32int kmalloc_ap(size_t sz, u32int *phys); // page aligned + phys addr
u32int kmalloc(size_t sz);				   // vanilla malloc

#endif