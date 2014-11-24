// task.c -- Brad Slayter

#include "kernel/task.h"
#include "kernel/loader.h"
#include "mm/blk.h"
#include "mm/physmem.h"
#include "lib/stdio.h"

static task_t *running_task;
static task_t main_task;
static task_t other_task;

extern pdirectory *kernel_directory;
extern pdirectory *cur_directory;
extern u32int read_eip();

u32int next_pid = 1;

#define GET_FLAGS(X) asm volatile ("pushfl;\
                                    popl %%eax;       \
                                    movl %%eax, %0;"  \
                                    :"=m" (X)         \
                                    ); 

static void other_main() {
	kprintf(K_OK, "Hello multitasking! :D\n");
	preempt();
}

void create_task(task_t *task, void (*main)(), u32int flags, u32int *pagedir) {
	task->regs.eax = 0;
	task->regs.ebx = 0;
	task->regs.ecx = 0;
	task->regs.edx = 0;
	task->regs.esi = 0;
	task->regs.edi = 0;
	task->regs.eflags = flags;
	task->regs.eip = (u32int)main;
	task->regs.cr3 = (u32int)pagedir;
	task->regs.esp = (u32int)mem_alloc_block() + 0x1000;
	task->next = NULL;
}

void initialize_tasking() {
	main_task.regs.cr3 = (physical_addr)&cur_directory->m_entries;
	
	GET_FLAGS(main_task.regs.eflags);
	kprintf(K_INFO, "Flags are: %x\n", main_task.regs.eflags);

	create_task(&other_task, other_main, main_task.regs.eflags, (u32int *)main_task.regs.cr3);

	main_task.next = &other_task;
	other_task.next = &main_task;

	running_task = &main_task;
	preempt();
}

void preempt() {
	task_t *last = (running_task == NULL) ? &main_task : running_task;
	running_task = running_task->next;
	switch_task(&last->regs, &running_task->regs);
}