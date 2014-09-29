// Exception.c -- Brad Slayter

#include "kernel/Exception.h"
#include "kernel/isr.h"

#include "lib/stdio.h"

void div_by_zero(registers_t *regs) {
	PANIC("Divide By Zero");
}

void single_step_trap(registers_t *regs) {
	PANIC("Single Step Trap");
}

void nmi_trap(registers_t *regs) {
	PANIC("NMI Trap");
}

void breakpoint_trap(registers_t *regs) {
	PANIC("Breakpoint Trap");
}

void overflow_trap(registers_t *regs) {
	PANIC("Overflow Trap");
}

void bounds_check_fault(registers_t *regs) {
	PANIC("Bounds Check Fault");
}

void invalid_opcode_fault(registers_t *regs) {
	kprintf(K_ERROR, "Invalid opcode.\n");
	kprintf(K_NONE, "\t\teip: %x, error code: %x\n", regs->eip, regs->err_code);

	PANIC("Invalid Opcode Fault");
}

void no_device_fault(registers_t *regs) {
	PANIC("No Device Fault");
}

void double_fault_abort(registers_t *regs) {
	PANIC("Double Fault Abort");
}

void invalid_tss_fault(registers_t *regs) {
	PANIC("Invalid TSS Fault");
}

void no_segment_fault(registers_t *regs) {
	PANIC("No Segment Fault");
}

void stack_fault(registers_t *regs) {
	PANIC("Stack Fault");
}

void general_protection_fault(registers_t *regs) {
	PANIC("General Protection Fault");
}

void register_exceptions() {
	register_interrupt_handler(0, div_by_zero);
	register_interrupt_handler(1, single_step_trap);
	register_interrupt_handler(2, nmi_trap);
	register_interrupt_handler(3, breakpoint_trap);
	register_interrupt_handler(4, overflow_trap);
	register_interrupt_handler(5, bounds_check_fault);
	register_interrupt_handler(6, invalid_opcode_fault);
	register_interrupt_handler(7, no_device_fault);
	register_interrupt_handler(8, double_fault_abort);
	register_interrupt_handler(10, invalid_tss_fault);
	register_interrupt_handler(11, no_segment_fault);
	register_interrupt_handler(12, stack_fault);
	register_interrupt_handler(13, general_protection_fault);
}