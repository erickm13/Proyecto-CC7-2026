ENTRY(main)

MEMORY
{
    ram : ORIGIN = 0x82100000, LENGTH = 64K
}

SECTIONS
{
    . = ORIGIN(ram);

    .text :
    {
        *(.text*)
        *(.rodata*)
    } > ram

    .data :
    {
        *(.data*)
    } > ram

    .bss :
    {
        *(.bss*)
        *(COMMON)
    } > ram
}