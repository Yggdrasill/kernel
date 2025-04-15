OUTPUT_FORMAT(binary);

BOOT_ADDR = 0x7E00;
BOOT_SIZE = 0x200;
MEM1_ADDR = 0x8000;
MEM1_SIZE = 0x400;
ST2_ADDR  = 0x8400;
ST2_SIZE  = 28K;

MEMORY {
    boot(rwx)   :   org = BOOT_ADDR,  len = BOOT_SIZE
    mem1(rwx)   :   org = MEM1_ADDR,  len = MEM1_SIZE
    stage2(rwx) :   org = ST2_ADDR,   len = ST2_SIZE
}

SECTIONS {
    . = BOOT_ADDR;
    .boot : { *(.boot) } > boot
    . = MEM1_ADDR;
    .stage15 : { *(.stage15) } > mem1
    . = ST2_ADDR;
    .stage2 (NOLOAD) : { *(.bss) } > stage2
}
