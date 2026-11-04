// crt0.s — 3DS entry point for Amenity3DS.
// Sets up a stack in bss, then hands control to libctru's initSystem()
// (which inits services via __appInit and the heap), then main().
	.section .crt0, "ax", %progbits
	.align 2
	.global _start
	.type _start, %function
_start:
	ldr	r2, =_stack_top
	mov	sp, r2
	bl	initSystem
	bl	main
	bl	__ctru_exit
1:	b	1b

	.section .bss.stack, "aw", %nobits
	.align 4
_stack_mem:
	.space 64 * 1024
	.global _stack_top
_stack_top:
