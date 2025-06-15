OUTPUT_FORMAT(elf32-i386);
ENTRY(__start);

BIOS_ADDR = 0x0;
BIOS_SIZE = 0x500;
MEM0_ADDR = 0x500;
MEM0_SIZE = 0x7700;
ENTR_ADDR = 0x7C00;
ENTR_SIZE = 0x200;
BOOT_ADDR = 0x7E00;
BOOT_SIZE = 0x1BE;
MBR_ADDR  = 0x7FBE;
MBR_SIZE  = 0x42;
MEM1_ADDR = 0x8000;
MEM1_SIZE = 0x600;
ST2_ADDR  = 0x8600;
ST2_SIZE  = 26K;
ELF_ADDR  = 0x10400;
ELF_SIZE  = 30K;
SEG_ADDR  = 0x17E00;
SEG_SIZE  = 64K;
MMAP_ADDR = 0x27E00;
MMAP_SIZE = 64K;
IDT_ADDR  = 0x37E00;
IDT_SIZE  = 64K;
MEM2_ADDR = 0x47E00;
MEM2_SIZE = 0x38200;
EBDA_ADDR = 0x80000;
EBDA_SIZE = 128K;
VGA_ADDR  = 0xA0000;
VGA_SIZE  = 96K;
FBR_ADDR  = 0xB8000;
FBR_SIZE  = 32K;
UPPR_ADDR = 0xC0000;
UPPR_SIZE = 256K;

MEMORY {
    bios(rx)    :   org = BIOS_ADDR,  len = BIOS_SIZE
    mem0(rw)    :   org = MEM0_ADDR,  len = MEM0_SIZE
    entry(rw)   :   org = ENTR_ADDR,  len = ENTR_SIZE
    boot(rw)    :   org = BOOT_ADDR,  len = BOOT_SIZE
    mbr(rw)     :   org = MBR_ADDR,   len = MBR_SIZE
    mem1(rx)    :   org = MEM1_ADDR,  len = MEM1_SIZE
    stage2(rx)  :   org = ST2_ADDR,   len = ST2_SIZE
    elf(rw)     :   org = ELF_ADDR,   len = ELF_SIZE
    seg(rw)     :   org = SEG_ADDR,   len = SEG_SIZE
    mmap(rw)    :   org = MMAP_ADDR,  len = MMAP_SIZE
    idt(rw)     :   org = IDT_ADDR,   len = IDT_SIZE
    mem2(rx)    :   org = MEM2_ADDR,  len = MEM2_SIZE
    ebda(rx)    :   org = EBDA_ADDR,  len = EBDA_SIZE
    vga(rw)     :   org = VGA_ADDR,   len = VGA_SIZE
    fbr(rw)     :   org = FBR_ADDR,   len = FBR_SIZE
    upper(rx)   :   org = UPPR_ADDR,  len = UPPR_SIZE
}

SECTIONS {
    . = BIOS_ADDR;
    __bios_start = .;
    . = BIOS_SIZE;
    __bios_end = .;
    . = BOOT_ADDR;
    __bootloader_start = .;
    . = BOOT_ADDR;
    .boot (NOLOAD) : {
      *(.boot) 
      *(.boot.util)
      *(.boot.rodata)
    } > boot
    . = MBR_ADDR;
    .mbr (NOLOAD) : { *(.mbr) } > mbr
    . = MEM1_ADDR;
    .stage15 (NOLOAD) : { *(.stage15) } > mem1
    . = ST2_ADDR;
    stage2.o = .;
    ._init : { *(._init) } > stage2
    .text : { *(.text) } > stage2
    .data : { *(.data) } > stage2
    .rodata : { *(.rodata) } > stage2
    .elf (NOLOAD) : { *(.elf) } > elf
    .bss (NOLOAD) : { *(.bss) } > seg
    __bootloader_end = .;
    . = IDT_ADDR;
    .idt (NOLOAD) : { 
        __IDT_PTR_LOCATION = .;
        __IDT_BASE_LOCATION = . + 8; 
        KEEP(*(.idt) )
      } > idt
    .mmap (NOLOAD) : {
        KEEP(*(.mmap) )
    } > mmap
    .pmm (NOLOAD) : {
        __MMAP_HEAP_LOCATION = .;
        KEEP(*(.pmm) )
    } > mem2 
    .fbr (NOLOAD) : {
        __FB_ADDR = .;
        KEEP(*(.fbr) )
    } > fbr
    . = FBR_ADDR + FBR_SIZE;
    __FB_END = .;
    .upper (NOLOAD) : {
        __upper_start = .;
        KEEP(*(.fbr) )
    } > upper
    . = UPPR_ADDR + UPPR_SIZE;
    __upper_end = .;
}
