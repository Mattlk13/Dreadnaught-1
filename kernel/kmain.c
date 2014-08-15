// kmain.c -- Brad Slayter
// C entry point for kernel. Do all initialization from here

#include "lib/common.h"
#include "lib/timer.h"
#include "lib/bootinfo.h"

#include "io/monitor.h"
#include "lib/stdio.h"

#include "kernel/descriptor_tables.h"

//#include "mm/paging.h"
#include "mm/physmem.h"
#include "mm/virtmem.h"

typedef struct memory_region_struct {

	u32int	startLo;	//base address
	u32int	startHi;
	u32int	sizeLo;		//length (in bytes)
	u32int	sizeHi;
	u32int	type;
	u32int	acpi_3_0;
} memory_region;

//! ...so we can print the different types of memory regions..
/*char* strMemoryTypes[] = {

	{"Available"},				//type 1
	{"Reserved"},				//type 2
	{"ACPI Reclaim"},	//type 3
	{"ACPI NVS Memory"}			//type 4
};*/

extern u32int end;
u32int kernelSize = (u32int)&end;

int kmain(struct multiboot *mbootPtr) {
	
	init_descriptor_tables();
	mon_clear();
	kprintf(K_INFO, "System Booted!\n");
	kprintf(K_INFO, "\"I am the one who knocks!\"\n");

	mem_init(0xF4240000, 0x100000 + kernelSize);

	memory_region*	region = (memory_region*)0x1000;

	for (int i=0; i<10; ++i) {

		if (region[i].type>4)
			break;

		if (i>0 && region[i].startLo==0)
			break;

		mem_init_region(region[i].startLo, region[i].sizeLo);
	}
	mem_deinit_region (0x100000, kernelSize*512);
	kprintf(K_INFO, "Physical memory initialized\n");

	virt_init();
	kprintf(K_INFO, "Virtual memory initialized\n");

	mem_init_region(0x100000+kernelSize*512, 0xF0000000);
	kprintf(K_INFO, "Initialized a shit ton of memory\n");

	asm volatile("sti");
	kprintf(K_INFO, "Interrupts enabled\n");
	//mem_init_region(kernelSize, 0xF0000000);
	u32int *arr = mem_alloc_blocks(sizeof(u32int) * 5);
	arr[4] = 32;
	kprintf(K_INFO, "Memory alloc test: %d\n", arr[4]);
	mem_free_blocks(arr, sizeof(u32int) * 5);
	kprintf(K_INFO, "Memory test completed\n");

	return 0xDEADBEEF;
}