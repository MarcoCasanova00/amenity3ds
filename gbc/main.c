// amenclk.gb — Game Boy / Game Boy Color clock-out + chiptune jungle ROM.
//
// Functions:
//   - Clock out: beat-locked pulses on the link port's serial-out wire
//     (HobbyChop-style), or on the headphone/audio output ("JACK" mode) as
//     an audio sync signal for Volca / Pocket Operator style gear.
//   - A small chiptune jungle kit (kick / snare / hats) sequenced to the same
//     BPM, so the Game Boy actually makes drum-and-bass-style audio.
//   - Metronome click in LINK mode (audible confirmation).
//
// Modes:  S24 = MIDI clock (24 PPQN)   S16 / S8 = nanoloop click rates.

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
	rTMA = 252;            // ~0.976 ms/tick
	rTAC = 0x04;
	set_interrupts(VBL_IFLAG | TIM_IFLAG);
	add_TIM(tim_isr);
	enable_interrupts();
}

// ---- APU ------------------------------------------------------------------
static void snd_init(void) {
	NR52_REG = 0x80;
	NR50_REG = 0x77;
	NR51_REG = 0xFF;
}

static void snd_all_off(void) {
	NR52_REG = 0x00;
	NR52_REG = 0x80;
}

// ---- serial clock edge (LINK mode) ---------------------------------------
static UINT8 out_state = 0;

static void send_edge(void) {
	out_state ^= 1;
	SER_D = out_state ? 0xFF : 0x00;
	SER_C = 0x81;
	while (SER_C & 0x80) /* wait */ ;
	SER_C = 0x00;
}

// ---- audio pulse (JACK mode) ---------------------------------------------
static void jack_pulse(UINT8 accent) {
	// square blip through the speaker/headphones
	if (accent) {
		NR10_REG = 0x50;          // sweep down for a thump
		NR11_REG = 0x3F;
		NR12_REG = 0xF1;
		NR13_REG = 0x11;          // ~96 Hz
		NR14_REG = 0xC0;
	} else {
		NR10_REG = 0x00;
		NR21_REG = 0x3F;
		NR22_REG = 0xF0;
		NR23_REG = 0x4A;          // ~1760 Hz
		NR24_REG = 0xC0;
	}
}

// ---- jungle kit (chiptune drums) -----------------------------------------
static void kit_kick(void) {
	NR10_REG = 0x60;              // sweep down for a pitched thump
	NR11_REG = 0x3F;
	NR12_REG = 0xF2;              // vol 15, fast decay
	NR13_REG = 0x10;              // ~110 Hz
	NR14_REG = 0xC0;
}

static void kit_snare(void) {
	NR41_REG = 0x3F;
	NR42_REG = 0xE2;              // vol 14, fast decay
	NR43_REG = 0x1B;              // noise, mid color
	NR44_REG = 0xC0;
}

static void kit_hat(UINT8 open) {
	NR41_REG = 0x1F;
	NR42_REG = open ? 0xD3 : 0xE0;
	NR43_REG = 0x5C;              // noise, bright
	NR44_REG = 0x80;
}

// step pattern (16 steps of 8ths): kick, snare, hats
static void kit_step(UINT8 step) {
	switch (step) {
		case 0: case 4: case 8: case 12:
			kit_kick();
			kit_hat(0);
			break;
		case 2: case 6: case 10: case 14:
			kit_hat(1);
			break;
		case 3: case 7: case 11: case 15:
			kit_hat(0);
			break;
		case 5: case 13:
			kit_hat(1);
			break;
	}
	if (step == 4 || step == 12) kit_snare();
}

// ---- modes -----------------------------------------------------------------
#define MODE_COUNT 3
static const UINT8 mode_ppqn[MODE_COUNT] = { 24, 16, 8 };
static const char* const mode_names[MODE_COUNT] = { "S24", "S16", "S8" };

static UINT8 clk_mode = 0;
static UINT16 bpm = 175;
static UINT8 running = 0;
static UINT8 muted = 0;
static UINT8 out_dest = 0;      // 0 = LINK (serial), 1 = JACK (audio)
static UINT8 kit_on = 0;

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
	put_str(0, 4, out_dest ? "JACK " : "LINK ");
	put_str(0, 5, muted ? "SND OFF" : "SND ON ");
	put_str(0, 6, "L/R mode U/D BPM A run");
	put_str(0, 7, "B snd SEL out L kit");
}

void main(void) {
	disable_interrupts();
	timer_setup();
	snd_init();

	UINT32 ck_last = 0;
	UINT32 ck_ph = 0;
	UINT32 kit_last = 0;
	UINT32 kit_ph = 0;
	UINT8 prevkeys = 0xFF;
	UINT16 pulse_count = 0;
	UINT8 kit_stepidx = 0;

	draw_screen();

	for (;;) {
		UINT8 keys = joypad();
		UINT8 down = keys & ~prevkeys;
		prevkeys = keys;

		if (down & J_RIGHT) { clk_mode = (clk_mode + 1) % MODE_COUNT; draw_screen(); }
		if (down & J_LEFT)  { clk_mode = (clk_mode + MODE_COUNT - 1) % MODE_COUNT; draw_screen(); }
		if (down & J_UP)    { if (bpm < 250) bpm++; draw_screen(); }
		if (down & J_DOWN)  { if (bpm > 61) bpm--; draw_screen(); }
		if (down & (J_A | J_START)) { running = !running; pulse_count = 0; kit_stepidx = 0; draw_screen(); }
		if (down & J_B) { muted = !muted; draw_screen(); }
		if (down & J_SELECT) { out_dest = !out_dest; draw_screen(); }
		if (down & J_LEFT) { kit_on = !kit_on; draw_screen(); }

		if (running) {
			UINT32 now = g_tick;
			UINT32 dt = now - ck_last;
			ck_last = now;

			// clock pulses
			ck_ph += dt;
			UINT32 period_ms = 60000UL / ((UINT32)bpm * mode_ppqn[clk_mode]);
			if (period_ms == 0) period_ms = 1;
			while (ck_ph >= period_ms) {
				ck_ph -= period_ms;
				pulse_count++;
				if (out_dest == 1) {
					jack_pulse(pulse_count % 16 == 0);
				} else {
					send_edge();
					if (!muted) {
						if (pulse_count % 16 == 0) {
							NR10_REG = 0x50;
							NR11_REG = 0x3F;
							NR12_REG = 0xF1;
							NR13_REG = 0x11;
							NR14_REG = 0xC0;
						} else {
							NR21_REG = 0x3F;
							NR22_REG = 0xF1;
							NR23_REG = 0x4A;
							NR24_REG = 0xC0;
						}
					}
				}
			}

			// jungle kit (8th-note steps)
			if (kit_on) {
				kit_ph += dt;
				UINT32 eighth_ms = 60000UL / ((UINT32)bpm * 2);
				while (kit_ph >= eighth_ms) {
					kit_ph -= eighth_ms;
					if (!muted) kit_step(kit_stepidx);
					kit_stepidx = (kit_stepidx + 1) & 0x0F;
				}
			}
		} else {
			ck_last = g_tick;
			kit_last = g_tick;
			ck_ph = 0;
			kit_ph = 0;
		}

		// if muted, silence everything continuously
		if (muted) snd_all_off();
	}
}
