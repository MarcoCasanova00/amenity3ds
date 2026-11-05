ENTRY(_start)

MEMORY
{
	ROM (rx)   : ORIGIN = 0x08000000, LENGTH = 32M
	EWRAM (rw) : ORIGIN = 0x02000000, LENGTH = 256K
	IWRAM (rw): ORIGIN = 0x03000000, LENGTH = 32K
}

SECTIONS
{
	.text : {
		*(.text.start)
		*(.text*)
		*(.rodata*)
	} >ROM

	.data : AT(ALIGN(LOADADDR(.text) + SIZEOF(.text), 4)) {
		__data_vma = .;
		*(.data*)
		. = ALIGN(4);
		__data_end = .;
	} >EWRAM
	__data_lma = LOADADDR(.data);

	.bss : {
		__bss_start = .;
		*(.bss*)
		*(COMMON)
		__bss_end = .;
	} >EWRAM

	. = 0x03007E00;
	_stack_top = .;
}
