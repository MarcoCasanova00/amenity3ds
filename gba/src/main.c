// GBA DnB break player — 8-bit Direct Sound FIFO (CPU-fed, IRQ-free)
// + PSG kit fallback.
//
// Audio path:
//   * Direct Sound FIFOs are 8-bit, one byte per Timer-0 overflow.
//   * Timer 0 runs at the sample rate (22.05 kHz at 175 BPM) and simply
//     drains the FIFO; the hardware needs no IRQ for this.
//   * The selected break is copied into an EWRAM buffer.  The main loop
//     (60 Hz, synced to VBlank) pushes exactly one frame's worth of samples
//     into FIFO A, spread across 16 VCOUNT-paced sub-batches so the small
//     32-byte FIFO never overflows.
//   * No interrupts at all: mGBA's HLE dispatches timer IRQs ~15x per
//     overflow, which would speed the break up drastically and starve the
//     UI.  The CPU-fed path is immune to that.
//
// UI: Mode 3, redrawn only on change (no flicker).
#include <stdint.h>
#include <string.h>

#include "regs.h"
#include "gfx.h"
#include "samples.h"

#define NBREAKS 4
#define BASE_BPM 175
#define BASE_RATE 22050
#define ABUF_SIZE 131072          // >= longest break (CLN086 = 123466 samples)
#define BATCHES 16

static int cur = 0, bpm = BASE_BPM, running = 0;
static int snd_mode = 0;        // 0 = Direct Sound break, 1 = PSG kit
static unsigned psg_step_idx = 0;
static unsigned psg_frame = 0;

static const char* const BREAK_NAMES[NBREAKS] = {
	"CLN086", "CLN105", "PRC095", "PRC098"
};
static const int8_t* const BRK[NBREAKS] = { brk0, brk1, brk2, brk3 };

static int brk_len_for(int i) {
	switch (i) {
		case 0: return brk0_len;
		case 1: return brk1_len;
		case 2: return brk2_len;
		default: return brk3_len;
	}
}

// ---- Direct Sound state ----------------------------------------------------
static int8_t abuf[ABUF_SIZE] __attribute__((aligned(4)));
static int cur_len = 0;         // current break length in samples (mult of 4)
static uint32_t pp = 0;         // next sample byte offset into abuf
static uint32_t frac = 0;       // sub-frame accumulator (1/60 units)

static void fifo_silence(void) {
	for (int i = 0; i < 4; i++) REG_FIFO_A = 0;
}

static void push_words(unsigned n) {
	while (n--) {
		const int8_t* s = abuf + pp;
		uint32_t w = (uint32_t)(uint8_t)s[0]
		  | ((uint32_t)(uint8_t)s[1] << 8)
		  | ((uint32_t)(uint8_t)s[2] << 16)
		  | ((uint32_t)(uint8_t)s[3] << 24);
		REG_FIFO_A = w;
		pp += 4;
		if (pp >= (uint32_t)cur_len) pp -= (uint32_t)cur_len;
	}
}

static void audio_start_break(void) {
	if (snd_mode == 1) {
		fifo_silence();
		return;
	}
	int len = brk_len_for(cur) & ~3;
	memcpy(abuf, BRK[cur], (size_t)len);
	cur_len = len;
	pp = 0;
	frac = 0;
	push_words(4);               // prefill
}

static void audio_stop(void) {
	fifo_silence();
}

// Push one 60 Hz frame worth of samples, paced in BATCHES sub-batches so
// the 32-byte FIFO stays balanced.  wv is the current scanline (0..159).
static void audio_frame_pump(unsigned wv) {
	uint32_t rate = ((uint32_t)BASE_RATE * (uint32_t)bpm) / BASE_BPM;
	uint32_t n = rate / 60;
	frac += rate % 60;
	if (frac >= 60) { frac -= 60; n++; }
	uint32_t words = n / 4;

	unsigned line = wv + 1;
	for (unsigned b = 0; b < BATCHES; b++) {
		unsigned tgt = (b + 1) * (160u / BATCHES);
		while (REG_VCOUNT < tgt) ;
		unsigned batch = (words + (BATCHES - 1 - b)) / (BATCHES - b);
		if (batch) { push_words(batch); words -= batch; }
		if (words == 0) break;
	}
	(void)line;
}

// ---- PSG kit fallback (channels 1 & 4) ------------------------------------
static void psg_kick(void) {
	REG_NR10 = 0x00;               // no sweep
	REG_NR11 = 0x80;               // 50% duty
	REG_NR12 = 0xF2;               // volume 15, decay
	REG_NR13 = 0x10;
	REG_NR14 = 0x80;               // trigger
}

static void psg_snare(void) {
	REG_NR41 = 0x00;
	REG_NR42 = 0xE2;
	REG_NR43 = 0x1B;
	REG_NR44 = 0x80;
}

static void psg_hat(int open) {
	REG_NR41 = 0x00;
	REG_NR42 = open ? 0xD3 : 0xE0;
	REG_NR43 = 0x5C;
	REG_NR44 = 0x80;
}

static void psg_step(unsigned step) {
	switch (step) {
		case 0: case 4: case 8: case 12:
			psg_kick(); psg_hat(0); break;
		case 2: case 6: case 10: case 14:
			psg_hat(1); break;
		case 3: case 7: case 11: case 15:
			psg_hat(0); break;
		case 5: case 13:
			psg_hat(1); break;
	}
	if (step == 4 || step == 12) psg_snare();
}

// ---- UI (Mode 3) ----------------------------------------------------------
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
	gfx_str(168, 3, snd_mode ? "PSG" : "DS", snd_mode ? COL_GRN : COL_TXT, COL_BG);

	gfx_str(4, 15, "BPM", COL_DIM, COL_BG);
	gfx_num(44, 15, bpm, COL_TXT, COL_BG);
	gfx_str(120, 15, running ? "PLAY" : "STOP",
			running ? COL_GRN : COL_RED, COL_BG);

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
				((i == 0 && running) || (i == 1 && !running)) ? 0x0000 : COL_TXT, cc);
	}

	gfx_str(4, 142, "L/R brk  U/D bpm  A play  SEL snd", COL_DIM, COL_BG);
}

void main(void) {
	REG_DISPCNT = 0x0403;
	REG_SOUNDBIAS = 0x0200;

	// Master enable first, then PSG / Direct Sound routing.
	REG_SOUNDCNT_X = 0x0080;
	REG_SOUNDCNT_L = 0xFF77;       // PSG ch1-4 both sides, max volume
	REG_SOUNDCNT_H = 0x0306;       // PSG 100%, DS A 100% to L+R, Timer 0

	// Timer 0 = sample clock.  No interrupts anywhere.
	REG_TM0CNT_L = 65536u - (16777216u / BASE_RATE);
	REG_TM0CNT_H = 0x0080;         // enable, 1-cycle prescale, no IRQ
	REG_TM1CNT_H = 0x0000;

	audio_start_break();
	running = 1;

	int prev_keys = 0;
	int need_draw = 1;

	for (;;) {
		// Wait for the start of active display (60 Hz frame boundary).
		while (REG_VCOUNT >= 160) ;
		unsigned wv = REG_VCOUNT;

		int keys = REG_KEYINPUT ^ 0x03FF;
		int down = keys & ~prev_keys;
		prev_keys = keys;

		if (down & KEY_RIGHT) {
			audio_stop();
			cur = (cur + 1) % NBREAKS;
			audio_start_break();
			need_draw = 1;
		}
		if (down & KEY_LEFT) {
			audio_stop();
			cur = (cur + NBREAKS - 1) % NBREAKS;
			audio_start_break();
			need_draw = 1;
		}
		if (down & KEY_UP) {
			if (bpm < 250) { bpm++; need_draw = 1; }
		}
		if (down & KEY_DOWN) {
			if (bpm > 61) { bpm--; need_draw = 1; }
		}
		if (down & (KEY_A | KEY_START)) {
			running = !running;
			if (running) audio_start_break();
			else audio_stop();
			need_draw = 1;
		}
		if (down & KEY_B) {
			audio_stop();
			cur = 0;
			audio_start_break();
			need_draw = 1;
		}
		if (down & KEY_SELECT) {
			snd_mode ^= 1;
			psg_step_idx = 0;
			psg_frame = 0;
			audio_start_break();
			need_draw = 1;
		}

		// PSG kit sequencing (only in PSG mode).
		if (running && snd_mode == 1) {
			unsigned eighth = 1800u / (unsigned)bpm;
			if (eighth < 1) eighth = 1;
			if (++psg_frame >= eighth) {
				psg_frame = 0;
				psg_step(psg_step_idx);
				psg_step_idx = (psg_step_idx + 1) & 15;
			}
		}

		// Stream the break (Direct Sound mode), paced across the active
		// display (VCOUNT ascends 0..159), then draw.
		if (running && snd_mode == 0) audio_frame_pump(wv);

		if (need_draw) {
			draw_ui();
			need_draw = 0;
		}

		// Remain until VBlank (consumes the rest of the active display).
		while (REG_VCOUNT < 160) ;
	}
}
