#include "kernel/gfx.h"
#include "mm/physmem.h"
#include "lib/stdio.h"

// define our structure
typedef struct __attribute__ ((packed)) {
    unsigned short di, si, bp, sp, bx, dx, cx, ax;
    unsigned short gs, fs, es, ds, eflags;
} regs16_t;
 
// tell compiler our int32 function is external
extern void int32(unsigned char intnum, regs16_t *regs);

// int32 test
void int32_test()
{
    //int y;
    regs16_t regs;
     
    // switch to 320x200x256 graphics mode
    regs.ax = 0x0013;
    int32(0x10, &regs);
    
    // wait for key
    /*regs.ax = 0x0000;
    int32(0x16, &regs);
     
    // switch to 80x25x16 text mode
    regs.ax = 0x0003;
    int32(0x10, &regs);*/
}

void int32_text() {
	regs16_t regs;
	regs.ax = 0x0000;
    int32(0x16, &regs);
     
    // switch to 80x25x16 text mode
    regs.ax = 0x0003;
    int32(0x10, &regs);
}

void draw_rect(unsigned char x, unsigned char y, unsigned char w, unsigned char h, unsigned char color) {
	for (int i = y; i < y+h; i++) {
		memset((char *)0xA0000 + (i*320+x), color, w);
	}
}

void go_text() {
	mem_enable_paging(0);
    int32_text();
    mem_enable_paging(1);
}

void draw() {
	// full screen with blue color (1)
    memset((char *)0xA0000, 1, (320*200));
    draw_rect(80, 50, 160, 100, 0);
    draw_rect(90, 60, 30, 30, 2);
    draw_rect(200, 60, 30, 30, 2);
    draw_rect(105, 107, 110, 30, 2);
    // draw horizontal line from 100,80 to 100,240 in multiple colors
    /*for(int y = 50; y < 150; y++)
        memset((char *)0xA0000 + (y*320+80), y, 160);*/
    getch();
    go_text();
}

void go_gfx() {
    mem_enable_paging(0);
    int32_test();
    mem_enable_paging(1);
    draw();
}