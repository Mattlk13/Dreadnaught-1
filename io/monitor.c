// monitor.c -- Brad Slayter

#include "io/monitor.h"
#include "lib/common.h"

u16int cursorX;
u16int cursorY;
u16int *terminalBuffer = (u16int *)0xB8000;

enum vgaColor {
	COLOR_BLACK = 0,
	COLOR_BLUE = 1,
	COLOR_GREEN = 2,
	COLOR_CYAN = 3,
	COLOR_RED = 4,
	COLOR_MAGENTA = 5,
	COLOR_BROWN = 6,
	COLOR_LIGHT_GREY = 7,
	COLOR_DARK_GREY = 8,
	COLOR_LIGHT_BLUE = 9,
	COLOR_LIGHT_GREEN = 10,
	COLOR_LIGHT_CYAN = 11,
	COLOR_LIGHT_RED = 12,
	COLOR_LIGHT_MAGENTA = 13,
	COLOR_LIGHT_BROWN = 14,
	COLOR_WHITE = 15,
};

u8int backColor = COLOR_BLACK;
u8int foreColor = COLOR_WHITE;

static void move_cursor() {
	u16int cursorLocation = cursorY * 80 + cursorX;
	outb(0x3D4, 14);
	outb(0x3D5, cursorLocation >> 8);
	outb(0x3d4, 15);
	outb(0x3D5, cursorLocation);
}

static void scroll() {
	u8int attributeByte = (0 << 4) | (15 & 0x0F);
	u16int blank = 0x20 | (attributeByte << 8);

	if (cursorY >= 25) {
		int i;
		for (i = 0*80; i < 24*80; i++) {
			terminalBuffer[i] = terminalBuffer[i+80];
		}

		for (i = 24*80; i < 25*80; i++) {
			terminalBuffer[i] = blank;
		}

		cursorY = 24;
	}
}

void mon_put(char c) {
	/*backColor = COLOR_BLACK;
	foreColor = COLOR_WHITE;*/

	u8int attributeByte = (backColor << 4) | (foreColor & 0x0F);
	u16int attribute = attributeByte << 8;
	u16int *location;

	if (c == 0x08 && cursorX) { // backspace byte
		cursorX--;
	} else if (c == 0x09) {
		cursorX = (cursorX + 4) & ~(4-1);
	} else if (c == '\r') {
		cursorX = 0;
	} else if (c == '\n') {
		cursorX = 0;
		cursorY++;
	} else if (c >= ' ') {
		location = terminalBuffer + (cursorY*80 + cursorX);
		*location = c | attribute;
		cursorX++;
	}

	if (cursorX >= 80) {
		cursorX = 0;
		cursorY++;
	}

	scroll();
	move_cursor();
}

void mon_clear() {
	u8int attributeByte = (0 << 4) | (15 & 0x0F);
	u16int blank = 0x20 | (attributeByte << 8);

	int i;
	for (i = 0; i < 80*25; i++) {
		terminalBuffer[i] = blank;
	}

	cursorX = 0;
	cursorY = 0;
	move_cursor();
}

void mon_write(const char *c) {
	int i = 0;
	while (c[i]) {
		mon_put(c[i++]);
	}
}