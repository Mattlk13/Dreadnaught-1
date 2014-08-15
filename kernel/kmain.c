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
	//init_paging();
	mon_write("\"I am the one who knocks!\"\n");

	mon_write("Mem size: ");
	mon_write_hex(mbootPtr->m_memorySize);
	mon_write("\n");

	mem_init(0xF4240000, 0x100000 + kernelSize);

	memory_region*	region = (memory_region*)0x1000;

	for (int i=0; i<10; ++i) {

		if (region[i].type>4)
			break;

		if (i>0 && region[i].startLo==0)
			break;

		/*DebugPrintf ("region %i: start: 0x%x%x length (bytes): 0x%x%x type: %i (%s)\n", i, 
			region[i].startHi, region[i].startLo,
			region[i].sizeHi,region[i].sizeLo,
			region[i].type, strMemoryTypes[region[i].type-1]);*/

		mem_init_region(region[i].startLo, region[i].sizeLo);
	}
	mem_deinit_region (0x100000, kernelSize*512);
	//DebugPrintf ("pmm regions initialized: %i allocation blocks; block size: %i bytes",
	//	pmmngr_get_block_count (), pmmngr_get_block_size () );
	u32int line = 73;
	kprintf(K_INFO, "About to crash at line %d\n", line);

	char str[] = "I am the ding dang diddly danger!";
	kprintf(K_ERROR, "Heisenberg says: %s\n", str);
	kprintf(K_WARN, "Here is that quote again:\n\t%s\nAnd the lucky number is %d", str, line);

	virt_init();

	mem_init_region(0x100000+kernelSize*512, 0xF0000000);

	mon_write("PAGING YEAHHHHHH\n");

	asm volatile("sti");
	//mem_init_region(kernelSize, 0xF0000000);
	u32int *arr = mem_alloc_blocks(sizeof(u32int) * 5);
	arr[4] = 32;
	mon_write_dec(arr[4]);
	mon_write("\n");

	return 0xDEADBEEF;
}