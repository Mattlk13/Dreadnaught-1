// cmd.c -- Brad Slayter

#include "kernel/cmd.h"
#include "kernel/descriptor_tables.h"
#include "kernel/loader.h"

#include "mm/physmem.h"

#include "lib/stdio.h"
#include "lib/string.h"
#include "lib/syscall.h"

#include "fs/vfs.h"

#include "io/monitor.h"


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

void cmd_help() {
	kprintf(K_NONE, "Welcome to the Dreadnaught Operating System!!\n\n");
	kprintf(K_NONE, "List of Commands:\n");
	kprintf(K_NONE, "read\tRead a file from the floppy.\n");
	kprintf(K_NONE, "help\tShow this message.\n");
	kprintf(K_NONE, "ls  \tList files in directory.\n");
	kprintf(K_NONE, "user\tEnter user mode\n");
	kprintf(K_NONE, "clr \tClear the screen\n");
}

void cmd_write_file() {
	kprintf(K_NONE, "Enter name of file to write: \n> ");
	char buf[101];
	get_line(buf);

	FILE file = vol_open_file(buf, F_WRITE);

	if (file.flags == FS_INVALID) {
		kprintf(K_ERROR, "[CMD] Unable to write file %s\n", buf);
		return;
	}

	kprintf(K_OK, "Succesfully wrote file!\n");
	vol_close_file(&file);
}

void cmd_read_file() {
	kprintf(K_NONE, "Enter the name of the file to read:\n> ");
	char buf[101];
	get_line(buf);

	FILE file = vol_open_file(buf, F_READ);

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
	vol_close_file(&file);
}

void cmd_ls() {
	vol_list_dir();
}

void cmd_user() {
	switch_to_user_mode();
	syscall_kprintf(K_NONE, "Welcome, to user mode!\n");
	for (;;);
}

void cmd_run() {
	kprintf(K_NONE, "Enter the name of the file to run:\n> ");
	char buf[101];
	get_line(buf);

	exec(buf, 0, 0, 0);
}

void cmd_clr_scr() {
	mon_clear();
}

void read_cmd() {
	kprintf(K_NONE, "root@dreadnaught$ ");

	char buf[100];
	get_line(buf);

	if (!strcmp(buf, "help"))
		cmd_help();
	else if (!strcmp(buf, "read"))
		cmd_read_file();
	else if (!strcmp(buf, "write"))
		cmd_write_file();
	else if (!strcmp(buf, "ls"))
		cmd_ls();
	else if (!strcmp(buf, "user"))
		cmd_user();
	else if (!strcmp(buf, "run"))
		cmd_run();
	else if (!strcmp(buf, "clr"))
		cmd_clr_scr();
}

void start_cmd_prompt() {
	// TODO: fork and detach from kernel

	while (!should_exit) {
		read_cmd();
	}
}