// gfx.h — RGB555 framebuffer helpers for GBA Mode 3.
#ifndef GFX_H
#define GFX_H

#include <stdint.h>

#define VRAM ((volatile uint16_t*)0x06000000)
#define SCREEN_W 240
#define SCREEN_H 160

#define RGB5(r,g,b) ((uint16_t)((r) | ((g) << 5) | ((b) << 10)))

void gfx_clear(uint16_t color);
void gfx_fill(int x, int y, int w, int h, uint16_t color);
void gfx_rect(int x, int y, int w, int h, uint16_t color);
void gfx_char(int x, int y, char c, uint16_t fg, uint16_t bg);
void gfx_str(int x, int y, const char* s, uint16_t fg, uint16_t bg);
int  gfx_strw(const char* s);
void gfx_num(int x, int y, int v, uint16_t fg, uint16_t bg);

#endif
