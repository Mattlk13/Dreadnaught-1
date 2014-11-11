// common.h -- Brad Slayter

#ifndef COMMON_H
#define COMMON_H

#define DEBUG 1

#define NULL 0

typedef unsigned long int u64int;
typedef unsigned int   u32int;
typedef			 int   s32int;
typedef unsigned short u16int;
typedef			 short s16int;
typedef unsigned char  u8int;
typedef			 char  s8int;

typedef long unsigned int size_t;
typedef unsigned long uintptr_t; // big enough to hold a void ptr

// Hardware speaking functions
void outb(u16int port, u8int value);
u8int inb(u16int port);
u16int inw(u16int port);
void outportsm(unsigned short port, unsigned char * data, unsigned long size);
void inportsm(unsigned short port, unsigned char * data, unsigned long size);

// mem
void *memcpy(void *dest, const void *src, size_t len);
void *memset(void *dest, char val, size_t len);

// string length
u32int strlen(const char *str);

#define PANIC(msg) panic(msg, __FILE__, __LINE__);
#define ASSERT(b) ((b) ? (void)0 : panic_assert(__FILE__, __LINE__, #b))

extern void panic(const char *message, const char *file, u32int line);
extern void panic_assert(const char *file, u32int line, const char *desc);

#endif