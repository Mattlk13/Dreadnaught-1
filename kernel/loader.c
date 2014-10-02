// loader.c -- Brad Slayter

#include "kernel/loader.h"
#include "kernel/descriptor_tables.h"
#include "fs/vfs.h"
#include "mm/physmem.h"
#include "lib/stdio.h"

#ifndef PAGE_SIZE
#define PAGE_SIZE 4096
#endif

#define PROCESS_INVALID_ID -1
static process _proc = {
	PROCESS_INVALID_ID, 0, 0, 0, 0
};

process *get_current_process() {
	return &_proc;
}

int exec(char *path, int argc, char **argv, char **env) {
	FILE exe = vol_open_file(path, F_READ);
	pdirectory *address_space;
	process *proc;
	thread *mainThread;
	unsigned char *memory;
	unsigned char buf[4096];

	if (exe.flags == FS_INVALID)
		return PROCESS_INVALID_ID;

	// Read a page into buffer
	Elf32_Header *header = (Elf32_Header *)mem_alloc_block();
	for (int i = 0; i < 8; i++) {
		if (exe.eof != 1)
			vol_read_file(&exe, (unsigned char *)header+512*i, 512);
		else
			break;
	}
	header = (Elf32_Header *)header;
	kprintf(K_INFO, "Header: %s\n", header->e_ident);

	if (header->e_ident[0] != ELFMAG0 || header->e_ident[1] != ELFMAG1 ||
		header->e_ident[2] != ELFMAG2 || header-> e_ident[3] != ELFMAG3) {
		kprintf(K_ERROR, "Not a valid ELF image.\n");

		mem_free_block(header);
		vol_close_file(&exe);
		return PROCESS_INVALID_ID;
	}

	// get address space
	address_space = virt_get_directory();

	kprintf(K_DEBUG, "Setup process\n");
	// setup process
	proc = get_current_process();
	proc->id = 1;
	proc->pageDirectory = address_space;
	proc->priority = 1;
	proc->state = PROCESS_STATE_ACTIVE;
	proc->threadCount = 1;

	kprintf(K_DEBUG, "Load segments\n");
	// load loadable segments
	for (uintptr_t x = 0; x < (u32int)header->e_shentsize * header->e_shnum; x += header->e_shentsize) {
		Elf32_Shdr *shdr = (Elf32_Shdr *)((uintptr_t)header + (header->e_shoff + x));
		if (shdr->sh_addr) {
			if (shdr->sh_addr < proc->image.entry) {
				// if this is the lowest entry point, store it
				proc->image.entry = shdr->sh_addr;
			}

			if (shdr->sh_addr + shdr->sh_size - proc->image.entry > proc->image.size) {
				// store total size of memory used by process
				proc->image.size = shdr->sh_addr + shdr->sh_size - proc->image.entry;
			}

			for (uintptr_t i = 0; i < shdr->sh_size + 0x2000; i += 0x1000) {

			}

			if (shdr->sh_type == SHT_NOBITS) {
				// .bss
				memset((void *)(shdr->sh_addr), 0, shdr->sh_size);
			} else {
				// copy section into memory
				memcpy((void *)(shdr->sh_addr), (void *)((uintptr_t)header + shdr->sh_offset), shdr->sh_size);
			}
		}
	}

	kprintf(K_DEBUG, "Setup thread\n");
	mainThread = &proc->threads[0];
	mainThread->kernelStack = 0;
	mainThread->parent = proc;
	mainThread->priority = 1;
	mainThread->state = PROCESS_STATE_ACTIVE;
	mainThread->initialStack = 0;
	mainThread->stackLimit = (void *)((u32int)mainThread->initialStack + 4096);
	mainThread->imageBase = proc->image.entry;
	mainThread->imageSize = proc->image.size;
	memset(&mainThread->frame, 0, sizeof(trapFrame));
	mainThread->frame.eip = header->e_entry;
	mainThread->frame.flags = 0x200;

	uintptr_t entry = (uintptr_t)header->e_entry;

	memory = (unsigned char *)mem_alloc_block();
	memset(memory, 0, 4096);
	memcpy(memory, header, 4096);

	kprintf(K_DEBUG, "Map header to %x as %x\n", (u32int)memory, mainThread->imageBase);
	virt_map_phys_addr(proc->pageDirectory, mainThread->imageBase, (u32int)memory,
		PTE_PRESENT|PTE_WRITABLE|PTE_USER);

	kprintf(K_DEBUG, "Load rest of image\n");
	int i = 1;
	while (exe.eof != 1) {
		kprintf(K_DEBUG, "Load a page.\n");
		unsigned char *cur = mem_alloc_block();
		int curBlock = 0;
		for (curBlock = 0; curBlock < 8; curBlock++) {
			if (exe.eof)
				break;

			vol_read_file(&exe, cur+512*curBlock, 512);
		}

		virt_map_phys_addr(proc->pageDirectory, mainThread->imageBase+i*4096,
			(u32int)cur, PTE_PRESENT|PTE_WRITABLE|PTE_USER);
		break;
		i++;
	}

	kprintf(K_DEBUG, "Setup stack\n");
	void *stack = (void *)(mainThread->imageBase+mainThread->imageSize+PAGE_SIZE);
	void *stackPhys = (void *)mem_alloc_block();

	kprintf(K_DEBUG, "Map stack\n");
	// map user stack
	virt_map_phys_addr(address_space, (u32int)stack, (u32int)stackPhys,
		PTE_PRESENT|PTE_WRITABLE|PTE_USER);

	kprintf(K_DEBUG, "More Debug stuff\n");
	mainThread->initialStack = stack;
	mainThread->frame.esp = (u32int)mainThread->initialStack;
	mainThread->frame.ebp = mainThread->frame.ebp;

	kprintf(K_DEBUG, "Close file\n");
	vol_close_file(&exe);

	asm volatile("cli");
	//mem_load_PDBR((physical_addr)proc->pageDirectory);

	kprintf(K_DEBUG, "Setting TSS stack\n");
	int kstack = 0;
	asm volatile("mov %%esp, %0": "=r"(kstack));
	tss_set_stack(0x10, (u32int)stackPhys);

	kprintf(K_INFO, "Dumping some info:\n\tentry: %x stack: %x\n",
		entry, mainThread->frame.esp);

	kprintf(K_DEBUG, "MAKE THE JUMP!\n");
	asm volatile(" \
		mov $0x23, %%ax; \
		mov %%ax, %%ds; \
		mov %%ax, %%es; \
		mov %%ax, %%fs; \
		mov %%ax, %%gs; \
		pushl $0x23; \
		push %0; \
		pushl $0x200; \
		pushl $0x1B; \
		push %1; \
		iret; \
		":: "r" (mainThread->frame.esp), "r" (entry));
	kprintf(K_ERROR, "How did we get here...\n");

	return -1; // we should never get here
}

void terminateProcess() {
	process *cur = &_proc;
	if (cur->id == PROCESS_INVALID_ID)
		return; // someone dun goofed

	// release threads
	int i = 0;
	thread *pthread = &cur->threads[i];

	// get phys addr of stack
	void *stackFrame = virt_get_phys_addr(cur->pageDirectory, 
		(u32int)pthread->initialStack);

	// unmap and release
	virt_unmap_phys_addr(cur->pageDirectory, (u32int)pthread->initialStack);
	mem_free_block(stackFrame);

	// release rest of image
	for (u32int page = 0; page < pthread->imageSize/PAGE_SIZE; page++) {
		u32int phys = 0;
		u32int virt = 0;

		virt = pthread->imageBase + (page * PAGE_SIZE);

		phys = (u32int)virt_get_phys_addr(cur->pageDirectory, virt);

		virt_unmap_phys_addr(cur->pageDirectory, virt);
		mem_free_block((void *)phys);
	}

	asm volatile(" \
		cli; \
		mov $0x10, %eax; \
		mov %ax, %ds; \
		mov %ax, %es; \
		mov %ax, %fs; \
		mov %ax, %gs; \
		sti; \
		");

	kprintf(K_OK, "Process done. Halting becuase I don't know what to do\n\twith my life\n");
	for (;;);
}