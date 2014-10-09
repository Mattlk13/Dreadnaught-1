// loader.c -- Brad Slayter

#include "kernel/loader.h"
#include "kernel/descriptor_tables.h"
#include "fs/vfs.h"
#include "mm/physmem.h"
#include "lib/stdio.h"
#include "kernel/cmd.h"

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
	Elf32_Header *header;
	pdirectory *address_space = virt_get_directory();
	process *proc;
	thread *mainThread;
	unsigned char *memory;
	unsigned char buf[4096];

	if (exe.flags == FS_INVALID)
		return PROCESS_INVALID_ID;

	int cnt = 0;
	while (!exe.eof) {
		unsigned char *page = (unsigned char *)mem_alloc_block();
		for (int i = 0; i < 8; i++) {
			if (exe.eof)
				break;

			vol_read_file(&exe, page+(512*i), 512);
		}

		if (cnt == 0)
			header = (Elf32_Header *)page; // Get head (hehe)

		cnt++;
	}

	if (header->e_ident[0] != ELFMAG0 || header->e_ident[1] != ELFMAG1
		|| header->e_ident[2] != ELFMAG2 || header->e_ident[3] != ELFMAG3) {
		kprintf(K_ERROR, "Not a valid ELF executable\n");

		mem_free_blocks(header, cnt);

		vol_close_file(&exe);
		return PROCESS_INVALID_ID;
	}


	kprintf(K_OK, "EXE Read!\n");
	kprintf(K_INFO, "Some header info:\n\tmachine: %d entry: %x\n\tph_off: %d sh_off: %d\n", 
		header->e_machine, header->e_entry, header->e_phoff, header->e_shoff);
	kprintf(K_NONE, "\tflags: %x\n\tehsize: %d phsize: %d\n",
		header->e_flags, header->e_ehsize, header->e_phentsize);
	kprintf(K_NONE, "\tphnum: %d\n\tshentsize: %d shnum: %d\n",
		header->e_phnum, header->e_shentsize, header->e_shnum);
	kprintf(K_NONE, "\tshstrndx: %d\n", header->e_shstrndx);

	// Do some process setup
	proc = get_current_process();
	proc->id = 1;
	proc->pageDirectory = address_space;
	proc->priority = 1;
	proc->state = PROCESS_STATE_ACTIVE;
	proc->threadCount = 1;

	// Thread setup
	mainThread = &proc->threads[0];
	mainThread->kernelStack = 0;
	mainThread->parent = proc;
	mainThread->priority = 1;
	mainThread->state = PROCESS_STATE_ACTIVE;
	mainThread->initialStack = 0;
	mainThread->stackLimit = (void *)((u32int)mainThread->initialStack + 4096);
	// NEED To set:
	//		imageBase
	//		imageSize
	//		Clear trap frame
	//		set trap eip
	//		set trap flags

	// load segments into memory
	for (int i = 0; i < header->e_phnum; i++) {
		Elf32_Phdr *phdr = (Elf32_Phdr *)((uintptr_t)header+(header->e_phoff+(header->e_phentsize*i)));

		if (phdr->p_type != 1)
			continue;

		proc->image.size = phdr->p_filesz;
		proc->image.entry = phdr->p_vaddr;

		kprintf(K_DEBUG, "phdr dump:\n\tp_filesz: %x p_vaddr: %x\n",
			phdr->p_filesz, phdr->p_vaddr);
		kprintf(K_NONE, "\tp_type: %d\n", phdr->p_type);

		int n = phdr->p_filesz / PAGE_SIZE;
		u32int *pHead; // head of block
		for (int j = 0; j < n; j++) {
			u32int *block = (u32int *)mem_alloc_block();

			virt_map_phys_addr(proc->pageDirectory,
				phdr->p_vaddr+j*PAGE_SIZE,
				(u32int)block,
				PTE_PRESENT|PTE_WRITABLE|PTE_USER);

			if (j == 0)
				pHead = block;
		}

		memcpy((void *)pHead, (void *)((uintptr_t)header + phdr->p_offset), phdr->p_filesz);
	}

	uintptr_t entry = (uintptr_t)header->e_entry;

	mainThread->imageBase = proc->image.entry;
	mainThread->imageSize = proc->image.size;
	memset(&mainThread->frame, 0, sizeof(trapFrame));
	mainThread->frame.eip = entry;
	mainThread->frame.flags = 0x200;

	void *stack = (void *)(mainThread->imageBase + mainThread->imageSize/* + PAGE_SIZE*/);
	void *stackPhys = (void *)mem_alloc_block();

	// map user stack
	virt_map_phys_addr(proc->pageDirectory,
		(u32int)stack, (u32int)stackPhys,
		PTE_PRESENT|PTE_WRITABLE|PTE_USER);

	mem_free_blocks(header, cnt);
	vol_close_file(&exe);

	// final init stuff
	mainThread->initialStack = stack;
	mainThread->frame.esp = (u32int)mainThread->initialStack;
	mainThread->frame.ebp = mainThread->frame.esp;

	asm volatile("cli");
	mem_load_PDBR((physical_addr)proc->pageDirectory);
	tss_set_stack(0x10, (u32int)stackPhys);

	kprintf(K_INFO, "Here we go.......\n");

	// Hold on to your butts!!
	asm volatile(" \
      pushl $0x23; \
      push %0; \
      pushl $0x200; \
      pushl $0x1B; \
      push %1; \
      mov $0x23, %%ax; \
      mov %%ax, %%ds; \
      mov %%ax, %%es; \
      mov %%ax, %%fs; \
      mov %%ax, %%gs; \
      iret; \
      ":: "r" (mainThread->frame.esp), "m" (mainThread->frame.eip));

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

	kprintf(K_OK, "Process done.\n");
	
	start_cmd_prompt();
}