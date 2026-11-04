// ui.c
#include "ui.h"
#include "gfx.h"
#include <3ds.h>
#include <stdio.h>
#include <math.h>

#define COL_BG    gfx_rgb(26, 24, 22)
#define COL_PANEL gfx_rgb(40, 38, 36)
#define COL_ACC   gfx_rgb(255, 168, 40)   // amenity amber
#define COL_ON    gfx_rgb(80, 220, 120)
#define COL_OFF   gfx_rgb(120, 116, 110)
#define COL_RED   gfx_rgb(235, 80, 60)
#define COL_TXT   gfx_rgb(235, 232, 224)
#define COL_DIM   gfx_rgb(140, 136, 128)

static const char* clk_label(int m) {
	switch (m) {
		case ENG_CLK_MIDI24: return "CLK24";
		case ENG_CLK_S16: return "S16";
		case ENG_CLK_S8: return "S8";
		default: return "CLK OFF";
	}
}

static void draw_btn(u16* fb, int stride, int x, int y, int w, int h,
					 const char* label, int active) {
	gfx_fill(fb, stride, x, y, w, h, active ? COL_ON : COL_PANEL);
	gfx_rect(fb, stride, x, y, w, h, active ? COL_ON : COL_OFF);
	int tw = gfx_strw(label);
	gfx_str(fb, stride, x + (w - tw) / 2, y + (h - 7) / 2, label,
			 active ? gfx_rgb(10, 10, 8) : COL_TXT, active ? COL_ON : COL_PANEL);
}

void app_init(App* a, int sr) {
	a->eng = eng_create(sr);
	a->ctrl.bpm = 175;
	a->ctrl.playing = 1;
	a->ctrl.vol = 100;
	a->ctrl.bank = 0;
	a->ctrl.variation = 45;
	a->ctrl.freeze = 0;
	a->sel = 0;
	a->guide = 0;
	a->patSlot = 0;
	eng_play(a->eng, 1);
	eng_set_ctrl(a->eng, &a->ctrl);
}

void app_input(App* a) {
	u32 kDown = hidKeysDown();
	u32 kHeld = hidKeysHeld();
	u32 kUp = hidKeysUp();
	(void)kUp;

	int wishGuide = (kDown & KEY_L) && (kDown & KEY_R);

	// guide is modal
	if (a->guide) {
		if (wishGuide) a->guide = 0;
		return;
	}
	if (wishGuide) { a->guide = 1; return; }

	int selHold = (kHeld & KEY_SELECT) != 0;

	// SELECT combos (pattern memory)
	if (selHold) {
		if (kDown & KEY_DUP) { a->patSlot = (a->patSlot + 1) % ENG_PAT_SLOTS; }
		else if (kDown & KEY_DDOWN) { a->patSlot = (a->patSlot + ENG_PAT_SLOTS - 1) % ENG_PAT_SLOTS; }
		else if (kDown & KEY_A) { eng_pat_save(a->eng, a->patSlot); }
		else if (kDown & KEY_B) { eng_pat_load(a->eng, a->patSlot); a->ctrl.freeze = 1; }
	}

	// touch
	touchPosition tp;
	hidTouchRead(&tp);
	if (kDown & KEY_TOUCH) {
		int x = tp.px, y = tp.py;
		if (y >= 2 && y < 30) {
			if (x < 54) { a->ctrl.playing = !a->ctrl.playing; eng_play(a->eng, a->ctrl.playing); }
			else if (x < 108) { a->ctrl.bpm -= 1; if (a->ctrl.bpm < 61) a->ctrl.bpm = 61; }
			else if (x < 162) { a->ctrl.bpm += 1; if (a->ctrl.bpm > 250) a->ctrl.bpm = 250; }
			else if (x < 216) { a->ctrl.clk_mode = (a->ctrl.clk_mode + 1) % 4; }
			else {
				a->ctrl.bank = (a->ctrl.bank + 1) % 8;
				eng_set_ctrl(a->eng, &a->ctrl);
				a->ctrl.bpm = eng_bank_bpm(a->eng);
			}
		}
		else if (y >= 30 && y < 58) {
			int i = (x - 2) / 46;
			switch (i) {
				case 0: a->ctrl.roll = a->ctrl.roll ? 0 : 4; break;
				case 1: a->ctrl.build = !a->ctrl.build; break;
				case 2: a->ctrl.echo = !a->ctrl.echo; break;
				case 3: a->ctrl.stretch = !a->ctrl.stretch; break;
				case 4: a->ctrl.filt = (a->ctrl.filt + 1) % 3; break;
				case 5: a->ctrl.vol = a->ctrl.vol > 0 ? a->ctrl.vol - 5 : 0; break;
				case 6: a->ctrl.vol = a->ctrl.vol < 100 ? a->ctrl.vol + 5 : 100; break;
			}
		}
		else if (y >= 58) {
			int col = (x - 2) / 79;
			int row = (y - 58) / 45;
			if (col >= 0 && col < 4 && row >= 0 && row < 4) {
				int idx = row * 4 + col;
				if (idx >= 0 && idx < 16) {
					a->sel = idx;
					a->padFlash[idx] = 8;
					eng_trigger_slice(a->eng, idx);
				}
			}
		}
	}

	// physical controls (D-pad/A/B skipped while SELECT is held)
	if (!selHold) {
		if (kDown & KEY_DUP) { a->sel = (a->sel - 4 + 16) % 16; }
		if (kDown & KEY_DDOWN) { a->sel = (a->sel + 4) % 16; }
		if (kDown & KEY_DLEFT) { int c = a->sel % 4; a->sel = a->sel - c + (c + 3) % 4; }
		if (kDown & KEY_DRIGHT) { int c = a->sel % 4; a->sel = a->sel - c + (c + 1) % 4; }
		if (kDown & KEY_A) { eng_trigger_slice(a->eng, a->sel); }
		if (kDown & KEY_B) { a->ctrl.playing = !a->ctrl.playing; eng_play(a->eng, a->ctrl.playing); }
	}
	if (kDown & KEY_X) { a->ctrl.roll = (a->ctrl.roll + 2) % 10; if (a->ctrl.roll > 8) a->ctrl.roll = 0; }
	if (kDown & KEY_Y) { a->ctrl.echo = !a->ctrl.echo; }
	if (kDown & KEY_L) { a->ctrl.filt = (a->ctrl.filt + 1) % 3; }
	if (kDown & KEY_R) { a->ctrl.clk_mode = (a->ctrl.clk_mode + 1) % 4; }
	if (kDown & KEY_SELECT) {
		if (kDown & KEY_DLEFT) { a->ctrl.variation -= 5; }
		if (kDown & KEY_DRIGHT) { a->ctrl.variation += 5; }
	}
	if (kDown & KEY_START) { a->ctrl.freeze = !a->ctrl.freeze; }

	// clamp + push to engine
	if (a->ctrl.bpm < 61) a->ctrl.bpm = 61;
	if (a->ctrl.bpm > 250) a->ctrl.bpm = 250;
	if (a->ctrl.vol < 0) a->ctrl.vol = 0;
	if (a->ctrl.vol > 100) a->ctrl.vol = 100;
	if (a->ctrl.variation < 0) a->ctrl.variation = 0;
	if (a->ctrl.variation > 100) a->ctrl.variation = 100;
	if (a->ctrl.bank < 0) a->ctrl.bank = 0;
	if (a->ctrl.bank > 7) a->ctrl.bank = 7;
	eng_set_ctrl(a->eng, &a->ctrl);

	// decay flashes
	for (int i = 0; i < 16; i++) if (a->padFlash[i] > 0) a->padFlash[i]--;
}

void app_draw_bottom(App* a, u16* fb, int stride) {
	if (a->guide) {
		gfx_clear(fb, stride, 320, 240, COL_BG);
		static const char* L[] = {
			"== AMENITY3DS GUIDE ==",
			"PADS           fire a slice",
			"DPAD + A       select / fire pad",
			"PLAY / STOP    transport",
			"BPM- / BPM+    tempo",
			"CLK            24 / S16 / S8 / OFF",
			"BRK            next break (auto BPM)",
			"ROLL BUILD ECHO STRTCH FILT",
			"X roll   Y echo   L filter",
			"SELECT + LEFT/RIGHT  variation -/+",
			"SELECT + UP/DOWN     pattern slot -/+",
			"SELECT + A save / SELECT + B load",
			"(slots last this session)",
			"START           freeze phrase",
			"VOL- / VOL+     master volume",
			"L+R close guide",
		};
		int y = 4;
		for (int i = 0; i < (int)(sizeof L / sizeof L[0]); i++) {
			gfx_str(fb, stride, 6, y, L[i], i == 0 ? COL_ACC : COL_TXT, COL_BG);
			y += 15;
		}
		return;
	}

	gfx_clear(fb, stride, 320, 240, COL_BG);

	// header
	draw_btn(fb, stride, 2, 2, 52, 26, a->ctrl.playing ? "STOP" : "PLAY", a->ctrl.playing);
	draw_btn(fb, stride, 56, 2, 52, 26, "BPM-", 0);
	draw_btn(fb, stride, 110, 2, 52, 26, "BPM+", 0);
	draw_btn(fb, stride, 164, 2, 52, 26, clk_label(a->ctrl.clk_mode), a->ctrl.clk_mode != ENG_CLK_OFF);
	{
		char blab[12];
		snprintf(blab, sizeof blab, "BRK %d", a->ctrl.bank + 1);
		draw_btn(fb, stride, 218, 2, 100, 26, blab, a->ctrl.bank != 0);
	}

	// effect row
	draw_btn(fb, stride, 2, 30, 44, 26, "ROLL", a->ctrl.roll != 0);
	draw_btn(fb, stride, 48, 30, 44, 26, "BUILD", a->ctrl.build);
	draw_btn(fb, stride, 94, 30, 44, 26, "ECHO", a->ctrl.echo);
	draw_btn(fb, stride, 140, 30, 44, 26, "STRTCH", a->ctrl.stretch);
	draw_btn(fb, stride, 186, 30, 44, 26, a->ctrl.filt == ENG_FILT_LP ? "LP"
			: a->ctrl.filt == ENG_FILT_HP ? "HP" : "FILT", a->ctrl.filt != ENG_FILT_OFF);
	draw_btn(fb, stride, 232, 30, 42, 26, "VOL-", 0);
	draw_btn(fb, stride, 276, 30, 42, 26, "VOL+", 0);

	// pads
	for (int i = 0; i < 16; i++) {
		int x = 2 + (i % 4) * 79;
		int y = 58 + (i / 4) * 45;
		int flash = a->padFlash[i];
		u16 col = flash ? COL_ACC : (i == a->sel ? COL_ON : COL_PANEL);
		gfx_fill(fb, stride, x, y, 77, 43, col);
		gfx_rect(fb, stride, x, y, 77, 43, i == a->sel ? COL_TXT : COL_OFF);
		char buf[4];
		buf[0] = (char)('0' + i / 10);
		buf[1] = (char)('0' + i % 10);
		buf[2] = 0;
		int tw = gfx_strw(buf);
		gfx_str(fb, stride, x + (77 - tw) / 2, y + (43 - 7) / 2, buf,
				 i == a->sel ? gfx_rgb(10, 10, 8) : COL_TXT, col);
	}

	// volume bar
	gfx_str(fb, stride, 4, 236 - 12, "VOL", COL_DIM, COL_BG);
	gfx_bar(fb, stride, 34, 236 - 10, 120, 6, a->ctrl.vol / 100.0f, COL_ACC);
	char vb[8];
	int vn = 0;
	if (a->ctrl.vol == 0) vb[vn++] = '0';
	int v = a->ctrl.vol;
	while (v > 0 && vn < 3) { vb[vn++] = (char)('0' + v % 10); v /= 10; }
	for (int i = vn - 1; i >= 0; i--) { gfx_char(fb, stride, 158 + (vn - 1 - i) * 6, 236 - 12, vb[i], COL_TXT, COL_BG); }
}

void app_draw_top(App* a, u16* fb, int stride) {
	gfx_clear(fb, stride, 400, 240, COL_BG);

	gfx_str(fb, stride, 10, 8, "AMENITY3DS", COL_ACC, COL_BG);
	gfx_str(fb, stride, 300, 8, clk_label(a->ctrl.clk_mode), COL_DIM, COL_BG);

	{
		const char* bn = eng_bank_name(a->eng);
		gfx_str(fb, stride, 140, 28, "BRK ", COL_DIM, COL_BG);
		gfx_str(fb, stride, 176, 28, bn, COL_TXT, COL_BG);
	}

	gfx_str(fb, stride, 10, 30, "BPM", COL_DIM, COL_BG);
	char bpm[8];
	int n = 0;
	int v = a->ctrl.bpm;
	while (v > 0 && n < 3) { bpm[n++] = (char)('0' + v % 10); v /= 10; }
	if (n == 0) bpm[n++] = '0';
	for (int i = n - 1; i >= 0; i--) {
		gfx_char(fb, stride, 60 + (n - 1 - i) * 8, 28, bpm[i], COL_TXT, COL_BG);
	}

	// step indicator
	int step = eng_current_step(a->eng);
	for (int i = 0; i < 16; i++) {
		int x = 10 + i * 12;
		int y = 70;
		u16 c = (i == step) ? COL_ACC : (i < step ? COL_DIM : COL_OFF);
		gfx_fill(fb, stride, x, y, 9, 22, c);
	}

	gfx_str(fb, stride, 10, 100, "STEP", COL_DIM, COL_BG);
	{
		char s[4];
		s[0] = (char)('0' + step / 10);
		s[1] = (char)('0' + step % 10);
		s[2] = 0;
		gfx_str(fb, stride, 50, 100, s, COL_TXT, COL_BG);
	}
	{
		char v[12];
		snprintf(v, sizeof v, "VAR %d", a->ctrl.variation);
		gfx_str(fb, stride, 120, 100, v, COL_ACC, COL_BG);
		gfx_str(fb, stride, 210, 100, a->ctrl.freeze ? "FRZ" : "-",
				 a->ctrl.freeze ? COL_ACC : COL_DIM, COL_BG);
	}
	{
		char p[12];
		snprintf(p, sizeof p, "PAT%d%s", a->patSlot, eng_pat_has(a->eng, a->patSlot) ? "*" : "");
		gfx_str(fb, stride, 270, 100, p, COL_DIM, COL_BG);
	}

	// effect status
	gfx_str(fb, stride, 10, 130, "FX", COL_DIM, COL_BG);
	gfx_str(fb, stride, 40, 130, a->ctrl.roll ? "ROLL" : "-", a->ctrl.roll ? COL_ACC : COL_DIM, COL_BG);
	gfx_str(fb, stride, 90, 130, a->ctrl.build ? "BUILD" : "-", a->ctrl.build ? COL_ACC : COL_DIM, COL_BG);
	gfx_str(fb, stride, 150, 130, a->ctrl.echo ? "ECHO" : "-", a->ctrl.echo ? COL_ACC : COL_DIM, COL_BG);
	gfx_str(fb, stride, 210, 130, a->ctrl.stretch ? "STRTCH" : "-", a->ctrl.stretch ? COL_ACC : COL_DIM, COL_BG);
	gfx_str(fb, stride, 280, 130, a->ctrl.filt == ENG_FILT_LP ? "LP" : a->ctrl.filt == ENG_FILT_HP ? "HP" : "-", a->ctrl.filt ? COL_ACC : COL_DIM, COL_BG);

	// voices + transport
	gfx_str(fb, stride, 10, 160, "VOICES", COL_DIM, COL_BG);
	{
		char s[8];
		int vn = 0;
		int vc = eng_voices(a->eng);
		do { s[vn++] = (char)('0' + vc % 10); vc /= 10; } while (vc > 0 && vn < 4);
		for (int i = vn - 1; i >= 0; i--) gfx_char(fb, stride, 70 + (vn - 1 - i) * 6, 160, s[i], COL_TXT, COL_BG);
	}
	gfx_str(fb, stride, 10, 190, a->ctrl.playing ? "PLAYING" : "STOPPED", a->ctrl.playing ? COL_ON : COL_RED, COL_BG);
}
