ENTRY(_start)

MEMORY
{
	RAM (rwx) : ORIGIN = 0x00100000, LENGTH = 48M
}

PHDRS
{
	text   PT_LOAD FLAGS(5);   /* R E */
	rodata PT_LOAD FLAGS(4);   /* R   */
	data   PT_LOAD FLAGS(6);   /* R W */
}

SECTIONS
{
	. = ORIGIN(RAM);

	.text : {
		*(.crt0)
		*(.text*)
		*(.init)
		*(.fini)
	} :text

	.rodata : {
		*(.rodata*)
		*(.ARM.exidx*)
		*(.ARM.extab*)
	} :rodata

	.data : {
		*(.data*)
	} :data

	.bss : {
		*(.bss*)
		*(COMMON)
	} :data
}
