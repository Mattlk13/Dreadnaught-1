// stdio.c -- Brad Slayter

#include "lib/stdio.h"
#include "io/monitor.h"

void set_color_for_level(int level) {
	switch (level) {
		case K_ERROR:
			mon_set_color(COLOR_BLACK, COLOR_RED);
			mon_write("ERROR");
			break;
		case K_WARN:
			mon_set_color(COLOR_BLACK, COLOR_LIGHT_GREEN);
			mon_write("WARN");
			break;
		case K_INFO:
			mon_set_color(COLOR_BLACK, COLOR_LIGHT_BLUE);
			mon_write("INFO");
			break;
		default:
			mon_set_color(COLOR_BLACK, COLOR_WHITE);
	}
}

void kprintf(int level, const char *format, ...) {
	mon_set_color(COLOR_BLACK, COLOR_WHITE); // print level
	mon_put('[');
	set_color_for_level(level);
	mon_set_color(COLOR_BLACK, COLOR_WHITE);
	mon_write("] ");

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
					//u32int toPrint = ;
					mon_write_dec(va_arg(list, u32int));
					break;
				case 's':
					//char *toPrint = varNext(list, char*);
					mon_write(va_arg(list, char*));
					break;
				case 'x':
					//u32int toPrint = va_arg(list, u32int);
					mon_write_hex(va_arg(list, u32int));
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