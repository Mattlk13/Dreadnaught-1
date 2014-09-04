// common.c -- Brad Slayter

#include "lib/common.h"

#include "io/monitor.h"

void outb(u16int port, u8int value) {
	asm volatile("outb %1, %0" : : "dN" (port), "a" (value));
}

u8int inb(u16int port) {
	u8int ret;
	asm volatile("inb %1, %0" : "=a" (ret) : "dN" (port));
	return ret;
}

u16int inw(u16int port) {
	u16int ret;
	asm volatile("inw %1, %0" : "=a" (ret) : "dN" (port));
	return ret;
}

void memcpy(u8int *dest, const u8int *src, u32int len) {
    const u8int *sp = (const u8int *)src;
    u8int *dp = (u8int *)dest;
    for(; len != 0; len--) *dp++ = *sp++;
}

void memset(void *dest, u8int val, u32int len) {
    if (dest && len > 0) {
        unsigned char* pChar =  dest;
        int i = 0;
        for ( i = 0; i < len; ++i) {
            unsigned char temp = (unsigned char) val;
            *pChar++ = temp; // or pChar[i] = temp (they both don't work)
        }
    }
}

u32int strlen(const char *str) {
	u32int len = 0;
	while (str[len])
		len++;

	return len;
}

extern void panic(const char *message, const char *file, u32int line)
{
    // We encountered a massive problem and have to stop.
    asm volatile("cli"); // Disable interrupts.

    mon_write("PANIC(");
    mon_write(message);
    mon_write(") at ");
    mon_write(file);
    mon_write(":");
    mon_write_dec(line);
    mon_write("\n");
    // Halt by going into an infinite loop.
    for(;;);
}

extern void panic_assert(const char *file, u32int line, const char *desc)
{
    // An assertion failed, and we have to panic.
    asm volatile("cli"); // Disable interrupts.

    mon_write("ASSERTION-FAILED(");
    mon_write(desc);
    mon_write(") at ");
    mon_write(file);
    mon_write(":");
    mon_write_dec(line);
    mon_write("\n");
    // Halt by going into an infinite loop.
    for(;;);
}