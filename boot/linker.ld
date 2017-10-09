OUTPUT_FORMAT(binary);
ENTRY(main);

SECTIONS
{
  . = 0x8400;
  .text : AT(0x8400){
    stage2.o (.text*);
    *(.text);
  }
  . = 0x17E00;
  .data : AT(0x17E00){ *(.data) }
  .bss : AT(0x17E00){ *(.bss) }
}
