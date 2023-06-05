OUTPUT_FORMAT(binary);
ENTRY(main);

SECTIONS
{
  . = 0x7E00;
  .boot : { *(.boot) }
  . = 0x8000;
  .stage15 : { *(.stage15) }
  .init : { *(.init) }
  stage2.o = .;
  .text : { *(.text) }
  .data = .;
  .rodata = .;
  .bss = .;
}
