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

void inportsm(unsigned short port, unsigned char * data, unsigned long size) {
    asm volatile ("rep insw" : "+D" (data), "+c" (size) : "d" (port) : "memory");
}

void *memcpy(void *dest, const void *src, size_t len) {
    const char *sp = (const char *)src;
    char *dp = (char *)dest;
    for(; len != 0; len--) *dp++ = *sp++;
    return dest;
}

void *memset(void *dest, char val, size_t len) {
    unsigned char *temp = (unsigned char *)dest;
    for (; len != 0; len--, temp[len] = val);
    return dest; 
}

u32int strlen(const char *str) {
	u32int len = 0;
	while (str[len])
		len++;

	return len;
}

static char *panicMsg = " \
                            --------\n\
                            |  X   X |\n\
                            |        |\n\
                            |  ----  |\n\
                            `--------`\n\
                            / d3adb0x \\\n\n";

extern void panic(const char *message, const char *file, u32int line)
{
    // We encountered a massive problem and have to stop.
    asm volatile("cli"); // Disable interrupts.

    mon_write(panicMsg);
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