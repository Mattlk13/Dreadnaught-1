// monitor.h -- Brad Slayter

#ifndef MONITOR_H
#define MONITOR_H

#include "lib/common.h"

void mon_clear();

void mon_put(char c);
void mon_write(const char *c);
void mon_write_line(const char *c);
void DEBUG_mon_write(const char *c);
void mon_backspace();
void mon_write_hex(u32int n);
void mon_write_dec(u32int n);

#endif