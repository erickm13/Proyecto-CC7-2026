ENTRY(_start)

MEMORY
{
    /* QEMU: P1 inicia en la marca de los 2MB */
    ram : ORIGIN = 0x00200000, LENGTH = 1M 
}

SECTIONS
{
    . = ORIGIN(ram);

    .text :
    {
        *(.text.startup)  
        *(.text*)
        *(.rodata*)
    } > ram

    .data : AT(ADDR(.text) + SIZEOF(.text))
    {
        __data_start__ = .;
        *(.data*)
        __data_end__ = .;
    } > ram

    .bss :
    {
        __bss_start__ = .;
        *(.bss*)
        *(COMMON)
        __bss_end__ = .;
    } > ram
}