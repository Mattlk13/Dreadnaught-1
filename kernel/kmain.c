// kmain.c -- Brad Slayter
// C entry point for kernel. Do all initialization from here

#include "lib/common.h"
#include "lib/timer.h"
#include "lib/bootinfo.h"
#include "lib/stdio.h"
#include "lib/syscall.h"
#include "io/monitor.h"

#include "kernel/descriptor_tables.h"
#include "kernel/cmd.h"
#include "kernel/Exception.h"

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
	sector = flpy_read_sector(0);

	if (flpy_write_sector(1, sector))
		kprintf(K_OK, "Wrote to floppy\n");
}

u8int detect_floppy_drive() {
	u8int c;
	outb(0x70, 0x10);
	c = inb(0x71);

	c >>= 4;
	return c;
}

int kmain(multiboot_info_t *bootinfo) {
	mon_clear();
	kprintf(K_OK, "System Booted!\n");

	init_descriptor_tables();
	register_exceptions();
	
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
	if (detect_floppy_drive()) {
		kprintf(K_INFO, "Floppy drive detected. Installing FDC.\n");
		flpy_set_working_drive(0);
		flpy_install(38);
		fsys_fat_initialize();
		kprintf(K_OK, "File system mounted\n");
	}
	
	//read_from_floppy();

	initialize_syscalls();
	switch_to_user_mode();
	asm volatile("int $0x80");
	//syscall_mon_write("Hello, user land?");
	//syscall_kprintf(K_OK, "Welcome to User Land!\n");
	for (;;);
	//start_cmd_prompt();

	return 0xDEADBEEF;
}