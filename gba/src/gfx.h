// gfx.h — minimal 8-bit palette-index drawing for GBA Mode 4 (two pages).
#ifndef GFX_H
#define GFX_H

#include <stdint.h>

#define SCREEN_W 240
#define SCREEN_H 160

void gfx_set_palette(int idx, int r, int g, int b);
void gfx_clear(volatile uint8_t* page, uint8_t color);
void gfx_fill(volatile uint8_t* page, int x, int y, int w, int h, uint8_t color);
void gfx_rect(volatile uint8_t* page, int x, int y, int w, int h, uint8_t color);
void gfx_char(volatile uint8_t* page, int x, int y, char c, uint8_t fg, uint8_t bg);
void gfx_str(volatile uint8_t* page, int x, int y, const char* s, uint8_t fg, uint8_t bg);
int  gfx_strw(const char* s);
void gfx_num(volatile uint8_t* page, int x, int y, int v, uint8_t fg, uint8_t bg);

#endif
