// GBA DnB break player — 8-bit Direct Sound FIFO + PSG kit fallback.
//
// Audio path (this is the part that used to be silent):
//   * Direct Sound FIFOs are 8-bit, one byte per Timer-0 overflow.
//   * Timer 0 therefore runs at the sample rate (22.05 kHz at 175 BPM).
//   * Timer 1 runs at exactly one quarter of that rate and its IRQ pushes
//     four 8-bit samples (one 32-bit FIFO word) per tick.
//   * GBA IME is bit 0 (not 0x8000), and timer reloads are 65536-period.
//
// UI: Mode 3, redrawn only on change (no flicker).
#include <stdint.h>

#include "regs.h"
#include "gfx.h"
#include "samples.h"

#define NBREAKS 4
#define BASE_BPM 175
#define BASE_RATE 22050

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

// ---- Direct Sound state (read by the Timer-1 IRQ) -------------------------
static const int8_t* volatile a_src;
static volatile int a_len;
static volatile int a_pos;
static volatile int a_active;



void audio_tick(void) {
	uint32_t w = 0;
	if (a_active) {
		const int8_t* s = (const int8_t*)a_src;
		int p = a_pos;
		if (p + 4 > a_len) p = 0;
		w = (uint32_t)(uint8_t)s[p]
		  | ((uint32_t)(uint8_t)s[p + 1] << 8)
		  | ((uint32_t)(uint8_t)s[p + 2] << 16)
		  | ((uint32_t)(uint8_t)s[p + 3] << 24);
		p += 4;
		if (p >= a_len) p = 0;
		a_pos = p;
	}
	REG_FIFO_A = w;
}

void irq_handler(void) __attribute__((naked));
void irq_handler(void) {
	__asm volatile(
		"push {r0-r3,r12,lr}\n\t"
		"ldr r0, =0x04000202\n\t"      // REG_IF
		"ldr r1, [r0]\n\t"
		"tst r1, #0x10\n\t"            // Timer 1?
		"beq 1f\n\t"
		"mov r1, #0x10\n\t"
		"str r1, [r0]\n\t"             // write-1-to-clear
		"bl audio_tick\n\t"
		"1:\n\t"
		"pop {r0-r3,r12,lr}\n\t"
		"bx lr\n\t"
	);
}

static void fifo_prefill(void) {
	for (int i = 0; i < 4; i++) audio_tick();
}

static void fifo_silence(void) {
	for (int i = 0; i < 4; i++) REG_FIFO_A = 0;
}

static void audio_start_break(void) {
	a_src = BRK[cur];
	a_len = brk_len_for(cur) & ~3;
	a_pos = 0;
	a_active = (snd_mode == 0);
	if (a_active) fifo_prefill();
	else fifo_silence();
}

static void audio_stop(void) {
	a_active = 0;
}

static void timers_stop(void) {
	REG_TM0CNT_H = 0x0000;
	REG_TM1CNT_H = 0x0000;
}

// Timer 0 = sample clock, Timer 1 = 1/4 sample clock (4 samples per IRQ).
static void timers_start(void) {
	uint32_t rate = ((uint32_t)BASE_RATE * (uint32_t)bpm) / BASE_BPM;
	uint32_t p0 = 16777216u / rate;
	uint32_t p1;
	if (p0 < 4) p0 = 4;
	p1 = p0 * 4;
	if (p1 > 65535) p1 = 65535;

	timers_stop();
	REG_TM0CNT_L = (uint16_t)(65536u - p0);
	REG_TM1CNT_L = (uint16_t)(65536u - p1);
	REG_TM0CNT_H = 0x0080;         // enable, 1-cycle prescale, no IRQ
	REG_TM1CNT_H = 0x00C0;         // enable, 1-cycle prescale, Timer-1 IRQ
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

	REG_IF = 0xFFFF;
	*(volatile void**)0x03007FFC = (void*)irq_handler;
	REG_IE = IE_TIMER1;
	REG_IME = 0x0001;

	audio_start_break();
	running = 1;
	timers_start();

	int prev_keys = 0;
	int need_draw = 1;

	for (;;) {
		// Run the UI/sequencer at 60 Hz.
		while (REG_VCOUNT >= 160) ;
		while (REG_VCOUNT < 160) ;

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
			if (bpm < 250) { bpm++; timers_start(); need_draw = 1; }
		}
		if (down & KEY_DOWN) {
			if (bpm > 61) { bpm--; timers_start(); need_draw = 1; }
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

		if (need_draw) {
			draw_ui();
			need_draw = 0;
		}
	}
}
