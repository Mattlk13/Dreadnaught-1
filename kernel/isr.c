// isr.c -- Brad Slayter

#include "kernel/isr.h"

#include "lib/common.h"
#include "lib/stdio.h"

isr_t interrupt_handlers[256];

void register_interrupt_handler(u8int n, isr_t handler) {
	kprintf(K_OK, "Interrupt %d registered\n", n);
	interrupt_handlers[n] = handler;
}

void isr_handler(registers_t regs) {
	u8int int_no = regs.int_no & 0xFF;
	if (interrupt_handlers[int_no] != 0) {
		isr_t handler = interrupt_handlers[regs.int_no];
		handler(regs);
	} else {
		kprintf(K_ERROR, "Unhandled ISR: %d\n", regs.int_no);
	}
}

void irq_handler(registers_t regs) {
	//kprintf(K_INFO, "IRQ %d reporting in\n", regs.int_no);
	if (regs.int_no >= 40) {
		outb(0xA0, 0x20);
	}

	// send reset signal to PICs
	outb(0x20, 0x20);

	if (interrupt_handlers[regs.int_no] != 0) {
		isr_t handler = interrupt_handlers[regs.int_no];
		handler(regs);
	}
}