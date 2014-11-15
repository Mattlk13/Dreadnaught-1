// task.h -- Brad Slayter

#ifndef TASK_H
#define TASK_H

#include "lib/common.h"
#include "mm/virtmem.h"

typedef struct Registers {
	u32int eax, ebx, ecx, edx, esi, edi, esp, ebp, eip, eflags, cr3;
} Registers_t;

typedef struct Task {
	Registers_t regs;
	struct Task *next;
} task_t;

extern void preempt();
extern void switch_task(Registers_t *old, Registers_t *new);
void initialize_tasking();

#endif