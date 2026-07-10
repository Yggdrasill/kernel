OUTPUT_FORMAT(binary);

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
ST2_SIZE  = 30K;
ELF_ADDR  = 0x10000;
ELF_SIZE  = 30K;

MEMORY {
	entry(rw)   :   org = ENTR_ADDR,  len = ENTR_SIZE
	boot(rwx)   :   org = BOOT_ADDR,  len = BOOT_SIZE
	mbr(rw)	 :   org = MBR_ADDR,   len = MBR_SIZE
	mem1(rwx)   :   org = MEM1_ADDR,  len = MEM1_SIZE
	stage2(rwx) :   org = ST2_ADDR,   len = ST2_SIZE
	elf(rw)	 :   org = ELF_ADDR,   len = ELF_SIZE
}

SECTIONS {
	. = BIOS_ADDR;
	__bios_start = .;
	. = BIOS_SIZE;
	__bios_end = .;

	. = ENTR_ADDR;
	__BOOT_ENTRY = .;
	. = BOOT_ADDR;
	__BOOT_ADDR = .;
	__bootloader_start = .;
	.boot : {
		*(.boot)
		*(.boot.util)
		*(.boot.rodata)
	} > boot
	PROVIDE(__BOOT_SIZE = BOOT_ADDR - ENTR_ADDR);
	. = MBR_ADDR;
	.mbr : { *(.mbr) } > mbr

	. = MEM1_ADDR;
	.stage15 : {
		__STAGE15_LOAD_ADDR = ABSOLUTE(.);
		__STAGE15_LOAD_SEG = ABSOLUTE((__STAGE15_LOAD_ADDR >> 4) & 0xF000);
		__STAGE15_LOAD_OFF = ABSOLUTE(__STAGE15_LOAD_ADDR & 0xFFFF);
		*(.stage15)
		*(.stage15.data)
		*(.stage15.rodata)
	} > mem1
	.stage15.bss (NOLOAD) : {
		*(.stage15.bss)
	} > mem1
	. = ST2_ADDR;
	.stage2.bss (NOLOAD) : {
		*(.stage2.bss)
	} > stage2
	.elf (NOLOAD) : { *(.elf) } > elf
}
