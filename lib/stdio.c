// stdio.c -- Brad Slayter

#include "lib/stdio.h"
#include "io/monitor.h"

#include "drivers/keyboard.h"

void set_color_for_level(int level) {
	switch (level) {
		case K_ERROR:
			mon_set_color(COLOR_BLACK, COLOR_RED);
			mon_write("ERROR");
			break;
		case K_WARN:
			mon_set_color(COLOR_BLACK, COLOR_LIGHT_MAGENTA);
			mon_write("WARN");
			break;
		case K_OK:
			mon_set_color(COLOR_BLACK, COLOR_LIGHT_GREEN);
			mon_write("OK");
			break;
		case K_INFO:
			mon_set_color(COLOR_BLACK, COLOR_LIGHT_CYAN);
			mon_write("INFO");
			break;
		default:
			mon_set_color(COLOR_BLACK, COLOR_WHITE);
	}
}

void kprintf(int level, const char *format, ...) {
	if (level != K_NONE) {
		mon_set_color(COLOR_BLACK, COLOR_WHITE); // print level
		mon_put('[');
		set_color_for_level(level);
		mon_set_color(COLOR_BLACK, COLOR_WHITE);
		mon_write("] ");
	}

	int i = 0;
	va_list list;
	va_start(list, format);
	u8int varNext = 0;
	while (format[i]) {
		if (format[i] == '%') {
			varNext = 1;
		} else if (varNext == 1) {
			switch (format[i]) {
				case 'd':
					mon_write_dec(va_arg(list, u32int));
					break;
				case 'x':
					mon_write_hex(va_arg(list, u32int));
					break;
				case 's':
					mon_write(va_arg(list, char*));
					break;
				case 'c':
					mon_put(va_arg(list, int));
					break;
				default:
					break;
			}
			varNext = 0;
		} else {
			mon_put(format[i]);
		}

		i++;
	}
}

char getch() {
	int key = KEY_UNKNOWN;

	while (key == KEY_UNKNOWN) {
		key = kb_get_last_key();
	}

	kb_discard_last_key();

	return kb_key_to_ascii(key);
}