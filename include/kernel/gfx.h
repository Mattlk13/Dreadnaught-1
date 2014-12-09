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

#endif