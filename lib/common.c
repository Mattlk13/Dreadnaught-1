// common.c -- Brad Slayter

#include "lib/common.h"

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

u32int strlen(char *str) {
	u32int len = 0;
	while (str[len])
		len++;

	return len;
}