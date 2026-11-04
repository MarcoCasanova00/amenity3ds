// host_test.c — offline sanity check for the Amenity3DS engine.
// Renders a few seconds of PCM to build/test.wav (48k stereo s16)
// to validate bank switching, slicing, variation/fills, effects and the
// audio clock-out pulse train without 3DS hardware.
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "engine.h"

#define SR 48000

static void write_wav_header(FILE* f, int frames) {
	unsigned char h[44] = {0};
	h[0]='R';h[1]='I';h[2]='F';h[3]='F';
	unsigned long data = (unsigned long)frames * 4;
	unsigned long sz = 36 + data;
	h[4]=sz&0xff; h[5]=(sz>>8)&0xff; h[6]=(sz>>16)&0xff; h[7]=(sz>>24)&0xff;
	h[8]='W';h[9]='A';h[10]='V';h[11]='E';
	h[12]='f';h[13]='m';h[14]='t';h[15]=' ';
	h[16]=16; h[17]=0; h[18]=0; h[19]=0;
	h[20]=1; h[21]=0;
	h[22]=2; h[23]=0;
	h[24]=SR&0xff; h[25]=(SR>>8)&0xff; h[26]=(SR>>16)&0xff; h[27]=(SR>>24)&0xff;
	unsigned brate = SR*4;
	h[28]=brate&0xff; h[29]=(brate>>8)&0xff; h[30]=(brate>>16)&0xff; h[31]=(brate>>24)&0xff;
	h[32]=4; h[33]=0;
	h[34]=16; h[35]=0;
	h[36]='d';h[37]='a';h[38]='t';h[39]='a';
	h[40]=data&0xff; h[41]=(data>>8)&0xff; h[42]=(data>>16)&0xff; h[43]=(data>>24)&0xff;
	fwrite(h, 1, 44, f);
}

int main(void) {
	int total = SR * 4;
	int16_t* L = malloc(sizeof(int16_t)*total);
	int16_t* R = malloc(sizeof(int16_t)*total);
	Engine* e = eng_create(SR);

	EngCtrl c;
	c.bpm = 175;
	c.playing = 1;
	c.roll = 0;
	c.build = 0;
	c.echo = 1;
	c.stretch = 0;
	c.filt = ENG_FILT_OFF;
	c.cutoff_n = 0.5f;
	c.clk_mode = ENG_CLK_MIDI24;
	c.vol = 100;
	c.bank = 2;       /* CLN103 */
	c.variation = 60;
	c.freeze = 0;
	eng_play(e, 1);
	eng_set_ctrl(e, &c);

	int pos = 0;
	int chunk = 4800;
	while (pos < total) {
		int n = total - pos; if (n > chunk) n = chunk;
		if (pos == SR) eng_trigger_slice(e, 9);
		if (pos == 2*SR) { c.roll = 4; c.filt = ENG_FILT_LP; c.cutoff_n = 0.2f; eng_set_ctrl(e, &c); }
		if (pos == 3*SR) { c.roll = 0; c.bank = 5; c.variation = 80; eng_set_ctrl(e, &c); }
		eng_render(e, L + pos, R + pos, n);
		pos += n;
	}

	FILE* f = fopen("build/test.wav", "wb");
	write_wav_header(f, total);
	fwrite(L, 2, total, f);
	fwrite(R, 2, total, f);
	fclose(f);

	long sum = 0; int peaks = 0;
	for (int i = 0; i < total; i++) {
		int v = L[i]; if (v < 0) v = -v;
		sum += v;
		if (v > 30000) peaks++;
	}
	printf("host test: %d frames rendered\n", total);
	printf("avg |L| = %ld, loud frames (>30000) = %d\n", sum / total, peaks);
	printf("bank = %s, voices = %d, step = %d, playing = %d\n",
		   eng_bank_name(e), eng_voices(e), eng_current_step(e), eng_is_playing(e));
	eng_destroy(e);
	free(L); free(R);
	return 0;
}
