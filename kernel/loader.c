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
			header = (Elf32_Header *)page;

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

	// load segments into memory
	

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