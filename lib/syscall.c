// syscall.c -- Brad Slayter

#include "lib/syscall.h"
#include "lib/stdio.h"

#include "kernel/isr.h"

static void syscall_handler(registers_t regs);

static void *syscalls[2] = {
	&kprintf,
	&getch
};
u32int num_syscalls = 2;

DEFN_SYSCALL2(kprintf, 0, int, const char *);
DEFN_SYSCALL0(getch, 1);

void initialize_syscalls() {
	register_interrupt_handler(0x80, &syscall_handler);
}

void syscall_handler(registers_t regs) {
	if (regs.eax >= num_syscalls)
		return;

	void *location = syscalls[regs.eax];

	int ret;
	asm volatile(" \
		push %1; \
		push %2; \
		push %3; \
		push %4; \
		push %5; \
		call *%6; \
		pop %%ebx; \
		pop %%ebx; \
		pop %%ebx; \
		pop %%ebx; \
		pop %%ebx; \
		" : "=a" (ret) : "r" (regs.edi), "r" (regs.esi), "r" (regs.edx), "r" (regs.ecx), "r" (regs.ebx), "r" (location));
	regs.eax = ret;
}