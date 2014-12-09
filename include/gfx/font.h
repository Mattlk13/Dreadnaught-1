// font.h -- Brad Slayter

#ifndef FONT_H
#define FONT_H

#include "lib/common.h"

#define FONT_WIDTH  6
#define FONT_HEIGHT 7

void draw_char(char c, u32int x, u32int y);
void init_font();

#endif