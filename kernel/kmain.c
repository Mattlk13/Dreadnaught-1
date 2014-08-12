// kmain.c -- Brad Slayter
// C entry point for kernel. Do all initialization from here

#include "lib/common.h"
#include "io/monitor.h"

int kmain(struct multiboot *mbootPtr) {
	mon_clear();
	mon_write("\"I am the one who knocks!\"\n");

	return 0xDEADBEEF;
}