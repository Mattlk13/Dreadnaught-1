// cmd.c -- Brad Slayter

#include "kernel/cmd.h"

#include "mm/physmem.h"

#include "lib/stdio.h"
#include "lib/string.h"

u8int should_exit = 0;

char get_line(char *buf) {
	//char str[1024];

	char c = 0;
	u8int i = 0;
	while (c != '\n') {
		c = getch();

		kprintf(K_NONE, "%c", c);
		if (c != '\n') {
			buf[i] = c;
			i++;
		}
	}
	buf[i] = '\0';
	//str[i] = '\0';

	//char *cmd = (char *)kmalloc(strlen(str));
	//strcpy(cmd, str);

	return 0;
}

void read_cmd() {
	kprintf(K_NONE, "root@heisenbergOS$ ");

	char buf[100];
	get_line(buf);

	if (!strcmp(buf, "help"))
		kprintf(K_NONE, "Welcome to HeisenbergOS!!\n\nList of Commands:\n...\nhelp?\n\n");
}

void start_cmd_prompt() {
	// TODO: fork and detach from kernel

	while (!should_exit) {
		read_cmd();
	}
}