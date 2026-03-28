ENTRY(_start)

MEMORY
{
    ram : ORIGIN = 0x82100000, LENGTH = 64K
}

SECTIONS
{
    . = ORIGIN(ram);

    .text :
    {
        *(.text.startup)  /* Garantiza que _start sea lo primerito */
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