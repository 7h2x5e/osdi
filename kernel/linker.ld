SECTIONS
{
  . = 0xffff000000000000; # kernel space
  . += 0x80000; # kernel load address
  _kernel_start = . ;
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
  .istack : {
    KEEP(*(.istack))
  }
  .kstack : {
    KEEP(*(.kstack))
  }
  .ustack : {
    KEEP(*(.ustack))
  }
  . = ALIGN(0x1000);
  pg_dir = .;
  .data.pgd :
  {
    . += (515 * (1 << 12)); /* (PGD * 1) + (PUD * 1) + (PMD * 1) + (PTE * 512) */
  }
  . = ALIGN(0x1000);
  _end = .;
}
__bss_size = (__bss_end - __bss_start)>>3;
__loader_size = (_end - _code)>>3;