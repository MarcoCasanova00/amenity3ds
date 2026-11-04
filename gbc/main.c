// amenclk.gb — Game Boy / Game Boy Color clock-out ROM.
//
// A tiny standalone utility that turns the Game Boy into a clock master:
// it emits beat-locked pulses on the link port's serial-out (SO) wire,
// exactly the signal HobbyChop's Drum & Bass series puts on the link cable:
//   - S24 = MIDI clock rate (24 PPQN) -> pair with an ArduinoBoy to get
//     real 5-pin MIDI clock/start/stop.
//   - S16 / S8 = nanoloop-style click rates -> drive a Volca / Pocket
//     Operator directly through the serial-out wire (tip) + ground (sleeve).
//
// Pulses are produced by shifting bytes out the serial port (internal clock);
// each edge is one short byte transfer, so the line toggles cleanly.

#include <gb/gb.h>
#include <gbdk/console.h>
#include <stdio.h>

#define SER_D (*(volatile UINT8*)0xFF01)   // serial data (SB)
#define SER_C (*(volatile UINT8*)0xFF02)   // serial control (SC)

// ---- 1 ms tick from the timer --------------------------------------------
volatile UINT32 g_tick;

void tim_isr(void) __interrupt(1) {
	g_tick++;
}

static void timer_setup(void) {
	g_tick = 0;
	rTMA = 252;            // count 4 at 4096 Hz => ~0.976 ms/tick
	rTAC = 0x04;           // timer on, 4096 Hz
	set_interrupts(VBL_IFLAG | TIM_IFLAG);
	add_TIM(tim_isr);
	enable_interrupts();
}

// ---- one edge on SO -------------------------------------------------------
static UINT8 out_state = 0;

static void send_edge(void) {
	out_state ^= 1;
	SER_D = out_state ? 0xFF : 0x00;
	SER_C = 0x81;                       // internal clock, 16384 bps
	while (SER_C & 0x80) /* wait */ ;
	SER_C = 0x00;
}

// ---- modes -----------------------------------------------------------------
#define MODE_COUNT 3
static const UINT8 mode_ppqn[MODE_COUNT] = { 24, 16, 8 };
static const char* const mode_names[MODE_COUNT] = { "S24", "S16", "S8" };

static UINT8 clk_mode = 0;
static UINT16 bpm = 175;
static UINT8 running = 0;

static void put_str(UINT8 x, UINT8 y, const char* s) {
	gotoxy(x, y);
	while (*s) putchar(*s++);
}

static void print_u16(UINT8 x, UINT8 y, UINT16 v) {
	char b[6];
	UINT8 n = 0;
	gotoxy(x, y);
	if (v == 0) { putchar('0'); return; }
	while (v && n < 5) { b[n++] = (char)('0' + v % 10); v /= 10; }
	while (n) putchar(b[--n]);
}

static void draw_screen(void) {
	put_str(0, 0, "AMEN CLK      ");
	put_str(0, 1, "MODE ");
	put_str(6, 1, mode_names[clk_mode]);
	put_str(0, 2, "BPM  ");
	print_u16(5, 2, bpm);
	put_str(0, 3, running ? "RUN  " : "STOP ");
	put_str(0, 4, "L/R mode U/D BPM A run");
}

void main(void) {
	disable_interrupts();
	timer_setup();

	UINT32 last = 0;
	UINT32 ph_ms = 0;
	UINT8 prevkeys = 0xFF;

	draw_screen();

	for (;;) {
		UINT8 keys = joypad();
		UINT8 down = keys & ~prevkeys;
		prevkeys = keys;

		if (down & J_RIGHT) { clk_mode = (clk_mode + 1) % MODE_COUNT; draw_screen(); }
		if (down & J_LEFT)  { clk_mode = (clk_mode + MODE_COUNT - 1) % MODE_COUNT; draw_screen(); }
		if (down & J_UP)    { if (bpm < 250) bpm++; draw_screen(); }
		if (down & J_DOWN)  { if (bpm > 61) bpm--; draw_screen(); }
		if (down & (J_A | J_START)) { running = !running; draw_screen(); }

		if (running) {
			UINT32 now = g_tick;
			UINT32 dt = now - last;
			last = now;
			ph_ms += dt;
			UINT32 period_ms = 60000UL / ((UINT32)bpm * mode_ppqn[clk_mode]);
			if (period_ms == 0) period_ms = 1;
			while (ph_ms >= period_ms) {
				ph_ms -= period_ms;
				send_edge();
			}
		} else {
			last = g_tick;
			ph_ms = 0;
		}
	}
}
