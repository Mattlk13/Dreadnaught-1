// gfx.h -- Brad Slayter

#ifndef GFX_H
#define GFX_H

#include "lib/common.h"

typedef struct {
	u32int height;
	u32int width;
	unsigned char *data;
} BITMAP;

void go_gfx();
void go_text();
void draw_bitmap(BITMAP *bmp, u32int x, u32int y);

#endif