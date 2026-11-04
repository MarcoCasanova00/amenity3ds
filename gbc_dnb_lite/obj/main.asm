;--------------------------------------------------------
; File Created by SDCC : free open source ISO C Compiler 
; Version 4.2.2 #13350 (Linux)
;--------------------------------------------------------
	.module main
	.optsdcc -msm83
	
;--------------------------------------------------------
; Public variables in this module
;--------------------------------------------------------
	.globl _main
	.globl _tim_isr
	.globl _play_break0
	.globl _putchar
	.globl _gotoxy
	.globl _set_interrupts
	.globl _joypad
	.globl _add_TIM
	.globl _g_tick
;--------------------------------------------------------
; special function registers
;--------------------------------------------------------
;--------------------------------------------------------
; ram data
;--------------------------------------------------------
	.area _DATA
_g_tick::
	.ds 4
;--------------------------------------------------------
; ram data
;--------------------------------------------------------
	.area _INITIALIZED
_cur:
	.ds 1
_bpm:
	.ds 2
_running:
	.ds 1
_dest:
	.ds 1
_clk_mode:
	.ds 1
_out_state:
	.ds 1
_ck_last:
	.ds 4
_ck_ph:
	.ds 4
_pulse_count:
	.ds 2
;--------------------------------------------------------
; absolute external ram data
;--------------------------------------------------------
	.area _DABS (ABS)
;--------------------------------------------------------
; global & static initialisations
;--------------------------------------------------------
	.area _HOME
	.area _GSINIT
	.area _GSFINAL
	.area _GSINIT
;--------------------------------------------------------
; Home
;--------------------------------------------------------
	.area _HOME
	.area _HOME
;--------------------------------------------------------
; code
;--------------------------------------------------------
	.area _CODE
;src/main.c:21: void tim_isr(void) __interrupt(1) { g_tick++; }
;	---------------------------------
; Function tim_isr
; ---------------------------------
_tim_isr::
	ei
	push	af
	push bc
	push de
	push hl
	add	sp, #-4
	ld	hl, #_g_tick
	ld	d, h
	ld	e, l
	ldhl	sp,	#0
	ld	a, (de)
	ld	(hl+),	a
	inc	de
	ld	a, (de)
	ld	(hl+),	a
	inc	de
	ld	a, (de)
	ld	(hl+),	a
	inc	de
	ld	a, (de)
	ld	(hl), a
	pop	de
	push	de
	ld	a, e
	add	a, #0x01
	ld	e, a
	ld	a, d
	adc	a, #0x00
	push	af
	ld	hl, #_g_tick + 1
	ld	(hl-), a
	ld	(hl), e
	ldhl	sp,#4
	ld	a, (hl+)
	ld	e, a
	ld	d, (hl)
	pop	af
	ld	a, e
	adc	a, #0x00
	ld	e, a
	ld	a, d
	adc	a, #0x00
	ld	hl, #_g_tick + 3
	ld	(hl-), a
	ld	(hl), e
	add	sp, #4
	pop	hl
	pop de
	pop bc
	pop af
	ret
_BREAK_NAMES:
	.dw __str_0
__str_0:
	.ascii "CLN105"
	.db 0x00
;src/main.c:36: static void start_break(void) {
;	---------------------------------
; Function start_break
; ---------------------------------
_start_break:
;src/main.c:37: play_break0();
;src/main.c:38: }
	jp	_play_break0
_PPQN:
	.db #0x18	; 24
	.db #0x10	; 16
	.db #0x08	; 8
_MODE_N:
	.dw __str_1
	.dw __str_2
	.dw __str_3
__str_1:
	.ascii "S24"
	.db 0x00
__str_2:
	.ascii "S16"
	.db 0x00
__str_3:
	.ascii "S8"
	.db 0x00
;src/main.c:40: static void put_str(UINT8 x, UINT8 y, const char* s) {
;	---------------------------------
; Function put_str
; ---------------------------------
_put_str:
	ld	b, a
;src/main.c:41: gotoxy(x, y);
	ld	a, e
	push	af
	inc	sp
	push	bc
	inc	sp
	call	_gotoxy
	pop	hl
;src/main.c:42: while (*s) putchar(*s++);
	ldhl	sp,	#2
	ld	a, (hl+)
	ld	c, a
	ld	b, (hl)
00101$:
	ld	a, (bc)
	or	a, a
	jr	Z, 00104$
	inc	bc
	push	bc
	push	af
	inc	sp
	call	_putchar
	inc	sp
	pop	bc
	jr	00101$
00104$:
;src/main.c:43: }
	pop	hl
	pop	af
	jp	(hl)
;src/main.c:44: static void print_u16(UINT8 x, UINT8 y, UINT16 v) {
;	---------------------------------
; Function print_u16
; ---------------------------------
_print_u16:
	add	sp, #-6
	ld	b, a
;src/main.c:46: gotoxy(x, y);
	ld	a, e
	push	af
	inc	sp
	push	bc
	inc	sp
	call	_gotoxy
	pop	hl
;src/main.c:47: if (!v) { putchar('0'); return; }
	ldhl	sp,	#9
	ld	a, (hl-)
	or	a, (hl)
	jr	NZ, 00115$
	ld	a, #0x30
	push	af
	inc	sp
	call	_putchar
	inc	sp
	jr	00110$
;src/main.c:48: while (v && n < 5) { b[n++] = '0' + v % 10; v /= 10; }
00115$:
	ld	e, #0x00
00104$:
	ldhl	sp,	#9
	ld	a, (hl-)
	or	a, (hl)
	jr	Z, 00117$
	ld	a, e
	sub	a, #0x05
	jr	NC, 00117$
	push	de
	ld	d, #0x00
	ld	hl, #2
	add	hl, sp
	add	hl, de
	pop	de
	inc	e
	push	hl
	push	de
	ld	bc, #0x000a
	push	hl
	ldhl	sp,	#14
	ld	e, (hl)
	ldhl	sp,	#15
	ld	d, (hl)
	pop	hl
	call	__moduint
	pop	de
	pop	hl
	ld	a, c
	add	a, #0x30
	ld	(hl), a
	push	de
	ld	bc, #0x000a
	ldhl	sp,	#10
	ld	a, (hl+)
	ld	e, a
	ld	d, (hl)
	call	__divuint
	pop	de
	ldhl	sp,	#8
	ld	a, c
	ld	(hl+), a
	ld	(hl), b
	jr	00104$
;src/main.c:49: while (n) putchar(b[--n]);
00117$:
	ld	c, e
00107$:
	ld	a, c
	or	a, a
	jr	Z, 00110$
	dec	c
	ld	e, c
	ld	d, #0x00
	ld	hl, #0
	add	hl, sp
	add	hl, de
	ld	a, (hl)
	push	bc
	push	af
	inc	sp
	call	_putchar
	inc	sp
	pop	bc
	jr	00107$
00110$:
;src/main.c:50: }
	add	sp, #6
	pop	hl
	pop	af
	jp	(hl)
;src/main.c:52: static void draw_screen(void) {
;	---------------------------------
; Function draw_screen
; ---------------------------------
_draw_screen:
;src/main.c:53: put_str(0, 0, "DNB III       ");
	ld	de, #___str_4
	push	de
	xor	a, a
	ld	e, a
	call	_put_str
;src/main.c:54: put_str(0, 1, "BREAK ");
	ld	de, #___str_5
	push	de
	ld	e, #0x01
	xor	a, a
	call	_put_str
;src/main.c:55: put_str(6, 1, BREAK_NAMES[cur]);
	ld	bc, #_BREAK_NAMES+0
	ld	hl, #_cur
	ld	l, (hl)
;	spillPairReg hl
;	spillPairReg hl
	ld	h, #0x00
;	spillPairReg hl
;	spillPairReg hl
	add	hl, hl
	add	hl, bc
	ld	a, (hl+)
	ld	c, a
	ld	b, (hl)
	push	bc
	ld	e, #0x01
	ld	a, #0x06
	call	_put_str
;src/main.c:56: put_str(0, 2, "BPM  ");
	ld	de, #___str_6
	push	de
	ld	e, #0x02
	xor	a, a
	call	_put_str
;src/main.c:57: print_u16(5, 2, bpm);
	ld	hl, #_bpm
	ld	a, (hl+)
	ld	e, a
	ld	d, (hl)
	push	de
	ld	e, #0x02
	ld	a, #0x05
	call	_print_u16
;src/main.c:58: put_str(0, 3, running ? "RUN  " : "STOP ");
	ld	a, (#_running)
	or	a, a
	jr	Z, 00103$
	ld	bc, #___str_7+0
	jr	00104$
00103$:
	ld	bc, #___str_8+0
00104$:
	push	bc
	ld	e, #0x03
	xor	a, a
	call	_put_str
;src/main.c:59: put_str(0, 4, MODE_N[clk_mode]);
	ld	bc, #_MODE_N+0
	ld	hl, #_clk_mode
	ld	l, (hl)
;	spillPairReg hl
;	spillPairReg hl
	ld	h, #0x00
;	spillPairReg hl
;	spillPairReg hl
	add	hl, hl
	add	hl, bc
	ld	a, (hl+)
	ld	c, a
	ld	b, (hl)
	push	bc
	ld	e, #0x04
	xor	a, a
	call	_put_str
;src/main.c:60: put_str(6, 4, dest ? " JACK" : " LINK");
	ld	a, (#_dest)
	or	a, a
	jr	Z, 00105$
	ld	bc, #___str_9+0
	jr	00106$
00105$:
	ld	bc, #___str_10+0
00106$:
	push	bc
	ld	e, #0x04
	ld	a, #0x06
	call	_put_str
;src/main.c:61: put_str(0, 5, "L/R break U/D BPM A run");
	ld	de, #___str_11
	push	de
	ld	e, #0x05
	xor	a, a
	call	_put_str
;src/main.c:62: put_str(0, 6, "B mode SEL out L click");
	ld	de, #___str_12
	push	de
	ld	e, #0x06
	xor	a, a
	call	_put_str
;src/main.c:63: }
	ret
___str_4:
	.ascii "DNB III       "
	.db 0x00
___str_5:
	.ascii "BREAK "
	.db 0x00
___str_6:
	.ascii "BPM  "
	.db 0x00
___str_7:
	.ascii "RUN  "
	.db 0x00
___str_8:
	.ascii "STOP "
	.db 0x00
___str_9:
	.ascii " JACK"
	.db 0x00
___str_10:
	.ascii " LINK"
	.db 0x00
___str_11:
	.ascii "L/R break U/D BPM A run"
	.db 0x00
___str_12:
	.ascii "B mode SEL out L click"
	.db 0x00
;src/main.c:65: static void send_edge(void) {
;	---------------------------------
; Function send_edge
; ---------------------------------
_send_edge:
;src/main.c:66: out_state ^= 1;
	ld	hl, #_out_state
	ld	a, (hl)
	xor	a, #0x01
	ld	(hl), a
;src/main.c:67: SER_D = out_state ? 0xFF : 0x00;
	ld	a, (hl)
	or	a, a
	ld	a, #0xff
	jr	NZ, 00107$
	xor	a, a
00107$:
	ldh	(#0xff01), a
;src/main.c:68: SER_C = 0x81;
	ld	a, #0x81
	ldh	(#0xff02), a
;src/main.c:69: while (SER_C & 0x80) /* wait */ ;
00101$:
	ldh	a, (#0xff02)
	rlca
	jr	C, 00101$
;src/main.c:70: SER_C = 0x00;
	xor	a, a
	ldh	(#0xff02), a
;src/main.c:71: }
	ret
;src/main.c:73: static void jack_pulse(UINT8 accent) {
;	---------------------------------
; Function jack_pulse
; ---------------------------------
_jack_pulse:
;src/main.c:74: if (accent) {
	or	a, a
	jr	Z, 00102$
;src/main.c:75: NR10_REG = 0x50;
	ld	a, #0x50
	ldh	(_NR10_REG + 0), a
;src/main.c:76: NR11_REG = 0x3F;
	ld	a, #0x3f
	ldh	(_NR11_REG + 0), a
;src/main.c:77: NR12_REG = 0xF1;
	ld	a, #0xf1
	ldh	(_NR12_REG + 0), a
;src/main.c:78: NR13_REG = 0x11;
	ld	a, #0x11
	ldh	(_NR13_REG + 0), a
;src/main.c:79: NR14_REG = 0xC0;
	ld	a, #0xc0
	ldh	(_NR14_REG + 0), a
	ret
00102$:
;src/main.c:81: NR10_REG = 0x00;
	xor	a, a
	ldh	(_NR10_REG + 0), a
;src/main.c:82: NR21_REG = 0x3F;
	ld	a, #0x3f
	ldh	(_NR21_REG + 0), a
;src/main.c:83: NR22_REG = 0xF0;
	ld	a, #0xf0
	ldh	(_NR22_REG + 0), a
;src/main.c:84: NR23_REG = 0x4A;
	ld	a, #0x4a
	ldh	(_NR23_REG + 0), a
;src/main.c:85: NR24_REG = 0xC0;
	ld	a, #0xc0
	ldh	(_NR24_REG + 0), a
;src/main.c:87: }
	ret
;src/main.c:89: static void set_bpm_tma(void) {
;	---------------------------------
; Function set_bpm_tma
; ---------------------------------
_set_bpm_tma:
;src/main.c:91: UINT8 tma = 240 - (UINT8)(((bpm - 61) * 51u) / 100u);
	ld	hl, #_bpm
	ld	a, (hl+)
	add	a, #0xc3
	ld	c, a
	ld	a, (hl)
	adc	a, #0xff
	ld	b, a
	ld	l, c
	ld	h, b
	add	hl, hl
	add	hl, bc
	add	hl, hl
	add	hl, hl
	add	hl, hl
	add	hl, bc
	add	hl, hl
	add	hl, bc
	ld	bc, #0x0064
	ld	e, l
	ld	d, h
	call	__divuint
	ld	a, #0xf0
	sub	a, c
	ld	c, a
;src/main.c:92: if (tma > 240) tma = 240;
	ld	a, #0xf0
	sub	a, c
	jr	NC, 00102$
	ld	c, #0xf0
00102$:
;src/main.c:93: if (tma < 144) tma = 144;
	ld	a, c
	sub	a, #0x90
	jr	NC, 00104$
	ld	c, #0x90
00104$:
;src/main.c:94: __critical { rTMA = tma; }
	di
	ld	a, c
	ldh	(_TMA_REG + 0), a
	ei
;src/main.c:95: }
	ret
;src/main.c:97: void main(void) {
;	---------------------------------
; Function main
; ---------------------------------
_main::
	add	sp, #-9
;../gbc/gbdk/include/gb/gb.h:743: __asm__("di");
	di
;src/main.c:99: NR52_REG = 0x80;
	ld	a, #0x80
	ldh	(_NR52_REG + 0), a
;src/main.c:100: NR50_REG = 0x77;
	ld	a, #0x77
	ldh	(_NR50_REG + 0), a
;src/main.c:101: NR51_REG = 0xFF;
	ld	a, #0xff
	ldh	(_NR51_REG + 0), a
;src/main.c:102: rTMA = 192;
	ld	a, #0xc0
	ldh	(_TMA_REG + 0), a
;src/main.c:103: rTAC = 0x07;
	ld	a, #0x07
	ldh	(_TAC_REG + 0), a
;src/main.c:104: set_interrupts(VBL_IFLAG | TIM_IFLAG);
	ld	a, #0x05
	push	af
	inc	sp
	call	_set_interrupts
	inc	sp
;src/main.c:105: add_TIM(tim_isr);
	ld	de, #_tim_isr
	push	de
	call	_add_TIM
	inc	sp
	inc	sp
;../gbc/gbdk/include/gb/gb.h:727: __asm__("ei");
	ei
;src/main.c:108: UINT8 prevkeys = 0xFF;
	ldhl	sp,	#0
	ld	(hl), #0xff
;src/main.c:109: draw_screen();
	call	_draw_screen
00147$:
;src/main.c:112: UINT8 keys = joypad();
	call	_joypad
	ld	c, a
;src/main.c:113: UINT8 down = keys & ~prevkeys;
	ldhl	sp,	#0
	ld	a, (hl)
	cpl
	and	a, c
	ldhl	sp,	#6
	ld	(hl), a
;src/main.c:114: prevkeys = keys;
	ldhl	sp,	#0
	ld	(hl), c
;src/main.c:116: if (down & J_RIGHT) { cur = (cur + 1) % NBREAKS; if (running) start_break(); draw_screen(); }
	push	hl
	ldhl	sp,	#8
	bit	0, (hl)
	pop	hl
	jr	Z, 00104$
	ld	hl, #_cur
	ld	e, (hl)
	ld	d, #0x00
	inc	de
	ld	bc, #0x0001
	call	__modsint
	ld	hl, #_cur
	ld	(hl), c
	ld	a, (#_running)
	or	a, a
	jr	Z, 00102$
	call	_start_break
00102$:
	call	_draw_screen
00104$:
;src/main.c:117: if (down & J_LEFT)  { cur = (cur + NBREAKS - 1) % NBREAKS; if (running) start_break(); draw_screen(); }
	ldhl	sp,	#6
	ld	a, (hl+)
	and	a, #0x02
	ld	(hl+), a
	ld	(hl), #0x00
	dec	hl
	or	a, #0x00
	jr	Z, 00108$
	xor	a, a
;	spillPairReg hl
;	spillPairReg hl
	ld	(_cur), a
	ld	a, (#_running)
	or	a, a
	jr	Z, 00106$
	call	_start_break
00106$:
	call	_draw_screen
00108$:
;src/main.c:118: if (down & J_UP)    { if (bpm < 250) { bpm++; set_bpm_tma(); } draw_screen(); }
	push	hl
	ldhl	sp,	#8
	bit	2, (hl)
	pop	hl
	jr	Z, 00112$
	ld	hl, #_bpm
	ld	a, (hl+)
	sub	a, #0xfa
	ld	a, (hl)
	sbc	a, #0x00
	jr	NC, 00110$
	dec	hl
	inc	(hl)
	jr	NZ, 00263$
	inc	hl
	inc	(hl)
00263$:
	call	_set_bpm_tma
00110$:
	call	_draw_screen
00112$:
;src/main.c:119: if (down & J_DOWN)  { if (bpm > 61)  { bpm--; set_bpm_tma(); } draw_screen(); }
	push	hl
	ldhl	sp,	#8
	bit	3, (hl)
	pop	hl
	jr	Z, 00116$
	ld	hl, #_bpm
	ld	a, #0x3d
	sub	a, (hl)
	inc	hl
	ld	a, #0x00
	sbc	a, (hl)
	jr	NC, 00114$
	ld	hl, #_bpm
	ld	a, (hl+)
	ld	e, a
	ld	a, (hl-)
	ld	d, a
	dec	de
	ld	a, e
	ld	(hl+), a
	ld	(hl), d
	call	_set_bpm_tma
00114$:
	call	_draw_screen
00116$:
;src/main.c:120: if (down & (J_A | J_START)) {
	ldhl	sp,	#6
	ld	a, (hl)
	and	a, #0x90
	jr	Z, 00120$
;src/main.c:121: running = !running;
	ld	hl, #_running
	ld	a, (hl)
	sub	a,#0x01
	ld	a, #0x00
	rla
	ld	(hl), a
;src/main.c:122: pulse_count = 0;
	xor	a, a
	ld	hl, #_pulse_count
	ld	(hl+), a
	ld	(hl), a
;src/main.c:123: ck_ph = 0;
	xor	a, a
	ld	hl, #_ck_ph
	ld	(hl+), a
	ld	(hl+), a
	ld	(hl+), a
	ld	(hl), a
;src/main.c:124: if (running) start_break();
	ld	a, (#_running)
	or	a, a
	jr	Z, 00118$
	call	_start_break
00118$:
;src/main.c:125: draw_screen();
	call	_draw_screen
00120$:
;src/main.c:127: if (down & J_B) { clk_mode = (clk_mode + 1) % 3; draw_screen(); }
	push	hl
	ldhl	sp,	#8
	bit	5, (hl)
	pop	hl
	jr	Z, 00122$
	ld	hl, #_clk_mode
	ld	e, (hl)
	ld	d, #0x00
	inc	de
	ld	bc, #0x0003
	call	__modsint
	ld	hl, #_clk_mode
	ld	(hl), c
	call	_draw_screen
00122$:
;src/main.c:128: if (down & J_SELECT) { dest = !dest; draw_screen(); }
	push	hl
	ldhl	sp,	#8
	bit	6, (hl)
	pop	hl
	jr	Z, 00124$
	ld	hl, #_dest
	ld	a, (hl)
	sub	a,#0x01
	ld	a, #0x00
	rla
	ld	(hl), a
	call	_draw_screen
00124$:
;src/main.c:129: if (down & J_LEFT) { /* metronome toggle (channel 1) via vol on/off */
	ldhl	sp,	#8
	ld	a, (hl-)
	or	a, (hl)
	jr	Z, 00129$
;src/main.c:130: if (NR12_REG == 0x00) NR12_REG = 0xF1; else NR12_REG = 0x00;
	ld	a, (#_NR12_REG)
	or	a, a
	jr	NZ, 00126$
	ld	a, #0xf1
	ldh	(_NR12_REG + 0), a
	jr	00129$
00126$:
	xor	a, a
	ldh	(_NR12_REG + 0), a
00129$:
;src/main.c:134: if (running && play_length == 0) start_break();
	ld	a, (#_running)
	or	a, a
	jr	Z, 00131$
	ld	hl, #_play_length + 1
	ld	a, (hl-)
	or	a, (hl)
	jr	NZ, 00131$
	call	_start_break
00131$:
;src/main.c:138: UINT32 now = g_tick;
	ld	hl, #_g_tick
	ld	d, h
	ld	e, l
	ldhl	sp,	#1
	ld	a, (de)
	ld	(hl+),	a
	inc	de
	ld	a, (de)
	ld	(hl+),	a
	inc	de
	ld	a, (de)
	ld	(hl+),	a
	inc	de
	ld	a, (de)
	ld	(hl), a
;src/main.c:137: if (running) {
	ld	a, (#_running)
	or	a, a
	jp	Z, 00142$
;src/main.c:139: UINT32 dt = now - ck_last;
	ldhl	sp,#1
	ld	a, (hl+)
	ld	e, a
	ld	d, (hl)
	ld	a, e
	ld	hl, #_ck_last
	sub	a, (hl)
	inc	hl
	ld	e, a
	ld	a, d
	sbc	a, (hl)
	push	af
	ldhl	sp,	#8
	ld	(hl-), a
	ld	a, e
	ld	(hl-), a
	dec	hl
	ld	a, (hl+)
	ld	e, a
	ld	d, (hl)
	ld	hl, #_ck_last + 2
	pop	af
	ld	a, e
	sbc	a, (hl)
	inc	hl
	ld	e, a
	ld	a, d
	sbc	a, (hl)
	ldhl	sp,	#8
	ld	(hl-), a
	ld	(hl), e
;src/main.c:140: ck_last = now;
	ldhl	sp,	#1
	ld	d, h
	ld	e, l
	ld	hl, #_ck_last
	ld	a, (de)
	ld	(hl+),	a
	inc	de
	ld	a, (de)
	ld	(hl+),	a
	inc	de
	ld	a, (de)
	ld	(hl+),	a
	inc	de
	ld	a, (de)
	ld	(hl), a
;src/main.c:141: ck_ph += dt;
	ld	hl, #_ck_ph
	ld	a, (hl+)
	ld	e, a
	ld	d, (hl)
	ld	a, e
	ldhl	sp,	#5
	add	a, (hl)
	inc	hl
	ld	e, a
	ld	a, d
	adc	a, (hl)
	push	af
	ld	hl, #_ck_ph + 1
	ld	(hl-), a
	ld	(hl), e
	ld	hl, #_ck_ph + 2
	ld	a, (hl+)
	ld	e, a
	ld	d, (hl)
	ldhl	sp,	#9
	pop	af
	ld	a, e
	adc	a, (hl)
	inc	hl
	ld	e, a
	ld	a, d
	adc	a, (hl)
	ld	hl, #_ck_ph + 3
	ld	(hl-), a
	ld	(hl), e
;src/main.c:142: UINT32 period_ms = 60000UL / ((UINT32)bpm * PPQN[clk_mode]);
	ld	a, (#_bpm)
	ldhl	sp,	#5
	ld	(hl), a
	ld	a, (#_bpm + 1)
	ldhl	sp,	#6
	ld	(hl+), a
	xor	a, a
	ld	(hl+), a
	ld	(hl), a
	ld	a, #<(_PPQN)
	ld	hl, #_clk_mode
	add	a, (hl)
	ld	c, a
	ld	a, #>(_PPQN)
	adc	a, #0x00
	ld	b, a
	ld	a, (bc)
	ld	e, a
	ld	d, #0x00
	ld	bc, #0x0000
	push	bc
	push	de
	ldhl	sp,	#9
	ld	a, (hl+)
	ld	c, a
	ld	a, (hl+)
	ld	b, a
	ld	a, (hl+)
	ld	e, a
	ld	d, (hl)
	call	__mullong
	push	de
	push	bc
	ld	bc, #0xea60
	ld	de, #0x0000
	call	__divulong
	ldhl	sp,	#5
	ld	a, c
	ld	(hl+), a
	ld	a, b
	ld	(hl+), a
	ld	a, e
;src/main.c:143: if (period_ms == 0) period_ms = 1;
	ld	(hl+), a
	ld	(hl), d
	dec	hl
	dec	hl
	or	a, d
	or	a, (hl)
	dec	hl
	or	a, (hl)
	jr	NZ, 00138$
	ld	a, #0x01
	ld	(hl+), a
	xor	a, a
	ld	(hl+), a
	ld	(hl+), a
	ld	(hl), a
;src/main.c:144: while (ck_ph >= period_ms) {
00138$:
	ld	de, #_ck_ph
	ldhl	sp,	#5
	ld	a, (de)
	inc	de
	sub	a, (hl)
	inc	hl
	ld	a, (de)
	inc	de
	sbc	a, (hl)
	inc	hl
	ld	a, (de)
	inc	de
	sbc	a, (hl)
	inc	hl
	ld	a, (de)
	sbc	a, (hl)
	jp	C, 00147$
;src/main.c:145: ck_ph -= period_ms;
	ld	hl, #_ck_ph
	ld	a, (hl+)
	ld	e, a
	ld	d, (hl)
	ld	a, e
	ldhl	sp,	#5
	sub	a, (hl)
	inc	hl
	ld	e, a
	ld	a, d
	sbc	a, (hl)
	push	af
	ld	hl, #_ck_ph + 1
	ld	(hl-), a
	ld	(hl), e
	ld	hl, #_ck_ph + 2
	ld	a, (hl+)
	ld	e, a
	ld	d, (hl)
	ldhl	sp,	#9
	pop	af
	ld	a, e
	sbc	a, (hl)
	inc	hl
	ld	e, a
	ld	a, d
	sbc	a, (hl)
	ld	hl, #_ck_ph + 3
	ld	(hl-), a
	ld	(hl), e
;src/main.c:146: pulse_count++;
	ld	hl, #_pulse_count
	inc	(hl)
	jr	NZ, 00268$
	inc	hl
	inc	(hl)
00268$:
;src/main.c:147: if (dest) jack_pulse(pulse_count % 16 == 0);
	ld	a, (#_dest)
	or	a, a
	jr	Z, 00136$
	ld	a, (#_pulse_count)
	and	a, #0x0f
	ld	c, a
	ld	b, #0x00
	ld	a, c
	or	a, a
	or	a, b
	ld	a, #0x01
	jr	Z, 00270$
	xor	a, a
00270$:
	call	_jack_pulse
	jr	00138$
00136$:
;src/main.c:148: else send_edge();
	call	_send_edge
	jr	00138$
00142$:
;src/main.c:151: ck_last = g_tick;
	ldhl	sp,	#1
	ld	d, h
	ld	e, l
	ld	hl, #_ck_last
	ld	a, (de)
	ld	(hl+),	a
	inc	de
	ld	a, (de)
	ld	(hl+),	a
	inc	de
	ld	a, (de)
	ld	(hl+),	a
	inc	de
	ld	a, (de)
	ld	(hl), a
;src/main.c:152: ck_ph = 0;
	xor	a, a
	ld	hl, #_ck_ph
	ld	(hl+), a
	ld	(hl+), a
	ld	(hl+), a
	ld	(hl), a
	jp	00147$
;src/main.c:155: }
	add	sp, #9
	ret
	.area _CODE
	.area _INITIALIZER
__xinit__cur:
	.db #0x00	; 0
__xinit__bpm:
	.dw #0x00af
__xinit__running:
	.db #0x00	; 0
__xinit__dest:
	.db #0x00	; 0
__xinit__clk_mode:
	.db #0x00	; 0
__xinit__out_state:
	.db #0x00	; 0
__xinit__ck_last:
	.byte #0x00, #0x00, #0x00, #0x00	; 0
__xinit__ck_ph:
	.byte #0x00, #0x00, #0x00, #0x00	; 0
__xinit__pulse_count:
	.dw #0x0000
	.area _CABS (ABS)
