// physmem.c -- Brad Slayter

#include "mm/physmem.h"
#include "io/monitor.h"
#include "lib/stdio.h"

#define BLOCKS_PER_BYTE 8 		// 8 blocks per byte
#define BLOCK_SIZE 4096			// blocks are 4k each
#define BLOCK_ALIGN BLOCK_SIZE	// blocks are 4k aligned

static u32int memory_size = 0;  // size of phys mem
static u32int used_blocks = 0;  // num blocks in use
static u32int max_blocks  = 0;  // max num of availible blocks
static u32int *memory_map = 0;  // mem map bit array. each bit represents a block

// MEMORY MAP INTERFACES

void mmap_set(int bit);
void mmap_unset(int bit);
u8int mmap_test(int bit);

int mmap_first_free();
int mmap_first_free_s(size_t size);

// MEMORY MAP IMPLEMENTATION

void mmap_set(int bit) {
	memory_map[bit / 32] |= (1 << (bit % 32));
}

void mmap_unset(int bit) {
	memory_map[bit / 32] &= ~(1 << (bit % 32));
}

u8int mmap_test(int bit) {
	return memory_map[bit / 32] & (1 << (bit % 32));
}

int mmap_first_free() {
	for (u32int i = 0; i < mem_get_block_count(); i++) {
		if (memory_map[i] != 0xFFFFFFFF) {
			for (int j = 0; j < 32; j++) {
				int bit = 1 << j;
				if (!(memory_map[i] & bit))
					return i*4*8+j;
			}
		}
	}

	return -1;
}

int mmap_first_free_s(size_t size) {
	if (size == 0)
		return -1;

	if (size == 1)
		return mmap_first_free();

	for (u32int i = 0; i < mem_get_block_count(); i++) {
		if (memory_map[i] != 0xFFFFFFFF) {
			for (int j = 0; j < 32; j++) {
				int bit = 1 << j;
				if (!(memory_map[i] & bit)) {
					int starting_bit = i * 32;
					starting_bit += bit;

					u32int free = 0;
					for (u32int count = 0; count <= size; count++) {
						if (!mmap_test(starting_bit + count))
							free++;

						if (free == size)
							return i*4*8+j;
					}
				}
			}
		}
	}

	return -1;
}

// ACTUAL MEM FUNCTIONS

void mem_init(size_t memSize, u32int bitmap) {
	memory_size = memSize;
	memory_map = (u32int *)bitmap;
	max_blocks = (mem_get_memory_size() * 1024) / BLOCK_SIZE;
	used_blocks = 0;

	// By default all memory is free
	memset(memory_map, 0x0, mem_get_block_count() / BLOCKS_PER_BYTE);

	// reserve necessarry areas of memory
	mem_deinit_region(0x0, 0x200000);
	kprintf(K_OK, "Memory initialized with %d blocks free\n", mem_get_free_block_count());
}

void mem_init_region(u32int base, size_t size) {
	int align = base / BLOCK_SIZE;
	int blocks = size / BLOCK_SIZE;

	for (; blocks > 0; blocks--) {
		mmap_unset(align++);
		used_blocks--;
	}

	mmap_set(0); // first block is always set. This insures allocs can't be 0
}

void mem_deinit_region(u32int base, size_t size) {
	int align = base / BLOCK_SIZE;
	int blocks = size / BLOCK_SIZE;

	for (; blocks > 0; blocks--) {
		mmap_set(align++);
		used_blocks++;
	}

	mmap_set(0);
}

void *mem_alloc_block() {
	if (mem_get_free_block_count() <= 0) {
		return 0; // out of memory
	}

	int frame = mmap_first_free();

	if (frame == -1) {
		return 0; // out of memory
	}

	mmap_set(frame);

	u32int addr = frame * BLOCK_SIZE;
	used_blocks++;

	return (void *)addr;
}

void mem_free_block(void *p) {
	u32int addr = (u32int)p;
	int frame = addr / BLOCK_SIZE;

	mmap_unset(frame);

	used_blocks--;
}

void *mem_alloc_blocks(size_t sz) {
	if (mem_get_free_block_count() <= sz)
		return 0; // not enough space

	int frame = mmap_first_free_s(sz);

	if (frame == -1)
		return 0; // not enough space

	for (u32int i = 0; i < sz; i++) {
		mmap_set(frame + i);
	}

	u32int addr = frame * BLOCK_SIZE;
	used_blocks += sz;

	return (void *)addr;
}

void mem_free_blocks(void *p, size_t sz) {
	u32int addr = (u32int)p;
	int frame = addr / BLOCK_SIZE;

	for (u32int i = 0; i < sz; i++) {
		mmap_unset(frame + i);
	}

	used_blocks -= sz;
}

void *kmalloc(size_t sz) {
	if (sz <= BLOCK_SIZE)
		return mem_alloc_block();
	else
		return mem_alloc_blocks(sz / BLOCK_SIZE);

}

size_t mem_get_memory_size() {
	return memory_size;
}

u32int mem_get_block_count() {
	return max_blocks;
}

u32int mem_get_used_block_count() {
	return used_blocks;
}

u32int mem_get_free_block_count() {
	return max_blocks - used_blocks;
}

u32int mem_get_block_size() {
	return BLOCK_SIZE;
}

void mem_enable_paging(u8int b) {
	u32int cr0;
	asm volatile("mov %%cr0, %0": "=r"(cr0));
	if (b == 1) {
    	cr0 |= 0x80000000; // Enable paging!
    	asm volatile("mov %0, %%cr0":: "r"(cr0));
    	kprintf(K_INFO, "Paging actually enabled!\n");
	} else {
		cr0 &= 0x7FFFFFFF;
		asm volatile("mov %0, %%cr0":: "r"(cr0));
	}
}

u8int mem_is_paging() {
	u32int cr0;
	asm volatile("mov %%cr0, %0": "=r"(cr0));

	return (cr0 & 0x80000000) ? 0 : 1;
}

void mem_load_PDBR(u32int addr) {
	asm volatile("mov %0, %%cr3":: "r"(addr));
}

u32int mem_get_PDBR() {
	u32int addr;
	asm volatile("mov %%cr3, %0": "=r"(addr));

	return addr;
}