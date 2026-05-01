MEMORY
{
    RAM (rwx) : ORIGIN = 0x60000000, LENGTH = 128M
}

ENTRY(_start)

SECTIONS
{
    .vectors : {
        KEEP(*(.vectors))
    } > RAM

    .text : {
        *(.text*)
        *(.rodata*)

        . = ALIGN(4);
        __shell_cmd_start = .;
        KEEP(*(SORT(.shell_cmd*)))
        __shell_cmd_end = .;
    } > RAM

    .data : {
        *(.data*)
    } > RAM

    .bss (NOLOAD) : {
        _bss_start = .;
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _bss_end = .;
    } > RAM

    . = ALIGN(8);
    . = . + 1024;
    _irq_stack_top = .;

    . = . + 1024;
    _fiq_stack_top = .;

    . = . + 256;
    _abt_stack_top = .;

    . = . + 256;
    _und_stack_top = .;

    . = . + 4096;
    _svc_stack_top = .;

    . = ALIGN(8);
    __heap_start = .;
    __heap_end = ORIGIN(RAM) + LENGTH(RAM);
}
