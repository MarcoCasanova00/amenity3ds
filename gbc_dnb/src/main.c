// DnB III-style Game Boy ROM: streams real 4-bit break samples through the
// wave channel (MBC1 banked ROM), with BPM control and clock out (link or
// headphone audio) — a compact clone of the HobbyChop approach.
#include <gb/gb.h>
#include <gbdk/console.h>
#include <stdio.h>

#include "sample_player.h"
#include "playbreaks.h"

#define SER_D (*(volatile UINT8*)0xFF01)
#define SER_C (*(volatile UINT8*)0xFF02)

#define NBREAKS 4
static const char* const BREAK_NAMES[NBREAKS] = {
	"CLN086", "CLN105", "PRC095", "PRC098"
};

// ---- 1 ms tick ------------------------------------------------------------
volatile UINT32 g_tick;
void tim_isr(void) __interrupt(1) { g_tick++; }

// ---- state -----------------------------------------------------------------
static UINT8 cur = 0;
static UINT16 bpm = 175;
static UINT8 running = 0;
static UINT8 dest = 0;        // 0 LINK (serial), 1 JACK (audio)
static UINT8 clk_mode = 0;    // 0 S24, 1 S16, 2 S8
static const UINT8 PPQN[3] = { 24, 16, 8 };
static const char* const MODE_N[3] = { "S24", "S16", "S8" };

static UINT8 out_state = 0;
static UINT32 ck_last = 0, ck_ph = 0;
static UINT16 pulse_count = 0;

static void start_break(void) {
	switch (cur) {
		case 0: play_break0(); break;
		case 1: play_break1(); break;
		case 2: play_break2(); break;
		default: play_break3(); break;
	}
}

static void put_str(UINT8 x, UINT8 y, const char* s) {
	gotoxy(x, y);
	while (*s) putchar(*s++);
}
static void print_u16(UINT8 x, UINT8 y, UINT16 v) {
	char b[6]; UINT8 n = 0;
	gotoxy(x, y);
	if (!v) { putchar('0'); return; }
	while (v && n < 5) { b[n++] = '0' + v % 10; v /= 10; }
	while (n) putchar(b[--n]);
}

static void draw_screen(void) {
	put_str(0, 0, "DNB III       ");
	put_str(0, 1, "BREAK ");
	put_str(6, 1, BREAK_NAMES[cur]);
	put_str(0, 2, "BPM  ");
	print_u16(5, 2, bpm);
	put_str(0, 3, running ? "RUN  " : "STOP ");
	put_str(0, 4, MODE_N[clk_mode]);
	put_str(6, 4, dest ? " JACK" : " LINK");
	put_str(0, 5, "L/R break U/D BPM A run");
	put_str(0, 6, "B mode SEL out L click");
}

static void send_edge(void) {
	out_state ^= 1;
	SER_D = out_state ? 0xFF : 0x00;
	SER_C = 0x81;
	while (SER_C & 0x80) /* wait */ ;
	SER_C = 0x00;
}

static void jack_pulse(UINT8 accent) {
	if (accent) {
		NR10_REG = 0x50;
		NR11_REG = 0x3F;
		NR12_REG = 0xF1;
		NR13_REG = 0x11;
		NR14_REG = 0xC0;
	} else {
		NR10_REG = 0x00;
		NR21_REG = 0x3F;
		NR22_REG = 0xF0;
		NR23_REG = 0x4A;
		NR24_REG = 0xC0;
	}
}

static void set_bpm_tma(void) {
	// Map BPM 61..250 -> TMA 240..144 (lower counter = faster/higher).
	UINT8 tma = 246 - (UINT8)(((bpm - 61) * 25u) / 100u);
	if (tma > 246) tma = 246;
	if (tma < 199) tma = 199;
	__critical { rTMA = tma; }
}

void main(void) {
	disable_interrupts();
	NR52_REG = 0x80;
	NR50_REG = 0x77;
	NR51_REG = 0xFF;
	rTMA = 240;
	rTAC = 0x04;
	set_interrupts(VBL_IFLAG | TIM_IFLAG);
	add_TIM(tim_isr);
	enable_interrupts();

	UINT8 prevkeys = 0xFF;
	draw_screen();

	for (;;) {
		UINT8 keys = joypad();
		UINT8 down = keys & ~prevkeys;
		prevkeys = keys;

		if (down & J_RIGHT) { cur = (cur + 1) % NBREAKS; if (running) start_break(); draw_screen(); }
		if (down & J_LEFT)  { cur = (cur + NBREAKS - 1) % NBREAKS; if (running) start_break(); draw_screen(); }
		if (down & J_UP)    { if (bpm < 250) { bpm++; set_bpm_tma(); } draw_screen(); }
		if (down & J_DOWN)  { if (bpm > 61)  { bpm--; set_bpm_tma(); } draw_screen(); }
		if (down & (J_A | J_START)) {
			running = !running;
			pulse_count = 0;
			ck_ph = 0;
			if (running) start_break();
			draw_screen();
		}
		if (down & J_B) { clk_mode = (clk_mode + 1) % 3; draw_screen(); }
		if (down & J_SELECT) { dest = !dest; draw_screen(); }
		if (down & J_LEFT) { /* metronome toggle (channel 1) via vol on/off */
			if (NR12_REG == 0x00) NR12_REG = 0xF1; else NR12_REG = 0x00;
		}

		// restart loop when sample ends
		if (running && play_length == 0) start_break();

		// clock pulses
		if (running) {
			UINT32 now = g_tick;
			UINT32 dt = now - ck_last;
			ck_last = now;
			ck_ph += dt;
			UINT32 period_ms = 60000UL / ((UINT32)bpm * PPQN[clk_mode]);
			if (period_ms == 0) period_ms = 1;
			while (ck_ph >= period_ms) {
				ck_ph -= period_ms;
				pulse_count++;
				if (dest) jack_pulse(pulse_count % 16 == 0);
				else send_edge();
			}
		} else {
			ck_last = g_tick;
			ck_ph = 0;
		}
	}
}
