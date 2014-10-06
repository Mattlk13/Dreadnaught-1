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
	for (uintptr_t i = 0; i < (u32int)header->e_shentsize * header->e_shnum; i += header->e_shentsize) {
		// get section header
		Elf32_Shdr *shdr = (Elf32_Shdr *)((uintptr_t)header + (header->e_shoff + i));
		if (shdr->sh_addr) {
			kprintf(K_DEBUG, "Section header found!\n");
			//if (shdr->sh_addr < proc->image.entry) {
				// set the lowest entry point
				proc->image.entry = shdr->sh_addr;
				kprintf(K_NONE, "\n\tSetting new image entry to %x\n", shdr->sh_addr);
			//}

			proc->image.size = shdr->sh_addr + shdr->sh_size - proc->image.entry;

			if (shdr->sh_type == SHT_NOBITS) {
				memset((void *)(shdr->sh_addr), 0x0, shdr->sh_size);
			} else {
				int n = shdr->sh_size / PAGE_SIZE;
				u32int *sHead; // head of block
				for (int j = 0; j < n; j++) {
					// allocate memory
					u32int *block = (u32int *)mem_alloc_block();

					// map the new block to where the process wants
					virt_map_phys_addr(proc->pageDirectory, 
						shdr->sh_addr+j*PAGE_SIZE,
						(u32int)block,
						PTE_PRESENT|PTE_WRITABLE|PTE_USER);

					if (j == 0)
						sHead = block; // get a reference to the start of mem
				}

				memcpy((void *)sHead, (void *)((uintptr_t)header + shdr->sh_offset), shdr->sh_size);
			}
		}
	}

	uintptr_t entry = (uintptr_t)header->e_entry;

	mainThread->imageBase = proc->image.entry;
	mainThread->imageSize = proc->image.size;
	memset(&mainThread->frame, 0, sizeof(trapFrame));
	mainThread->frame.eip = entry;
	mainThread->frame.flags = 0x200;

	mem_free_blocks(header, cnt);
	vol_close_file(&exe);

	void *stack = (void *) = (void *)(mainThread->imageBase + mainThread->imageSize + PAGE_SIZE);
	void *stackPhys = (void *)mem_alloc_block();

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