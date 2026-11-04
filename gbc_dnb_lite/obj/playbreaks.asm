;--------------------------------------------------------
; File Created by SDCC : free open source ISO C Compiler 
; Version 4.2.2 #13350 (Linux)
;--------------------------------------------------------
	.module playbreaks
	.optsdcc -msm83
	
;--------------------------------------------------------
; Public variables in this module
;--------------------------------------------------------
	.globl _play_break0
	.globl _set_sample
;--------------------------------------------------------
; special function registers
;--------------------------------------------------------
;--------------------------------------------------------
; ram data
;--------------------------------------------------------
	.area _DATA
;--------------------------------------------------------
; ram data
;--------------------------------------------------------
	.area _INITIALIZED
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
;src/playbreaks.c:7: void play_break0(void) {
;	---------------------------------
; Function play_break0
; ---------------------------------
_play_break0::
;src/playbreaks.c:8: set_sample(1, sample_break0, sizeof(sample_break0));
	ld	de, #0x3000
	push	de
	ld	de, #_sample_break0
	ld	a, #0x01
	call	_set_sample
;src/playbreaks.c:9: }
	ret
	.area _CODE
	.area _INITIALIZER
	.area _CABS (ABS)
