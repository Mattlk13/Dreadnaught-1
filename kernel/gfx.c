#include "kernel/gfx.h"
#include "mm/physmem.h"
#include "mm/blk.h"
#include "lib/stdio.h"
#include "gfx/font.h"
#include "io/monitor.h"
#include "kernel/cmd.h"

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

void draw_bitmap(BITMAP *bmp, u32int x, u32int y) {
	/*for (int i = y; i < y+bmp->height; i++) {
		for (int j = 0; j < bmp->width; j++) {
			memset((char *)0xA0000 + (i*320+(x+j)), bmp->data[i*bmp->width+j], 1);
		}
	}*/

    for (int py = 0; py < bmp->height; py++) {
        memcpy((char *)0xA0000 + ((y+py)*320+x), &bmp->data[py*bmp->width], bmp->width);
    }
}

void go_text() {
	mem_enable_paging(0);
    int32_text();
    mem_enable_paging(1);
    mon_clear();
    kprintf(K_OK, "Exited GFX mode\n");
}

void set_pixel(BITMAP *bmp, int x, int y, unsigned char color) {
    bmp->data[y * bmp->width + x] = color;
}

void draw_table_bitmap() {
    BITMAP *bmp;
    bmp->width = 16;
    bmp->height = 10;
    bmp->data = (unsigned char *)malloc(bmp->height * bmp->width);

    for (int y = 0; y < bmp->height; y++) {
        for (int x = 0; x < bmp->width; x++) {
            if (y < 2 || y > 7) {
                set_pixel(bmp, x, y, 0);
            } else if (y == 2) {
                if (x < 2 || x > 13)
                    set_pixel(bmp, x, y, 0);
                else
                    set_pixel(bmp, x, y, 4);
            } else {
                if (x < 4 || (x > 4 && x < 11) || x > 11)
                    set_pixel(bmp, x, y, 0);
                else
                    set_pixel(bmp, x, y, 4);
            }
        }
    }

    draw_bitmap(bmp, 20, 20);
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

    init_font();
    
    BITMAP bmp;
    bmp.height = 7;
    bmp.width = 6;

    char data[] = {0,0,0,0,0,0,\
                   0,0,0,0,0,0,\
                   0,0,15,15,15,0,\
                   0,0,0,0,0,15,\
                   0,0,15,15,15,15,\
                   0,15,0,0,0,15,\
                   0,0,15,15,15,15};
    
    memcpy(bmp.data, data, 6*7);
    draw_bitmap(&bmp, 20, 60);
    draw_table_bitmap();
    

    getch();
    go_text();
    start_cmd_prompt();
}

void go_gfx() {
    mem_enable_paging(0);
    int32_test();
    mem_enable_paging(1);
    draw();
}