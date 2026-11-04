// ui.h — touch/HID input mapping and screen rendering for Amenity3DS.
#pragma once
#include <stdint.h>
#include "engine.h"

typedef struct {
	Engine* eng;
	EngCtrl ctrl;
	int sel;        // focused pad (D-pad / A)
	int padFlash[16]; // frames remaining of pad flash
	int guide;      // guide menu open
	int patSlot;    // pattern memory slot
} App;

void app_init(App* a, int sr);
void app_input(App* a);                 // poll HID + touch
void app_draw_bottom(App* a, uint16_t* fb, int stride);
void app_draw_top(App* a, uint16_t* fb, int stride);
