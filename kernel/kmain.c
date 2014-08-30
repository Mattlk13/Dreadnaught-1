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
	kprintf(K_OK, "System Booted!\n");
	kprintf(K_INFO, "\"I am the one who knocks!\"\n");

	kb_install_kb();

	char c = 0;
	while (c != '\n') {
		c = getch();
		mon_put(c);
	}

	//start_cmd_prompt();

	return 0xDEADBEEF;
}