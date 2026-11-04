// GBA DnB break player — 16-bit Direct Sound streaming + control UI.
// 8-section screen: 4 break pads (top), PLAY/STOP/BPM−/BPM+ (bottom).
#include <stdint.h>

#include "regs.h"
#include "gfx.h"
#include "samples.h"

#define NBREAKS 4
#define SR_WORDS 11025          // words per second (each word = 2 samples)
#define BUFW 184                // words per 1/60s frame
#define NBUF 6

static const char* const BREAK_NAMES[NBREAKS] = {
	"CLN086", "CLN105", "PRC095", "PRC098"
};
static const int16_t* BRK[NBREAKS] = { brk0, brk1, brk2, brk3 };

static int brk_len_for(int i) {
	switch (i) {
		case 0: return brk0_len;
		case 1: return brk1_len;
		case 2: return brk2_len;
		default: return brk3_len;
	}
}

static uint32_t ring[NBUF][BUFW];
static int cur = 0, bpm = 175;
static int running = 0, playBuf = 0, fillBuf = 1;
static int samplePos = 0, sampleEnd = 0;

// ---- palette --------------------------------------------------------------
#define RGB5(r,g,b) ((uint16_t)((r) | ((g) << 5) | ((b) << 10)))
static const uint16_t COL_BG  = RGB5(12, 10, 9);
static const uint16_t COL_PAN = RGB5(58, 54, 50);
static const uint16_t COL_DIM = RGB5(17, 17, 16);
static const uint16_t COL_TXT = RGB5(29, 29, 28);
static const uint16_t COL_AMB = RGB5(31, 21, 5);
static const uint16_t COL_GRN = RGB5(10, 27, 15);
static const uint16_t COL_RED = RGB5(29, 10, 7);

static void draw_ui(void) {
	gfx_clear(COL_BG);

	gfx_str(4, 3, "AMENITY-DNB", COL_AMB, COL_BG);
	gfx_str(176, 3, "16bit", COL_DIM, COL_BG);

	gfx_str(4, 15, "BPM", COL_DIM, COL_BG);
	gfx_num(44, 15, bpm, COL_TXT, COL_BG);
	gfx_str(120, 15, running ? "PLAY" : "STOP",
			running ? COL_GRN : COL_RED, COL_BG);

	// 8 sections: top row = breaks, bottom row = transport/controls
	const char* top[4] = { "BRK1", "BRK2", "BRK3", "BRK4" };
	const char* bot[4] = { "PLAY", "STOP", "BPM-", "BPM+" };
	int x0 = 4, y0 = 34, cw = 57, ch = 42, gap = 3;
	for (int i = 0; i < 4; i++) {
		int x = x0 + i * (cw + gap);
		uint16_t c = (i == cur) ? COL_AMB : COL_PAN;
		gfx_fill(x, y0, cw, ch, c);
		gfx_rect(x, y0, cw, ch, i == cur ? COL_TXT : COL_DIM);
		int tw = gfx_strw(top[i]);
		gfx_str(x + (cw - tw) / 2, y0 + 6, top[i], i == cur ? 0x0000 : COL_TXT, c);
		tw = gfx_strw(BREAK_NAMES[i]);
		gfx_str(x + (cw - tw) / 2, y0 + 18, BREAK_NAMES[i], COL_DIM, c);

		int y = y0 + ch + gap;
		uint16_t cc = COL_PAN;
		if (i == 0) cc = running ? COL_GRN : COL_PAN;
		if (i == 1) cc = running ? COL_PAN : COL_RED;
		gfx_fill(x, y, cw, ch, cc);
		gfx_rect(x, y, cw, ch, COL_DIM);
		tw = gfx_strw(bot[i]);
		gfx_str(x + (cw - tw) / 2, y + (ch - 7) / 2, bot[i],
				(i == 0 && running) || (i == 1 && !running) ? 0x0000 : COL_TXT, cc);
	}

	gfx_str(4, 142, "L/R brk   U/D bpm   A play", COL_DIM, COL_BG);
}

// ---- audio -----------------------------------------------------------------
static void fill_next(int force) {
	if (samplePos >= sampleEnd) {
		samplePos = 0;
		if (!running) return;
	}
	int n = sampleEnd - samplePos;
	if (n > BUFW * 2) n = BUFW * 2;
	const int16_t* s = BRK[cur] + samplePos;
	uint32_t* dst = ring[fillBuf];
	for (int w = 0; w < BUFW; w++) {
		int16_t a = (samplePos + w * 2 < sampleEnd) ? s[w * 2] : 0;
		int16_t b = (samplePos + w * 2 + 1 < sampleEnd) ? s[w * 2 + 1] : 0;
		dst[w] = ((uint32_t)(uint16_t)a) | ((uint32_t)(uint16_t)b << 16);
	}
	samplePos += BUFW * 2;
	REG_DMA1SAD = (uint32_t)dst;
	REG_DMA2SAD = (uint32_t)dst;
	playBuf = fillBuf;
	fillBuf = (fillBuf + 1) % NBUF;
}

static void set_timer(void) {
	uint32_t period = (uint32_t)(16777216u / SR_WORDS) * 175u / (uint32_t)bpm;
	if (period < 2) period = 2;
	REG_TM0CNT_L = (uint16_t)(period - 1);
	REG_TM0CNT_H = 0x0080;
}

static void start_audio(void) {
	samplePos = 0;
	sampleEnd = brk_len_for(cur);
	if (sampleEnd > 22050 * 5) sampleEnd = 22050 * 5;
	fillBuf = 1;
	fill_next(1);
	fill_next(1);
	set_timer();
	REG_DMA1CNT = DMA_ENABLE | DMA_REPEAT | DMA_AT_FIFO | DMA_DST_FIXED | DMA_32 | (BUFW & 0xFFFF);
	REG_DMA2CNT = DMA_ENABLE | DMA_REPEAT | DMA_AT_FIFO | DMA_DST_FIXED | DMA_32 | (BUFW & 0xFFFF);
	running = 1;
}

static void stop_audio(void) {
	running = 0;
	REG_DMA1CNT = 0;
	REG_DMA2CNT = 0;
	REG_TM0CNT_H = 0;
}

void main(void) {
	REG_DISPCNT = 0x0403;
	REG_SOUNDCNT_X = 0x0080;
	REG_SOUNDCNT_H = 0x0F0F;
	start_audio();

	int prevKeys = 0x03FF;
	for (;;) {
		while (REG_VCOUNT >= 160) ;
		while (REG_VCOUNT < 160) ;

		int keys = REG_KEYINPUT ^ 0x03FF;
		int down = keys & ~prevKeys;
		prevKeys = keys;

		if (down & KEY_RIGHT) { stop_audio(); cur = (cur + 1) % NBREAKS; start_audio(); }
		if (down & KEY_LEFT)  { stop_audio(); cur = (cur + NBREAKS - 1) % NBREAKS; start_audio(); }
		if (down & KEY_UP)    { if (bpm < 250) { bpm++; set_timer(); } }
		if (down & KEY_DOWN)  { if (bpm > 61) { bpm--; set_timer(); } }
		if (down & (KEY_A | KEY_START)) {
			if (running) stop_audio(); else start_audio();
		}
		if (down & KEY_B) { stop_audio(); cur = 0; start_audio(); }
		if (down & KEY_SELECT) { stop_audio(); cur = (cur + 2) % NBREAKS; start_audio(); }

		if (running) fill_next(0);
		draw_ui();
	}
}
