// GBA DnB break player — 16-bit Direct Sound (fixed regs!) + PSG kit fallback.
// UI: Mode 3, redrawn only on change (no flicker).
#include <stdint.h>

#include "regs.h"
#include "gfx.h"
#include "samples.h"

#define NBREAKS 4
#define SR_WORDS 11025          // words/sec (each word = 2 samples -> 22.05 kHz)

#define SDS_H 0x0306            // DMG 100% + DS A 100% + A left/right

static int cur = 0, bpm = 175, running = 0;
static int snd_mode = 0;        // 0 = Direct Sound break, 1 = PSG kit
static unsigned psg_step_idx = 0;

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

// ---- Direct Sound state (read by IRQ) -------------------------------------
static const int16_t* a_src = 0;
static int a_len = 0;
static volatile int a_pos = 0;
static volatile int a_active = 0;

void audio_tick(void) {
	if (!a_active) return;
	const int16_t* s = a_src;
	int p = a_pos;
	int16_t x = s[p];
	int16_t y = (p + 1 < a_len) ? s[p + 1] : 0;
	uint32_t w = ((uint32_t)(uint16_t)x) | ((uint32_t)(uint16_t)y << 16);
	REG_FIFO_A = w;
	REG_FIFO_B = w;
	p += 2;
	if (p >= a_len) p = 0;
	a_pos = p;
}

void irq_handler(void) __attribute__((naked));
void irq_handler(void) {
	__asm volatile(
		"push {r0-r3,lr}\n\t"
		"ldr r0, =0x04000210\n\t"
		"ldr r1, [r0]\n\t"
		"tst r1, #1\n\t"
		"beq 1f\n\t"
		"bl audio_tick\n\t"
		"mov r1, #1\n\t"
		"str r1, [r0]\n\t"
		"1:\n\t"
		"pop {r0-r3,lr}\n\t"
		"movs pc, lr\n\t"
	);
}

static void audio_start_break(void) {
	a_src = BRK[cur];
	a_len = brk_len_for(cur);
	if (a_len > 22050 * 5) a_len = 22050 * 5;
	a_pos = 0;
	a_active = (snd_mode == 0) ? 1 : 0;
}

static void audio_stop(void) {
	a_active = 0;
}

static void set_timer(void) {
	uint32_t period = (uint32_t)(16777216u / SR_WORDS) * 175u / (uint32_t)bpm;
	if (period < 2) period = 2;
	REG_TM0CNT_L = (uint16_t)(period - 1);
	REG_TM0CNT_H = 0x00C0;
}

// ---- PSG kit fallback (channels 1 & 4) ------------------------------------
static void psg_kick(void) {
	REG_NR10 = 0x0060;
	REG_NR11 = 0x0080;
	REG_NR12 = 0x00F2;
	REG_NR13 = 0x0010;
	REG_NR14 = 0x00C0;
}

static void psg_snare(void) {
	REG_NR41 = 0x003F;
	REG_NR42 = 0x00E2;
	REG_NR43 = 0x001B;
	REG_NR44 = 0x00C0;
}

static void psg_hat(int open) {
	REG_NR41 = 0x001F;
	REG_NR42 = open ? 0x00D3 : 0x00E0;
	REG_NR43 = 0x005C;
	REG_NR44 = 0x0080;
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
	REG_SOUNDCNT_X = 0x0080;
	REG_SOUNDCNT_H = SDS_H;

	*(void**)0x03007FFC = (void*)irq_handler;
	REG_IE = IE_TIMER0;
	REG_IME = 0x8000;

	audio_start_break();
	running = 1;
	set_timer();

	int prevKeys = 0x03FF;
	int need_draw = 1;
	unsigned psg_frame = 0;
	unsigned frame_count = 0;
	int beep_on = 1;             // siren until a key is pressed

	for (;;) {
		int keys = REG_KEYINPUT ^ 0x03FF;
		int down = keys & ~prevKeys;
		prevKeys = keys;

		// siren test: alternating two-tone, retriggered every 32 frames
		if (beep_on) {
			if ((frame_count & 31) == 0) {
				REG_NR21 = 0x0080;
				REG_NR22 = 0x00F0;
				REG_NR23 = (frame_count & 64) ? 0x004A : 0x003A;
				REG_NR24 = 0x0080;
			}
			if (keys) beep_on = 0;
		}
		frame_count++;

		if (down & KEY_RIGHT) { audio_stop(); cur = (cur + 1) % NBREAKS; if (running) audio_start_break(); need_draw = 1; }
		if (down & KEY_LEFT)  { audio_stop(); cur = (cur + NBREAKS - 1) % NBREAKS; if (running) audio_start_break(); need_draw = 1; }
		if (down & KEY_UP)    { if (bpm < 250) { bpm++; set_timer(); need_draw = 1; } }
		if (down & KEY_DOWN)  { if (bpm > 61) { bpm--; set_timer(); need_draw = 1; } }
		if (down & (KEY_A | KEY_START)) {
			if (running) { running = 0; audio_stop(); }
			else { running = 1; audio_start_break(); set_timer(); }
			need_draw = 1;
		}
		if (down & KEY_B) { audio_stop(); cur = 0; if (running) audio_start_break(); need_draw = 1; }
		if (down & KEY_SELECT) {
			snd_mode ^= 1;
			audio_stop();
			if (running) audio_start_break();
			need_draw = 1;
		}

		// PSG kit sequencing (only in PSG mode)
		if (running && snd_mode == 1) {
			unsigned eighth = 1800u / (unsigned)bpm;   // frames per 8th @60fps
			if (eighth < 1) eighth = 1;
			if (psg_frame >= eighth) {
				psg_frame = 0;
				psg_step(psg_step_idx);
				psg_step_idx = (psg_step_idx + 1) & 15;
			}
			psg_frame++;
		}

		if (need_draw) {
			while (REG_VCOUNT >= 160) ;
			while (REG_VCOUNT < 160) ;
			draw_ui();
			need_draw = 0;
		}
	}
}
