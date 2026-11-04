// gfx.h — minimal RGB565 framebuffer helpers for the 3DS.
#pragma once
#include <stdint.h>

typedef uint16_t u16;

static inline u16 gfx_rgb(int r, int g, int b) {
	return (u16)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | ((b & 0xF8) >> 3));
}

void gfx_clear(u16* fb, int stride, int w, int h, u16 color);
void gfx_fill(u16* fb, int stride, int x, int y, int w, int h, u16 color);
void gfx_rect(u16* fb, int stride, int x, int y, int w, int h, u16 color);
void gfx_char(u16* fb, int stride, int x, int y, char c, u16 fg, u16 bg);
void gfx_str(u16* fb, int stride, int x, int y, const char* s, u16 fg, u16 bg);
int  gfx_strw(const char* s);   // width in px of a string
void gfx_bar(u16* fb, int stride, int x, int y, int w, int h, float frac, u16 color);
