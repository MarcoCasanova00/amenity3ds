// main.c — Amenity3DS: a live Amen-style break slicer for the Nintendo 3DS.
// Native homebrew (CIA). Uses libctru for services, NDSP for audio out.
#include <3ds.h>
#include <stdint.h>
#include <string.h>

#include "engine.h"
#include "ui.h"

#define NBUF   6
#define FRAMES 1536
#define SR     48000

static u16* buf[NBUF];
static ndspWaveBuf wb[NBUF];

// ---- NDSP streaming -------------------------------------------------------
static void audio_init(void) {
	ndspSetOutputMode(NDSP_OUTPUT_STEREO);
	ndspChnReset(0);
	ndspChnSetFormat(0, NDSP_FORMAT_STEREO_PCM16);
	ndspChnSetInterp(0, NDSP_INTERP_LINEAR);
	ndspChnSetRate(0, SR);
	{
		float mix[12] = { 1.0f, 1.0f, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
		ndspChnSetMix(0, mix);
	}

	for (int i = 0; i < NBUF; i++) {
		buf[i] = (u16*)linearAlloc(FRAMES * 2 * sizeof(u16));
		memset(buf[i], 0, FRAMES * 2 * sizeof(u16));
		wb[i].data_vaddr = buf[i];
		wb[i].nsamples = FRAMES;
		wb[i].status = NDSP_WBUF_FREE;
		ndspChnWaveBufAdd(0, &wb[i]);
	}
}

static void audio_refill(Engine* e) {
	for (int i = 0; i < NBUF; i++) {
		if (wb[i].status == NDSP_WBUF_DONE) {
			eng_render(e, (int16_t*)buf[i], (int16_t*)buf[i] + FRAMES, FRAMES);
			wb[i].status = NDSP_WBUF_QUEUED;
			ndspChnWaveBufAdd(0, &wb[i]);
		}
	}
}

static void audio_exit(void) {
	ndspChnWaveBufClear(0);
	ndspExit();
	for (int i = 0; i < NBUF; i++) {
		if (buf[i]) linearFree(buf[i]);
	}
}

// ---- startup hooks (called by libctru) -----------------------------------
void __appInit(void) {
	srvInit();
	aptInit();
	hidInit();
	gfxInitDefault();
	ndspInit();
}

void __appExit(void) {
	audio_exit();
	hidExit();
	gfxExit();
	aptExit();
}

// ---- device framebuffer blit (rotated storage) ---------------------------
// The 3DS framebuffers are stored rotated: memory row = screen X,
// memory column = screen Y, row length 240.  We render everything into
// landscape canvases first, then blit with that mapping.
static void blit_canvas(const u16* src, int sw, int sh, u16* dst, int dstride) {
	for (int y = 0; y < sh; y++) {
		const u16* srow = src + (size_t)y * sw;
		for (int x = 0; x < sw; x++) {
			dst[(size_t)x * dstride + (size_t)y] = srow[x];
		}
	}
}

int main(void) {
	App app;
	app_init(&app, SR);
	audio_init();

	u16* canvasTop = (u16*)linearAlloc(400 * 240 * sizeof(u16));
	u16* canvasBot = (u16*)linearAlloc(320 * 240 * sizeof(u16));

	while (aptMainLoop()) {
		hidScanInput();
		app_input(&app);

		u16* fbTop = (u16*)gfxGetFramebuffer(GFX_TOP, GFX_LEFT, NULL, NULL);
		u16* fbBot = (u16*)gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, NULL, NULL);

		app_draw_top(&app, canvasTop, 400);
		app_draw_bottom(&app, canvasBot, 320);

		blit_canvas(canvasTop, 400, 240, fbTop, 240);
		blit_canvas(canvasBot, 320, 240, fbBot, 240);

		gfxFlushBuffers();
		gfxSwapBuffers();
		gspWaitForVBlank();

		audio_refill(app.eng);
	}

	eng_destroy(app.eng);
	linearFree(canvasTop);
	linearFree(canvasBot);
	return 0;
}
