SECTIONS
{
  . = 0x80000;
  PROVIDE(_code = .);
  .text : { 
    KEEP(*(.text.boot))
    *(.text .text.*)
  }
  .data : {
    *(.data .data.*)
  }
  .bss (NOLOAD) : {
      . = ALIGN(16);
      __bss_start = .;
      *(.bss .bss.*)
      __bss_end = .;
  }
  _end = .;
}
__bss_size = (__bss_end - __bss_start)>>3;
__loader_size = (_end - _code)>>3;