ENTRY(_start)

SECTIONS
{
    /* QEMU VersatilePB: Vector table MUST be at 0x00000000 */
    . = 0x00000000;

    .text :
    {
        *(.text*)
        *(.rodata*)
    }

    /* Alineamos la memoria de datos */
    . = ALIGN(4);
    .data :
    {
        *(.data*)
    }

    /* Aquí se definen las variables que usa tu root.s para limpiar el BSS */
    . = ALIGN(4);
    .bss :
    {
        __bss_start__ = .;
        *(.bss*)
        *(COMMON)
        __bss_end__ = .;
    }
    
    /* Stack grows down from here */
    _stack_top = 0x00010000;
}