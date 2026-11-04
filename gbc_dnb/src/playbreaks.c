// Banked play entry points for the four bundled breaks.
// Each break lives in its own MBC1 ROM bank (2..5); the streaming ISR
// switches banks internally, so these entry points live together in bank 2.
#pragma bank 2

#include "sample_player.h"

#include "../res/sample_break0.h"
#include "../res/sample_break1.h"
#include "../res/sample_break2.h"
#include "../res/sample_break3.h"

void play_break0(void) __banked { set_sample(2, sample_break0, sizeof(sample_break0)); }
void play_break1(void) __banked { set_sample(3, sample_break1, sizeof(sample_break1)); }
void play_break2(void) __banked { set_sample(4, sample_break2, sizeof(sample_break2)); }
void play_break3(void) __banked { set_sample(5, sample_break3, sizeof(sample_break3)); }
