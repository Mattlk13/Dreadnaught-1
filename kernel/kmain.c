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
#include "drivers/Floppy.h"

#include "fs/fat12.h"

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

void read_from_floppy() {
	kprintf(K_WARN, "Attempt floppy read\n");
	u8int *sector = 0;
	sector = flpy_read_sector(19);

	kprintf(K_INFO, "Whole sector %s and %d", sector, sector);
	if (sector != 0) {
		int i = 0;
		for (int c = 0; c < 4; c++) {
			for (int j = 0; j < 128; j++)
				kprintf(K_NONE, "%x ", sector[i+j]);
			i += 128;

			kprintf(K_NONE, "\n\n");
			kprintf(K_OK, "Press any key to continue...\n");
			getch();
		}
	} else {
		kprintf(K_ERROR, "Error reading sector from disk");
	}
}

int kmain(multiboot_info_t *bootinfo) {
	mon_clear();
	kprintf(K_OK, "System Booted!\n");
	kprintf(K_INFO, "\"I am the one who knocks!\"\n");

	init_descriptor_tables();
	
	asm volatile("sti");
	kprintf(K_OK, "Interrupts Enabled\n");

	kprintf(K_INFO, "Mem lower is %d\n", bootinfo->mem_lower);
	kprintf(K_INFO, "Mem upper is %d\n", bootinfo->mem_upper);

	mem_init(bootinfo->mem_upper, kernelSize);
	kprintf(K_OK, "Physical Memory initialized\n");
	virt_init();
	kprintf(K_OK, "Virtual Memory initialized\n");

	init_timer(100);

	kb_install_kb();

	// FLOPPY
	flpy_set_working_drive(0);
	flpy_install(38);

	fsys_fat_initialize();
	kprintf(K_OK, "File system mounted\n");
	//read_from_floppy();

	start_cmd_prompt();

	return 0xDEADBEEF;
}