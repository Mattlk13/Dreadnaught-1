// physmem.h -- Brad Slayter

#ifndef PHYSMEM_H
#define PHYSMEM_H

#include "lib/common.h"

typedef u32int physical_addr;

void mem_init(size_t memSize, u32int bitmap);
void mem_init_region(u32int base, size_t sz);
void mem_deinit_region(u32int base, size_t sz);

void *mem_alloc_block();
void mem_free_block(void *p);
void *mem_alloc_blocks(size_t sz);
void mem_free_blocks(void *p, size_t sz);
void *kmalloc(size_t sz);

size_t mem_get_memory_size();
u32int mem_get_block_count();
u32int mem_get_used_block_count();
u32int mem_get_free_block_count();
u32int mem_get_block_size();

void mem_enable_paging(u8int b);
u8int mem_is_paging();

void mem_load_PDBR(u32int addr);
u32int mem_get_PDBR();

#endif