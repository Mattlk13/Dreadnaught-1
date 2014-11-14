// task.c -- Brad Slayter

#include "kernel/task.h"
#include "mm/blk.h"
#include "lib/stdio.h"

volatile task_t *current_task;
volatile task_t *ready_queue;

extern pdirectory *kernel_directory;
extern pdirectory *cur_directory;
extern u32int read_eip();

u32int next_pid = 1;

int getpid() {
	if (current_task)
		return current_task->id;
	else
		return -1;
}

void initialize_tasking() {
	asm volatile("cli");

	current_task = ready_queue = (task_t *)malloc(sizeof(task_t));
	current_task->id = next_pid++;
	current_task->esp = current_task->ebp = 0;
	current_task->eip = 0;
	current_task->page_directory = cur_directory;
	current_task->next = 0;

	asm volatile("sti");
}

int fork() {
	asm volatile("cli");

	task_t *parent_task = (task_t *)current_task;

	pdirectory *directory = virt_clone_directory(cur_directory);

	task_t *new_task = (task_t *)malloc(sizeof(task_t));
	new_task->id = next_pid++;
	new_task->esp = new_task->ebp = 0;
	new_task->eip = 0;
	new_task->page_directory = directory;
	new_task->next = 0;

	task_t *tmp_task = (task_t *)ready_queue;
	while (tmp_task->next)
		tmp_task = tmp_task->next;

	tmp_task->next = new_task;

	u32int eip = read_eip();

	if (current_task == parent_task) {
		u32int esp; asm volatile("mov %%esp, %0" : "=r"(esp));
		u32int ebp; asm volatile("mov %%ebp, %0" : "=r"(ebp));
		new_task->esp = esp;
		new_task->ebp = ebp;
		new_task->eip = eip;

		asm volatile("sti");

		return new_task->id;
	} else {
		return 0;
	}
}

void task_switch() {
	if (!current_task)
		return;

	kprintf(K_DEBUG, "Switching task!\n");
	u32int esp, ebp, eip;
	asm volatile("mov %%esp, %0" : "=r"(esp));
	asm volatile("mov %%ebp, %0" : "=r"(ebp));

	eip = read_eip();
	if (eip == 0x12345)
		return;

	current_task->eip = eip;
	current_task->esp = esp;
	current_task->ebp = ebp;

	current_task = current_task->next;
	if (!current_task) current_task = ready_queue;

	esp = current_task->esp;
	ebp = current_task->ebp;

	//kprintf(K_DEBUG, "Lets do the do!\n");
	asm volatile(" \
		cli; \
		mov %0, %%ebx; \
		mov %1, %%esp; \
		mov %2, %%ebp; \
		mov %3, %%cr3; \
		mov $0x12345, %%eax; \
		sti; \
		jmp *%%ebx; \
		" : : "r"(eip), "r"(esp), "r"(ebp), "r"((physical_addr)&cur_directory->m_entries)
		  : "%ebx", "%esp", "%eax");
}