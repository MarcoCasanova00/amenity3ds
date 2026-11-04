// gfx.h — minimal RGB555 framebuffer helpers for GBA mode 3.
#ifndef GFX_H
#define GFX_H

#include <stdint.h>

#define VRAM ((volatile uint16_t*)0x06000000)
#define SCREEN_W 240
#define SCREEN_H 160

static inline uint16_t rgb555(int r, int g, int b) {
	return (uint16_t)((r & 0x1F) | ((g & 0x1F) << 5) | ((b & 0x1F) << 10));
}

void gfx_clear(uint16_t color);
void gfx_fill(int x, int y, int w, int h, uint16_t color);
void gfx_rect(int x, int y, int w, int h, uint16_t color);
void gfx_char(int x, int y, char c, uint16_t fg, uint16_t bg);
void gfx_str(int x, int y, const char* s, uint16_t fg, uint16_t bg);
int  gfx_strw(const char* s);
void gfx_num(int x, int y, int v, uint16_t fg, uint16_t bg);

#endif
