// GBA entry + cartridge header (exact 0xC0-byte layout).
	.section .text.start, "ax", %progbits
	.balign 4

	.global _start

	.org 0x000
	// 0x000: branch over header to _start
	.word 0xEA000000 | (((_start - . - 8) >> 2) & 0x00FFFFFF)
	// 0x004..0x09F: logo + reserved (zeroed; emulators do not validate)
	.fill 0x9C, 1, 0
	// 0x0A0: title (12 bytes)
	.ascii "AMENITYDNB00"
	// 0x0AC: game code + maker
	.ascii "AMNB"
	.ascii "HC"
	// 0x0B2: fixed 0x96
	.byte 0x96
	// 0x0B3: main unit code
	.byte 0x00
	// 0x0B4: device type
	.byte 0x00
	// 0x0B5..0x0B7: reserved
	.fill 3, 1, 0
	// 0x0B8: version
	.byte 0x01
	// 0x0B9: complement (patched)
	.byte 0x00
	// 0x0BA: checksum (patched)
	.byte 0x00

	.org 0x0C0
_start:
	// stack
	ldr	sp, =_stack_top

	// copy .data from ROM to EWRAM
	ldr	r0, =__data_lma
	ldr	r1, =__data_vma
	ldr	r2, =__data_end
	cmp	r1, r2
	beq	2f
1:	ldmia	r0!, {r3-r10}
	stmia	r1!, {r3-r10}
	cmp	r1, r2
	blo	1b
2:

	// zero .bss
	ldr	r0, =__bss_start
	ldr	r1, =__bss_end
	mov	r2, #0
	cmp	r0, r1
	beq	4f
3:	str	r2, [r0], #4
	cmp	r0, r1
	blo	3b
4:

	bl	main
	b	.
