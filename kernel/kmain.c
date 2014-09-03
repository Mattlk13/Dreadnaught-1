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

int kmain(multiboot_info_t *bootinfo) {
	mon_clear();
	init_descriptor_tables();
	asm volatile("sti");
	kprintf(K_OK, "System Booted!\n");
	kprintf(K_INFO, "\"I am the one who knocks!\"\n");
	kprintf(K_INFO, "Mem lower is %d\n", bootinfo->mem_lower);
	kprintf(K_INFO, "Mem upper is %d\n", bootinfo->mem_upper);

	mem_init(bootinfo->mem_upper, kernelSize);
	kprintf(K_OK, "Physical Memory initialized\n");
	virt_init();
	kprintf(K_OK, "Virtual Memory initialized\n");

	kb_install_kb();

	char c = 0;
	while (c != '\n') {
		//kprintf(K_INFO, "Looping\n");
		c = getch();
		//kprintf(K_INFO, "GETCH returned");
		mon_put(c);
	}

	//start_cmd_prompt();

	return 0xDEADBEEF;
}