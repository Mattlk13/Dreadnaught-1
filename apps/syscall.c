#include "syscall.h"

DEFN_SYSCALL2(kprintf, 0, int, const char *);
DEFN_SYSCALL0(getch, 1);
DEFN_SYSCALL1(mon_write, 2, const char *);
DEFN_SYSCALL0(terminateProcess, 3);