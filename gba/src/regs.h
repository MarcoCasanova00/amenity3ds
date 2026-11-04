// Minimal GBA register definitions (values from GBATEK public documentation).
#ifndef REGS_H
#define REGS_H

#include <stdint.h>

#define REG_DISPCNT   (*(volatile uint16_t*)0x04000000)
#define REG_DISPCNT_L (*(volatile uint16_t*)0x04000000)
#define REG_DISPCNT_H (*(volatile uint16_t*)0x04000002)
#define REG_VCOUNT    (*(volatile uint16_t*)0x04000006)
#define REG_KEYINPUT  (*(volatile uint16_t*)0x04000130)

#define REG_SOUNDCNT_X (*(volatile uint16_t*)0x04000084)
#define REG_SOUNDCNT_H (*(volatile uint16_t*)0x04000082)
#define REG_FIFO_A     (*(volatile uint32_t*)0x040000A0)
#define REG_FIFO_B     (*(volatile uint32_t*)0x040000A4)

#define REG_TM0CNT_L (*(volatile uint16_t*)0x04000100)
#define REG_TM0CNT_H (*(volatile uint16_t*)0x04000102)
#define REG_TM1CNT_L (*(volatile uint16_t*)0x04000104)
#define REG_TM1CNT_H (*(volatile uint16_t*)0x04000106)

#define REG_DMA1SAD (*(volatile uint32_t*)0x040000BC)
#define REG_DMA1DAD (*(volatile uint32_t*)0x040000C0)
#define REG_DMA1CNT (*(volatile uint32_t*)0x040000C4)
#define REG_DMA2SAD (*(volatile uint32_t*)0x040000C8)
#define REG_DMA2DAD (*(volatile uint32_t*)0x040000CC)
#define REG_DMA2CNT (*(volatile uint32_t*)0x040000D0)

#define REG_VRAM ((volatile uint16_t*)0x06000000)

#define VRAM_PAGE0 ((volatile uint8_t*)0x06000000)
#define VRAM_PAGE1 ((volatile uint8_t*)0x0600A000)
#define BG_PALETTE ((volatile uint16_t*)0x05000000)
#define REG_IME     (*(volatile uint16_t*)0x04000208)
#define REG_IE      (*(volatile uint16_t*)0x04000200)
#define REG_IF      (*(volatile uint16_t*)0x04000210)
#define IE_TIMER0   0x0001

// keys (bit set = not pressed)
#define KEY_A      0x0001
#define KEY_B      0x0002
#define KEY_SELECT 0x0004
#define KEY_START  0x0008
#define KEY_RIGHT  0x0010
#define KEY_LEFT   0x0020
#define KEY_UP     0x0040
#define KEY_DOWN   0x0080
#define KEY_R      0x0100
#define KEY_L      0x0200

// DMA flags
#define DMA_DST_FIXED 0x00400000
#define DMA_REPEAT     0x02000000
#define DMA_32         0x04000000
#define DMA_AT_FIFO    0x30000000
#define DMA_ENABLE     0x80000000

#endif
