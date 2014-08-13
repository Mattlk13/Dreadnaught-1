// kmain.c -- Brad Slayter
// C entry point for kernel. Do all initialization from here

#include "lib/common.h"
#include "io/monitor.h"
#include "kernel/descriptor_tables.h"

int kmain(struct multiboot *mbootPtr) {
	mon_clear();

	init_descriptor_tables();

	mon_write("\"I am the one who knocks!\"\n");

	asm volatile("int $0x3");
	asm volatile("int $0x4");

	return 0xDEADBEEF;
}