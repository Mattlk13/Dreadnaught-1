// virtmem.h -- Brad Slayter

#ifndef VIRTMEM_H
#define VIRTMEM_H

#include "lib/common.h"
#include "mm/pte.h"
#include "mm/pde.h"

typedef u32int virtual_addr;

#define PAGES_PER_TABLE 1024
#define PAGES_PER_DIR	1024

#define PAGE_DIRECTORY_INDEX(x) (((x) >> 22) & 0x3FF)
#define PAGE_TABLE_INDEX(x) (((x) >> 12) & 0x3FF)
#define PAGE_GET_PHYSICAL_ADDRESS(x) (*x & ~0xFFF)

typedef struct ptable_struct {
	pt_entry m_entries[PAGES_PER_TABLE];
} ptable;

typedef struct pdirectory_struct {
	pd_entry m_entries[PAGES_PER_DIR];
} pdirectory;

void MmMapPage(void *phys, void *virt);
void virt_init();

void virt_map_page(pdirectory *pageDirectory, void *virt, void *phys, u32int flags);
void virt_check_address_present(pdirectory *dir, u32int virt, u32int phys);

u8int virt_alloc_page(pt_entry *p);
void virt_free_page(pt_entry *p);

u8int virt_switch_pdirectory(pdirectory *pd);
pdirectory *virt_get_directory();

void virt_flush_tlb_entry(virtual_addr addr);
void virt_ptable_clear(ptable *p);

u32int virt_ptable_virt_to_index(virtual_addr addr);
pt_entry *virt_ptable_lookup_entry(ptable *p, virtual_addr addr);

u32int virt_pdirectory_virt_to_index(virtual_addr addr);
void virt_pdirectory_clear(pdirectory *dir);
pd_entry *virt_pdirectory_lookup_entry(pdirectory *p, virtual_addr addr); 

// Additions for loader
int virt_create_page_table(pdirectory *dir, u32int virt, u32int flags);
void virt_map_phys_addr(pdirectory *dir, u32int virt, u32int phys, u32int flags);
void virt_unmap_page_table(pdirectory *dir, u32int virt);
void virt_unmap_phys_addr(pdirectory *dir, u32int virt);
pdirectory *virt_create_addr_space();
void *virt_get_phys_addr(pdirectory *dir, u32int virt);

#endif