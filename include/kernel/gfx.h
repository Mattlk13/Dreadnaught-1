// gfx.h -- Brad Slayter

#ifndef GFX_H
#define GFX_H

#include "lib/common.h"

typedef struct {
	u8int height;
	u8int width;
	char *data;
} BITMAP;

void go_gfx();

#endif