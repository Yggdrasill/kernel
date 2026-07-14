INCLUDE bootdefs.ld
OUTPUT_FORMAT(binary);

SECTIONS {
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
