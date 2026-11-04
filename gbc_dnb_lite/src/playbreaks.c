// No-mapper compatibility player: single break placed in the plain 32KB ROM
// (no MBC1 banking), so it runs even in emulators without bank switching.
#include "sample_player.h"

#include "../res/sample_break0.h"

void play_break0(void) {
	set_sample(1, sample_break0, sizeof(sample_break0));
}
