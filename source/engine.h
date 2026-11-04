// engine.h — Amenity-style live break slicer for 3DS (bank + pitch-locked
// granular stretch + phrase variation).
#pragma once
#include <stdint.h>

#define ENG_SLICES 16

typedef enum {
	ENG_FILT_OFF = 0,
	ENG_FILT_LP,
	ENG_FILT_HP
} EngFilt;

typedef enum {
	ENG_CLK_OFF = 0,
	ENG_CLK_MIDI24,
	ENG_CLK_S16,
	ENG_CLK_S8
} EngClk;

typedef struct {
	int bpm;        // 61..250
	int playing;    // transport running
	int roll;       // 0 off, else hits per 8th (2,4,8)
	int build;      // accelerating subdivisions across the phrase
	int echo;       // dotted-8th delay send
	int stretch;    // loop each hit at 1/4 speed
	int filt;       // EngFilt
	float cutoff_n; // 0..1
	int clk_mode;   // EngClk
	int vol;        // 0..100
	int bank;       // 0..BANK_COUNT-1
	int variation;  // 0..100 phrase variation amount (0 = straight)
	int freeze;     // keep the current phrase (no new variation)
} EngCtrl;

typedef struct Engine Engine;

Engine* eng_create(int output_rate);
void eng_destroy(Engine* e);

void eng_set_ctrl(Engine* e, const EngCtrl* c);
void eng_play(Engine* e, int on);
void eng_trigger_slice(Engine* e, int slice);
void eng_render(Engine* e, int16_t* l, int16_t* r, int frames);

int eng_current_step(const Engine* e);
int eng_is_playing(const Engine* e);
int eng_voices(const Engine* e);
const char* eng_bank_name(const Engine* e);
int eng_bank_bpm(const Engine* e);

#define ENG_PAT_SLOTS 8
void eng_pat_save(Engine* e, int slot);
void eng_pat_load(Engine* e, int slot);
int eng_pat_has(const Engine* e, int slot);
