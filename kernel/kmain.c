// kmain.c -- Brad Slayter
// C entry point for kernel. Do all initialization from here

#include "lib/common.h"
#include "lib/timer.h"
#include "lib/bootinfo.h"
#include "lib/stdio.h"
#include "io/monitor.h"

#include "kernel/descriptor_tables.h"
#include "kernel/cmd.h"

#include "mm/physmem.h"
#include "mm/virtmem.h"

#include "drivers/keyboard.h"

typedef struct memory_region_struct {
	u32int	startLo;	//base address
	u32int	startHi;
	u32int	sizeLo;		//length (in bytes)
	u32int	sizeHi;
	u32int	type;
	u32int	acpi_3_0;
} memory_region;

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

	mem_init_region(0x100000+kernelSize*512, 0xF4240000);
	kprintf(K_INFO, "Initialized a shit ton of memory\n");

	asm volatile("sti");
	kprintf(K_INFO, "Interrupts enabled\n");
	
	u32int *arr = mem_alloc_blocks(sizeof(u32int) * 5);
	arr[4] = 32;
	kprintf(K_INFO, "Memory alloc test: %d\n", arr[4]);
	mem_free_blocks(arr, sizeof(u32int) * 5);
	kprintf(K_INFO, "Memory test completed\n");

	kb_install_kb();

	/*char c = 0;
	while (c != '\n') {
		c = getch();
		mon_put(c);
	}*/

	start_cmd_prompt();

	return 0xDEADBEEF;
}