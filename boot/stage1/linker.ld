OUTPUT_FORMAT(binary);

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

MEMORY {
    entry(rw)   :   org = ENTR_ADDR,  len = ENTR_SIZE
    boot(rwx)   :   org = BOOT_ADDR,  len = BOOT_SIZE
    mbr(rw)     :   org = MBR_ADDR,   len = MBR_SIZE
    mem1(rwx)   :   org = MEM1_ADDR,  len = MEM1_SIZE
    stage2(rwx) :   org = ST2_ADDR,   len = ST2_SIZE
    elf(rw)     :   org = ELF_ADDR,   len = ELF_SIZE
    seg(rw)     :   org = SEG_ADDR,   len = SEG_SIZE
}

SECTIONS {
    . = BOOT_ADDR;
    .boot : {
      *(.boot) 
      *(.boot.util)
    } > boot
    . = MBR_ADDR;
    .mbr : { *(.mbr) } > mbr
    . = MEM1_ADDR;
    .stage15 : { *(.stage15) } > mem1
    . = ST2_ADDR;
    .stage2.bss (NOLOAD) : { *(.stage2.bss) } > stage2
    .elf (NOLOAD) : { *(.elf) } > elf
    .seg (NOLOAD) : { *(.seg) } > seg
}
