// engine.c — Amenity-style live break slicer for the 3DS.
//
// - Breaks: a selectable bank of royalty-free breaks (44.1 kHz mono),
//   each sliced into 16 eighth-note pieces.
// - Pitch-locked time-stretch: granular OLA (overlap-add of crossfaded
//   grains), so changing tempo changes speed while keeping the break's pitch.
// - Phrase variation: on each new phrase the pattern can be re-sequenced
//   (slice swaps) and finished with a fill (roll/build/stretch tool) — the
//   amount is set by `variation`, like Amenity's VARIATION knob + SEQ fills.
// - Clock out: beat-locked pulse train (audio) at MIDI 24 PPQN or S16/S8.
#include <string.h>
#include <math.h>
#include <stdlib.h>

#include "engine.h"
#include "break_bank.h"

#define MAXVOICES 24
#define MAXTRIGS 64

#define GRAIN (2400)    // 50 ms grains at 48 kHz
#define CROSS (480)     // 10 ms crossfade

enum {
	FX_NONE = 0,
	FX_ROLL2,
	FX_ROLL4,
	FX_ROLL8,
	FX_BUILD,
	FX_STRETCH
};

typedef struct {
	int slice;
	int fx;
} PatStep;

typedef struct {
	int active;
	int wrap;
	int base;        // absolute offset into bank_pcm
	int sliceLen;
	float vel;
	float g;
	float fade;      // attack ramp countdown (frames)
	float rel;       // release countdown (frames)

	// granular time-stretch state
	float curPos;    // read position (source samples) of current grain
	float adv;       // source advance per output frame (BANK_SR / sr)
	float nextPos;   // read position where the next grain starts
	float crossPos;  // second read position while crossfading
	float eff;       // source advance per output frame required for tempo/pitch
	int left;        // output frames left in the current grain
	int crossLeft;   // crossfade countdown
} Voice;

typedef struct {
	long t;
	int slice;
	float vel;
	int wrap;
	float pitch;
} Trig;

struct Engine {
	int sr;
	EngCtrl ctrl;

	// transport
	int playing;
	int step;
	double eighthFrames;
	double phase;
	long frame;

	// voices / triggers
	Voice voices[MAXVOICES];
	Trig trigs[MAXTRIGS];
	int trigHead, trigTail;

	// phrase pattern
	PatStep pat[ENG_SLICES];
	PatStep patMem[ENG_PAT_SLOTS][ENG_SLICES];
	int patHas[ENG_PAT_SLOTS];
	int bankCur;

	// filter state
	float lpL, lpR, hpL, hpR, xpL, xpR;

	// echo delay line
	float* dline;
	int dlen;
	int dpos;
	float damp;
	float wet;

	// clock out
	int clkPeriodNote;
	int clkPhase;
	int clkPulseW;
	int clkPulseOn;

	// meters
	int curStep;
	int voiceCount;

	// rng
	unsigned rng;
};

static float clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }

static unsigned rnd(Engine* e) {
	unsigned x = e->rng;
	x ^= x << 13; x ^= x >> 17; x ^= x << 5;
	e->rng = x;
	return x;
}

static float readAt(int idx) {
	if (idx < 0) idx = 0;
	if (idx >= BANK_TOTAL) idx = BANK_TOTAL - 1;
	return (float)bank_pcm[idx] / 32768.0f;
}

static void sched(Engine* e, long t, int slice, float vel, int wrap, float pitch) {
	int head = (e->trigHead + 1) % MAXTRIGS;
	if (head == e->trigTail) return;
	e->trigs[e->trigHead].t = t;
	e->trigs[e->trigHead].slice = slice;
	e->trigs[e->trigHead].vel = vel;
	e->trigs[e->trigHead].wrap = wrap;
	e->trigs[e->trigHead].pitch = pitch;
	e->trigHead = head;
}

static void spawn(Engine* e, int slice, float vel, int wrap, float pitch) {
	int i, best = -1;
	for (i = 0; i < MAXVOICES; i++) {
		if (!e->voices[i].active) { best = i; break; }
	}
	if (best < 0) {
		best = 0;
		for (i = 1; i < MAXVOICES; i++) {
			if (e->voices[i].curPos > e->voices[best].curPos) best = i;
		}
	}
	const BankEntry* be = &bank_entries[e->bankCur];
	Voice* v = &e->voices[best];
	v->active = 1;
	v->wrap = wrap;
	v->base = be->off + slice * be->slice_len;
	v->sliceLen = be->slice_len;
	v->vel = clampf(vel, 0.1f, 1.2f);
	v->g = 0.9f * v->vel;
	v->fade = 24.0f;
	v->rel = 0;

	v->adv = (float)BANK_SR / (float)e->sr;
	v->eff = (float)v->sliceLen / (float)e->eighthFrames * pitch;
	if (v->eff < 0.05f) v->eff = 0.05f;
	v->curPos = (float)v->base;
	v->nextPos = v->curPos + GRAIN * v->eff;
	v->crossPos = v->curPos;
	v->left = GRAIN;
	v->crossLeft = 0;
}

// Generate a fresh 16-step phrase from `variation`.
static void eng_phrase(Engine* e) {
	for (int s = 0; s < ENG_SLICES; s++) {
		e->pat[s].slice = s;
		e->pat[s].fx = FX_NONE;
	}
	int v = e->ctrl.variation;
	if (v <= 0) return;

	// slice swaps
	int swaps = v >= 75 ? 3 : (v >= 40 ? 2 : 1);
	for (int i = 0; i < swaps; i++) {
		int a = rnd(e) % ENG_SLICES;
		int b = rnd(e) % ENG_SLICES;
		if (a == b) continue;
		int t = e->pat[a].slice;
		e->pat[a].slice = e->pat[b].slice;
		e->pat[b].slice = t;
	}

	// end-of-phrase fill (from 8/4/2 steps before the end)
	if ((int)(rnd(e) % 100) < v) {
		int k = v >= 70 ? 8 : (v >= 40 ? 4 : 2);
		int tool = rnd(e) % 5;
		int fx = tool == 0 ? FX_ROLL2 :
		         tool == 1 ? FX_ROLL4 :
		         tool == 2 ? FX_ROLL8 :
		         tool == 3 ? FX_BUILD : FX_STRETCH;
		for (int s = ENG_SLICES - k; s < ENG_SLICES; s++) {
			e->pat[s].fx = fx;
		}
	}
}

static void eng_step_event(Engine* e) {
	int s = e->step;
	PatStep ps = e->pat[s];
	int hits = 1;
	float velDrop = 1.0f;
	float basePitch = 1.0f;
	int wrap = e->ctrl.stretch ? 1 : 0;

	int fx = ps.fx;
	if (e->ctrl.build) fx = FX_BUILD;
	if (e->ctrl.roll > 1) {
		hits = e->ctrl.roll;
		velDrop = 0.72f;
		fx = FX_NONE;
	}

	switch (fx) {
		case FX_ROLL2: hits = 2; velDrop = 0.75f; break;
		case FX_ROLL4: hits = 4; velDrop = 0.72f; break;
		case FX_ROLL8: hits = 8; velDrop = 0.70f; break;
		case FX_BUILD: {
			int group = (s % 8) / 2;
			hits = 1 << group;
			velDrop = 0.75f;
			break;
		}
		case FX_STRETCH: wrap = 1; hits = 1; break;
		default: break;
	}

	float eighth = (float)e->eighthFrames;
	long base = e->frame;
	for (int h = 0; h < hits; h++) {
		long t = base + (long)(h * eighth / hits);
		float vel = (h == 0) ? 1.0f : velDrop * (float)pow(0.75, h);
		float pitch = (fx == FX_ROLL2 || fx == FX_ROLL4 || fx == FX_ROLL8 ||
		              fx == FX_BUILD) ? (float)pow(2.0, h / 12.0) : basePitch;
		sched(e, t, ps.slice, vel, wrap, pitch);
	}
}

void eng_play(Engine* e, int on) {
	if (on && !e->playing) {
		e->playing = 1;
		e->step = 0;
		e->phase = 0;
		e->clkPhase = 0;
		eng_phrase(e);
	}
	e->playing = on;
}

void eng_trigger_slice(Engine* e, int slice) {
	if (slice < 0 || slice >= ENG_SLICES) return;
	sched(e, e->frame, slice, 1.0f, e->ctrl.stretch ? 1 : 0, 1.0f);
}

static void eng_apply_bank(Engine* e, int bank) {
	if (bank < 0) bank = 0;
	if (bank >= BANK_COUNT) bank = BANK_COUNT - 1;
	e->bankCur = bank;
	eng_phrase(e);
}

void eng_set_ctrl(Engine* e, const EngCtrl* c) {
	if (c->bank != e->bankCur) {
		e->ctrl.bank = c->bank;
		eng_apply_bank(e, c->bank);
	}
	e->ctrl = *c;
	e->eighthFrames = (double)(60.0 * e->sr) / (c->bpm * 2.0);

	switch (c->clk_mode) {
		case ENG_CLK_MIDI24: e->clkPeriodNote = (int)((double)(60.0 * e->sr) / (c->bpm * 24.0)); break;
		case ENG_CLK_S16:    e->clkPeriodNote = (int)((double)(60.0 * e->sr) / (c->bpm * 16.0)); break;
		case ENG_CLK_S8:     e->clkPeriodNote = (int)((double)(60.0 * e->sr) / (c->bpm * 8.0)); break;
		default: e->clkPeriodNote = 0; break;
	}
	e->clkPulseW = e->clkPeriodNote / 6;
	if (e->clkPulseW < 6) e->clkPulseW = 6;
	if (e->clkPulseW > e->clkPeriodNote) e->clkPulseW = e->clkPeriodNote;
	e->damp = 0.72f;
	e->wet = 0.38f;
}

Engine* eng_create(int output_rate) {
	Engine* e = (Engine*)calloc(1, sizeof(Engine));
	if (!e) return NULL;
	e->sr = output_rate;
	e->ctrl.bpm = 175;
	e->ctrl.vol = 100;
	e->ctrl.bank = 0;
	e->ctrl.variation = 50;
	e->bankCur = -1;
	e->rng = 0x12345678u ^ (unsigned)output_rate;
	e->eighthFrames = (double)(60.0 * output_rate) / (175 * 2.0);
	e->dlen = output_rate * 3 / 2;
	e->dline = (float*)calloc(e->dlen, sizeof(float));
	e->playing = 0;
	e->phase = 0;
	eng_apply_bank(e, 0);
	return e;
}

void eng_destroy(Engine* e) {
	if (!e) return;
	free(e->dline);
	free(e);
}

int eng_current_step(const Engine* e) { return e->curStep; }
int eng_is_playing(const Engine* e) { return e->playing; }
int eng_voices(const Engine* e) { return e->voiceCount; }
const char* eng_bank_name(const Engine* e) { return bank_entries[e->bankCur].name; }
int eng_bank_bpm(const Engine* e) { return bank_entries[e->bankCur].bpm; }

void eng_pat_save(Engine* e, int slot) {
	if (slot < 0 || slot >= ENG_PAT_SLOTS) return;
	for (int s = 0; s < ENG_SLICES; s++) e->patMem[slot][s] = e->pat[s];
	e->patHas[slot] = 1;
}

void eng_pat_load(Engine* e, int slot) {
	if (slot < 0 || slot >= ENG_PAT_SLOTS || !e->patHas[slot]) return;
	for (int s = 0; s < ENG_SLICES; s++) e->pat[s] = e->patMem[slot][s];
}

int eng_pat_has(const Engine* e, int slot) {
	return slot >= 0 && slot < ENG_PAT_SLOTS && e->patHas[slot];
}

void eng_render(Engine* e, int16_t* l, int16_t* r, int frames) {
	for (int f = 0; f < frames; f++) {
		long now = e->frame;

		if (e->playing) {
			e->phase += 1.0;
			while (e->phase >= e->eighthFrames) {
				e->phase -= e->eighthFrames;
				int s = e->step;
				eng_step_event(e);
				e->step = (s + 1) % ENG_SLICES;
				e->curStep = s;
				if (e->step == 0 && !e->ctrl.freeze) eng_phrase(e);
			}
		}

		while (e->trigHead != e->trigTail) {
			Trig* t = &e->trigs[e->trigTail];
			if (t->t > now) break;
			spawn(e, t->slice, t->vel, t->wrap, t->pitch);
			e->trigTail = (e->trigTail + 1) % MAXTRIGS;
		}

		float dryL = 0, dryR = 0;
		int vc = 0;
		for (int i = 0; i < MAXVOICES; i++) {
			Voice* v = &e->voices[i];
			if (!v->active) continue;
			vc++;

			float env = 1.0f;
			if (v->fade > 0) {
				v->fade -= 1.0f;
				env = (24.0f - v->fade) / 24.0f;
			}
			if (v->rel > 0) {
				env *= v->rel / 64.0f;
				v->rel -= 1.0f;
				if (v->rel <= 0) { v->active = 0; continue; }
			}

			float samp;
			if (v->crossLeft > 0) {
				float a = v->crossLeft / (float)CROSS;
				float sOld = readAt((int)v->curPos);
				v->curPos += v->adv;
				float sNew = readAt((int)v->crossPos);
				v->crossPos += v->adv;
				samp = sOld * a + sNew * (1.0f - a);
				if (--v->crossLeft == 0) {
					v->curPos = v->crossPos;
					v->left = GRAIN - CROSS;
				}
			} else {
				samp = readAt((int)v->curPos);
				v->curPos += v->adv;
			}
			v->left--;

			// start crossfade into the next grain
			if (v->left <= CROSS && v->crossLeft == 0) {
				v->crossPos = v->nextPos;
				v->crossLeft = CROSS;
				v->nextPos += GRAIN * v->eff;
			}

			// source bounds
			int end = v->base + v->sliceLen;
			if (v->wrap) {
				if (v->curPos >= end) v->curPos -= v->sliceLen;
				if (v->crossPos >= end) v->crossPos -= v->sliceLen;
				if (v->nextPos >= end) v->nextPos -= v->sliceLen;
			} else {
				if (v->curPos >= end && v->crossLeft == 0 && v->rel <= 0) {
					v->rel = 64.0f;
				}
			}

			float g = v->g * env;
			dryL += samp * g * 0.55f;
			dryR += samp * g * 0.55f;
		}
		e->voiceCount = vc;

		// DJ filter
		float xL = dryL, xR = dryR;
		if (e->ctrl.filt == ENG_FILT_LP) {
			float fc = 100.0f + 9000.0f * powf(e->ctrl.cutoff_n, 1.5f);
			float a = 1.0f - expf(-2.0f * (float)M_PI * fc / e->sr);
			e->lpL += a * (xL - e->lpL); e->lpR += a * (xR - e->lpR);
			xL = e->lpL; xR = e->lpR;
		} else if (e->ctrl.filt == ENG_FILT_HP) {
			float fc = 200.0f + 8000.0f * powf(1.0f - e->ctrl.cutoff_n, 1.5f);
			float a = 1.0f - expf(-2.0f * (float)M_PI * fc / e->sr);
			float nL = a * (e->xpL + xL - e->lpL);
			float nR = a * (e->xpR + xR - e->lpR);
			e->lpL = xL; e->lpR = xR;
			e->xpL = nL; e->xpR = nR;
			xL = nL; xR = nR;
		}

		// echo
		float wetL = 0, wetR = 0;
		if (e->ctrl.echo) {
			int len = e->dlen;
			int dp = e->dpos;
			int delay = (int)(e->eighthFrames * 1.5f);
			if (delay < 32) delay = 32;
			if (delay > len) delay = len;
			int rd = dp - delay;
			if (rd < 0) rd += len;
			float dl = e->dline[rd];
			wetL = dl; wetR = dl;
			float fb = (e->dline[dp] + xL) * 0.45f;
			float damped = e->damp * e->dline[(dp - 1 + len) % len] + (1.0f - e->damp) * fb;
			e->dline[dp] = damped;
			e->dpos = dp + 1;
			if (e->dpos >= len) e->dpos = 0;
		}

		float outL = xL + wetL * e->wet;
		float outR = xR + wetR * e->wet;

		// clock out pulse on LEFT
		if (e->playing && e->clkPeriodNote > 0) {
			e->clkPhase++;
			if (e->clkPhase >= e->clkPeriodNote) {
				e->clkPhase = 0;
				e->clkPulseOn = e->clkPulseW;
			}
			if (e->clkPulseOn > 0) {
				e->clkPulseOn--;
				outL += 0.55f;
			}
		}

		float vol = e->ctrl.vol * 0.008f;
		outL = tanhf(outL * 1.4f) / 1.4f * vol;
		outR = tanhf(outR * 1.4f) / 1.4f * vol;
		l[f] = (int16_t)clampf(outL * 32767.0f, -32767.0f, 32767.0f);
		r[f] = (int16_t)clampf(outR * 32767.0f, -32767.0f, 32767.0f);

		e->frame++;
	}
}
