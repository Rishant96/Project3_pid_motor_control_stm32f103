MEMORY
{
    FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 64K
    SRAM  (rwx) : ORIGIN = 0x20000000, LENGTH = 20K
}

/* Top of SRAM = initial stack pointer */
_estack = ORIGIN(SRAM) + LENGTH(SRAM);

SECTIONS
{
    .text :
    {
        . = ALIGN(4);
        KEEP(*(.isr_vector))   
        *(.text)
        *(.text*)
        *(.rodata)
        *(.rodata*)
        . = ALIGN(4);
        _etext = .;
    } > FLASH

    .data :
	{
		. = ALIGN(4);
		_sdata = .;
		*(.data)
		*(.data*)
		. = ALIGN(4);
		_edata = .;
	} > SRAM AT> FLASH

	_sidata = LOADADDR(.data);

    .bss :
    {
        . = ALIGN(4);
        _sbss = .;
        *(.bss)
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
    } > SRAM
}
