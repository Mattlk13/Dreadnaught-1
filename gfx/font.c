// font.c -- Brad Slayter

#include "gfx/font.h"
#include "kernel/gfx.h"
#include "mm/blk.h"

BITMAP *font_set;

#define CHAR_BYTE_SIZE (FONT_HEIGHT*FONT_WIDTH)

void init_font() {
	font_set = (BITMAP *)malloc(sizeof(BITMAP) * 72);

	int i;
	for (i = 0; i < (26*2)+10; i++) { // set sizes for letters and numbers
		font_set[i].height = FONT_HEIGHT;
		font_set[i].width = FONT_WIDTH;
	}

	for (; i < 72; i++) { // set sizes for special chars
		switch (i) {
			case 66: // "(,*,?,)"
			case 67:
			case 69:
			case 71:
				font_set[i].height = FONT_HEIGHT;
				font_set[i].width = 5;
				break;
			default:
				font_set[i].height = FONT_HEIGHT;
				font_set[i].width = 2;
				break;
		}
	}
	i = 0;

	char a[] = {0,0,0,0,0,0,\
                0,0,0,0,0,0,\
                0,0,0,0,0,0,\
                0,0,0,0,0,0,\
                0,0,0,0,0,0,\
                0,0,0,0,0,0,\
                0,0,0,0,0,0};
    font_set[i].data = (unsigned char *)malloc(CHAR_BYTE_SIZE);
    memcpy(font_set[i].data, a, CHAR_BYTE_SIZE);
    i++;

    char b[] = {0,0,0,0,0,0,\
                0,0,0,0,0,0,\
                0,0,0,0,0,0,\
                0,0,0,0,0,0,\
                0,0,0,0,0,0,\
                0,0,0,0,0,0,\
                0,0,0,0,0,0};
    font_set[i].data = (unsigned char *)malloc(CHAR_BYTE_SIZE);
    memcpy(font_set[i].data, b, CHAR_BYTE_SIZE);
    i++;

    char c[] = {0,0,0,0,0,0,\
                0,0,0,0,0,0,\
                0,0,0,0,0,0,\
                0,0,0,0,0,0,\
                0,0,0,0,0,0,\
                0,0,0,0,0,0,\
                0,0,0,0,0,0};
    font_set[i].data = (unsigned char *)malloc(CHAR_BYTE_SIZE);
    memcpy(font_set[i].data, c, CHAR_BYTE_SIZE);
    i++;

    char d[] = {0,0,0,0,0,0,\
                0,0,0,0,0,0,\
                0,0,0,0,0,0,\
                0,0,0,0,0,0,\
                0,0,0,0,0,0,\
                0,0,0,0,0,0,\
                0,0,0,0,0,0};
    font_set[i].data = (unsigned char *)malloc(CHAR_BYTE_SIZE);
    memcpy(font_set[i].data, d, CHAR_BYTE_SIZE);
    i++;

    char e[] = {0,0,0,0,0,0,\
                0,0,0,0,0,0,\
                0,0,0,0,0,0,\
                0,0,0,0,0,0,\
                0,0,0,0,0,0,\
                0,0,0,0,0,0,\
                0,0,0,0,0,0};
    font_set[i].data = (unsigned char *)malloc(CHAR_BYTE_SIZE);
    memcpy(font_set[i].data, e, CHAR_BYTE_SIZE);
    i++;

    char f[] = {0,0,0,0,0,0,\
                0,0,0,0,0,0,\
                0,0,0,0,0,0,\
                0,0,0,0,0,0,\
                0,0,0,0,0,0,\
                0,0,0,0,0,0,\
                0,0,0,0,0,0};
    font_set[i].data = (unsigned char *)malloc(CHAR_BYTE_SIZE);
    memcpy(font_set[i].data, f, CHAR_BYTE_SIZE);
    i++;

    char g[] = {0,0,0,0,0,0,\
                0,0,0,0,0,0,\
                0,0,0,0,0,0,\
                0,0,0,0,0,0,\
                0,0,0,0,0,0,\
                0,0,0,0,0,0,\
                0,0,0,0,0,0};
    font_set[i].data = (unsigned char *)malloc(CHAR_BYTE_SIZE);
    memcpy(font_set[i].data, g, CHAR_BYTE_SIZE);
    i++;

    char h[] = {0,0,0,0,0,0,\
                0,0,0,0,0,0,\
                0,0,0,0,0,0,\
                0,0,0,0,0,0,\
                0,0,0,0,0,0,\
                0,0,0,0,0,0,\
                0,0,0,0,0,0};
    font_set[i].data = (unsigned char *)malloc(CHAR_BYTE_SIZE);
    memcpy(font_set[i].data, h, CHAR_BYTE_SIZE);
    i++;

    font_set[i].width = 2;
    char ic[] = {0,0,\
                 0,0,\
                 0,0,\
                 0,0,\
                 0,0,\
                 0,0,\
                 0,0};
    font_set[i].data = (unsigned char *)malloc(CHAR_BYTE_SIZE);
    memcpy(font_set[i].data, ic, CHAR_BYTE_SIZE);
    i++;

    char j[] = {0,0,0,0,0,0,\
                0,0,0,0,0,0,\
                0,0,0,0,0,0,\
                0,0,0,0,0,0,\
                0,0,0,0,0,0,\
                0,0,0,0,0,0,\
                0,0,0,0,0,0};
    font_set[i].data = (unsigned char *)malloc(CHAR_BYTE_SIZE);
    memcpy(font_set[i].data, j, CHAR_BYTE_SIZE);
    i++;

    char k[] = {0,0,0,0,0,0,\
                0,0,0,0,0,0,\
                0,0,0,0,0,0,\
                0,0,0,0,0,0,\
                0,0,0,0,0,0,\
                0,0,0,0,0,0,\
                0,0,0,0,0,0};
    font_set[i].data = (unsigned char *)malloc(CHAR_BYTE_SIZE);
    memcpy(font_set[i].data, k, CHAR_BYTE_SIZE);
    i++;

    font_set[i].width = 3;
    char l[] = {0,0,0,\
                0,0,0,\
                0,0,0,\
                0,0,0,\
                0,0,0,\
                0,0,0,\
                0,0,0};
    font_set[i].data = (unsigned char *)malloc(CHAR_BYTE_SIZE);
    memcpy(font_set[i].data, l, CHAR_BYTE_SIZE);
    i++;

    char m[] = {0,0,0,0,0,0,\
                0,0,0,0,0,0,\
                0,0,0,0,0,0,\
                0,0,0,0,0,0,\
                0,0,0,0,0,0,\
                0,0,0,0,0,0,\
                0,0,0,0,0,0};
    font_set[i].data = (unsigned char *)malloc(CHAR_BYTE_SIZE);
    memcpy(font_set[i].data, m, CHAR_BYTE_SIZE);
    i++;

    char n[] = {0,0,0,0,0,0,\
                0,0,0,0,0,0,\
                0,0,0,0,0,0,\
                0,0,0,0,0,0,\
                0,0,0,0,0,0,\
                0,0,0,0,0,0,\
                0,0,0,0,0,0};
    font_set[i].data = (unsigned char *)malloc(CHAR_BYTE_SIZE);
    memcpy(font_set[i].data, n, CHAR_BYTE_SIZE);
    i++;

    char o[] = {0,0,0,0,0,0,\
                0,0,0,0,0,0,\
                0,0,0,0,0,0,\
                0,0,0,0,0,0,\
                0,0,0,0,0,0,\
                0,0,0,0,0,0,\
                0,0,0,0,0,0};
    font_set[i].data = (unsigned char *)malloc(CHAR_BYTE_SIZE);
    memcpy(font_set[i].data, o, CHAR_BYTE_SIZE);
    i++;

    char p[] = {0,0,0,0,0,0,\
                0,0,0,0,0,0,\
                0,0,0,0,0,0,\
                0,0,0,0,0,0,\
                0,0,0,0,0,0,\
                0,0,0,0,0,0,\
                0,0,0,0,0,0};
    font_set[i].data = (unsigned char *)malloc(CHAR_BYTE_SIZE);
    memcpy(font_set[i].data, p, CHAR_BYTE_SIZE);
    i++;

    char q[] = {0,0,0,0,0,0,\
                0,0,0,0,0,0,\
                0,0,0,0,0,0,\
                0,0,0,0,0,0,\
                0,0,0,0,0,0,\
                0,0,0,0,0,0,\
                0,0,0,0,0,0};
    font_set[i].data = (unsigned char *)malloc(CHAR_BYTE_SIZE);
    memcpy(font_set[i].data, q, CHAR_BYTE_SIZE);
    i++;

    char r[] = {0,0,0,0,0,0,\
                0,0,0,0,0,0,\
                0,15,0,15,15,0,\
                0,15,15,0,0,15,\
                0,15,0,0,0,0,\
                0,15,0,0,0,0,\
                0,15,0,0,0,0};
    font_set[i].data = (unsigned char *)malloc(CHAR_BYTE_SIZE);
    memcpy(font_set[i].data, r, CHAR_BYTE_SIZE);
    i++;

    char s[] = {0,0,0,0,0,0,\
                0,0,0,0,0,0,\
                0,0,15,15,15,15,\
                0,15,0,0,0,0,\
                0,0,15,15,15,0,\
                0,0,0,0,0,15,\
                0,15,15,15,15,0};
    font_set[i].data = (unsigned char *)malloc(CHAR_BYTE_SIZE);
    memcpy(font_set[i].data, s, CHAR_BYTE_SIZE);
    i++;

    char t[] = {0,0,15,0,0,0,\
                0,15,15,15,0,0,\
                0,0,15,0,0,0,\
                0,0,15,0,0,0,\
                0,0,15,0,0,0,\
                0,0,15,0,0,0,\
                0,0,0,15,0,0};
    font_set[i].data = (unsigned char *)malloc(CHAR_BYTE_SIZE);
    memcpy(font_set[i].data, t, CHAR_BYTE_SIZE);
    i++;

    char u[] = {0,0,0,0,0,0,\
                0,0,0,0,0,0,\
                0,15,0,0,0,15,\
                0,15,0,0,0,15,\
                0,15,0,0,0,15,\
                0,15,0,0,0,15,\
                0,0,15,15,15,15};
    font_set[i].data = (unsigned char *)malloc(CHAR_BYTE_SIZE);
    memcpy(font_set[i].data, u, CHAR_BYTE_SIZE);
    i++;

    char v[] = {0,0,0,0,0,0,\
                0,0,0,0,0,0,\
                0,15,0,0,0,15,\
                0,15,0,0,0,15,\
                0,15,0,0,0,15,\
                0,0,15,0,15,0,\
                0,0,0,15,0,0};
    font_set[i].data = (unsigned char *)malloc(CHAR_BYTE_SIZE);
    memcpy(font_set[i].data, v, CHAR_BYTE_SIZE);
    i++;

    char w[] = {0,0,0,0,0,0,\
                0,0,0,0,0,0,\
                0,15,0,0,0,15,\
                0,15,0,0,0,15,\
                0,15,0,15,0,15,\
                0,15,0,15,0,15,\
                0,0,15,15,15,15};
    font_set[i].data = (unsigned char *)malloc(CHAR_BYTE_SIZE);
    memcpy(font_set[i].data, w, CHAR_BYTE_SIZE);
    i++;

    char x[] = {0,0,0,0,0,0,\
                0,0,0,0,0,0,\
                0,15,0,0,0,15,\
                0,0,15,0,15,0,\
                0,0,0,15,0,0,\
                0,0,15,0,15,0,\
                0,15,0,0,0,15};
    font_set[i].data = (unsigned char *)malloc(CHAR_BYTE_SIZE);
    memcpy(font_set[i].data, x, CHAR_BYTE_SIZE);
    i++;

    char y[] = {0,0,0,0,0,0,\
                0,15,0,0,0,15,\
                0,15,0,0,0,15,\
                0,15,0,0,0,15,\
                0,0,15,15,15,15,\
                0,0,0,0,0,15,\
                0,15,15,15,15,0};
    font_set[i].data = (unsigned char *)malloc(CHAR_BYTE_SIZE);
    memcpy(font_set[i].data, y, CHAR_BYTE_SIZE);
    i++;

    char z[] = {0,0,0,0,0,0,\
                0,0,0,0,0,0,\
                0,15,15,15,15,15,\
                0,0,0,0,15,0,\
                0,0,0,15,0,0,\
                0,0,15,0,0,0,\
                0,15,15,15,15,15};
    font_set[i].data = (unsigned char *)malloc(CHAR_BYTE_SIZE);
    memcpy(font_set[i].data, z, CHAR_BYTE_SIZE);
    i++;

    // FONT TEST
    u32int xOff = 0;
    for (int idx = 0; idx < 26; idx++) {
    	draw_bitmap(&font_set[idx], xOff, 10);
    	xOff += font_set[idx].width;
    }
}

void draw_char(char c, u32int x, u32int y) {

}