// GBA DnB break player (16-bit Direct Sound streaming).
// Plays real breaks at 22050 Hz via FIFO DMA (tonc semantics) with a small
// double-buffer ring. Controls:
//   LEFT/RIGHT  next/prev break
//   UP/DOWN     BPM (scales playback rate)
//   A           play/stop
//   START       restart
#include <stdint.h>

#include "regs.h"
#include "samples.h"

#define NBREAKS 4
#define SR_WORDS 11025          // words per second (each word = 2 samples)
#define BUFW 184                // words per 1/60s frame (~1 frame of audio)
#define NBUF 6

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
static int samplePos = 0;        // current sample index into BRK[cur]
static int sampleEnd = 0;

static void fill_next(int force) {
	if (samplePos >= sampleEnd) {
		if (!running || force) {
			samplePos = 0;
			if (!running) return;
		}
	}
	int n = sampleEnd - samplePos;
	if (n > BUFW * 2) n = BUFW * 2;
	const int16_t* s = BRK[cur] + samplePos;
	uint32_t* dst = ring[fillBuf];
	int w = 0;
	while (w < BUFW) {
		int16_t a = (samplePos + w * 2 < sampleEnd) ? s[w * 2] : 0;
		int16_t b = (samplePos + w * 2 + 1 < sampleEnd) ? s[w * 2 + 1] : 0;
		dst[w] = ((uint32_t)(uint16_t)a) | ((uint32_t)(uint16_t)b << 16);
		w++;
	}
	samplePos += BUFW * 2;

	// point both DMA channels at the newly filled buffer
	REG_DMA1SAD = (uint32_t)dst;
	REG_DMA2SAD = (uint32_t)dst;
	playBuf = fillBuf;
	fillBuf = (fillBuf + 1) % NBUF;
}

static void set_timer(void) {
	// timer0 reload for SR_WORDS words/s at current BPM (base 175)
	uint32_t period = (uint32_t)(16777216u / SR_WORDS) * 175u / (uint32_t)bpm;
	if (period < 2) period = 2;
	REG_TM0CNT_L = (uint16_t)(period - 1);
	REG_TM0CNT_H = 0x0080;       // start (no irq)
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
	REG_DISPCNT = 0x0403;        // mode 3, bg2 (black)
	REG_SOUNDCNT_X = 0x0080;     // master enable
	REG_SOUNDCNT_H = 0x0F0F;     // FIFO A -> both sides, full volume
	start_audio();

	int prevKeys = 0x03FF;

	for (;;) {
		// hold vsync
		while (REG_VCOUNT >= 160) ;
		while (REG_VCOUNT < 160) ;

		int keys = REG_KEYINPUT ^ 0x03FF;   // pressed = 1
		int down = keys & ~prevKeys;
		prevKeys = keys;

		if (down & KEY_RIGHT) { stop_audio(); cur = (cur + 1) % NBREAKS; start_audio(); }
		if (down & KEY_LEFT)  { stop_audio(); cur = (cur + NBREAKS - 1) % NBREAKS; start_audio(); }
		if (down & KEY_UP)    { if (bpm < 250) { bpm++; set_timer(); } }
		if (down & KEY_DOWN)  { if (bpm > 61) { bpm--; set_timer(); } }
		if (down & (KEY_A | KEY_START)) {
			if (running) stop_audio(); else start_audio();
		}

		if (running) fill_next(0);
	}
}
