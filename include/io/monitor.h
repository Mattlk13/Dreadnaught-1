// monitor.h -- Brad Slayter

#ifndef MONITOR_H
#define MONITOR_H

#include "lib/common.h"

void mon_putch(char c);
void mon_write(const char *c);

void mon_clear();

#endif