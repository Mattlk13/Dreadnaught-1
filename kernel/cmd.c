// cmd.c -- Brad Slayter

#include "kernel/cmd.h"

#include "mm/physmem.h"

#include "lib/stdio.h"
#include "lib/string.h"

#include "fs/vfs.h"

u8int should_exit = 0;

char get_line(char *buf) {
	char c = 0;
	u8int i = 0;
	while (c != '\n') {
		c = getch();

		kprintf(K_NONE, "%c", c);
		if (c == 0x08) {
			i--;
			continue;
		} else if (c != '\n') {
			buf[i] = c;
			i++;
		}
	}
	buf[i] = '\0';

	return 0;
}

void cmd_read_file() {
	kprintf(K_NONE, "Enter the name of the file to read:\n> ");
	char buf[11];
	get_line(buf);

	FILE file = vol_open_file(buf);

	if (file.flags == FS_INVALID) {
		kprintf(K_ERROR, "Unable to open file %s\n", buf);
		return;
	}

	while (!file.eof) {
		unsigned char fileBuf[512];

		vol_read_file(&file, fileBuf, 512);

		kprintf(K_NONE, "%s", fileBuf);

		if (!file.eof) {
			kprintf(K_NONE, "\nPress the any key to continue...");
			getch();
			kprintf(K_NONE, "\n");
		}
	}

	kprintf(K_NONE, "\n============= EOF =============\n");
}

void read_cmd() {
	kprintf(K_NONE, "root@heisenbergOS$ ");

	char buf[100];
	get_line(buf);

	if (!strcmp(buf, "help"))
		kprintf(K_NONE, "Welcome to HeisenbergOS!!\n\nList of Commands:\n...\nhelp?\n\n");
	else if (!strcmp(buf, "read"))
		cmd_read_file();
}

void start_cmd_prompt() {
	// TODO: fork and detach from kernel

	while (!should_exit) {
		read_cmd();
	}
}