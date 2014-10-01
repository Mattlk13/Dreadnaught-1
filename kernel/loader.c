// loader.c -- Brad Slayter

#include "kernel/loader.h"
#include "fs/vfs.h"
#include "mm/physmem.h"

#define PROCESS_INVALID_ID -1
static process _proc = {
	PROCESS_INVALID_ID, 0, 0, 0, 0
};

process *get_current_process() {
	return &_proc;
}

int exec(char *path, int argc, char **argv, char **env) {
	FILE exe = vol_open_file(path);
	if (exe.flags == FS_INVALID)
		return PROCESS_INVALID_ID;

	// Read entire image into buffer
	Elf32_Header *header = mem_alloc_block();
	for (u32int i = exe.fileLength - 4096; i > 0; i -= 4096)
		mem_alloc_block();
}