// task.h -- Brad Slayter

#ifndef TASK_H
#define TASK_H

#include "lib/common.h"
#include "mm/virtmem.h"

typedef struct task {
	int id;
	u32int esp, ebp;
	u32int eip;
	pdirectory *page_directory;
	struct task *next;
} task_t;

void initialize_tasking();
void task_switch();
int fork();
int getpid();

#endif