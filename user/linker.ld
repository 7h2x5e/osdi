SECTIONS
{
    . = 0x0;
    .text.entry : {
        *(.text.entry)
    }
    .text : {
        *(.text)
    }
    .data : {
        *(.data)
    }
    .bss : {
        . = ALIGN(16);
        *(.bss .bss.*)
    }
}
