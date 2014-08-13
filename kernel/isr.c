// isr.c -- Brad Slayter

#include "kernel/isr.h"
#include "lib/common.h"
#include "io/monitor.h"

void isr_handler(registers_t regs) {
	mon_write("Recieved interrupt: ");
	mon_write_dec(regs.int_no);
	mon_put('\n');
}