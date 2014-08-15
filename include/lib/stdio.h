// stdio.h -- Brad Slayter

#ifndef STDIO_H
#define STDIO_H

#include <stdarg.h>
#include "lib/common.h"

enum info_levels {
	K_ERROR = 0,
	K_WARN  = 1,
	K_INFO  = 2
};

void kprintf(int level, const char *format, ...);

#endif