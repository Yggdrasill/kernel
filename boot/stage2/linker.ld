INCLUDE bootdefs.ld
OUTPUT_FORMAT(elf32-i386);
ENTRY(__start);

PHDRS {
	P_NOLOAD PT_NULL;
	P_STAGE2 PT_LOAD;
}

SECTIONS {
	. = BOOT_ADDR;
	__BOOT_ADDR = .;
	__BOOTLOADER_START = .;
	.boot (NOLOAD) : {
		*(.boot)
		*(.boot.util)
		*(.boot.rodata)
	} > boot : P_NOLOAD
	__BOOT_SIZE = BOOT_ADDR - ENTR_ADDR;
	. = MBR_ADDR;
	.mbr (NOLOAD) : {
		*(.mbr)
	} > mbr : P_NOLOAD

	. = MEM1_ADDR;
	.stage15 (NOLOAD) : {
		__STAGE15_LOAD_ADDR = .;
		*(.stage15)
		*(.stage15.data)
		*(.stage15.rodata)
		*(.stage15.bss)
	} > mem1 : P_NOLOAD

	. = ST2_ADDR;
	stage2.o = .;
	._init : { *(._init) } > stage2 : P_STAGE2
	.text : { *(.text) } > stage2 : P_STAGE2
	.data : { *(.data) } > stage2 : P_STAGE2
	.rodata : { *(.rodata) } > stage2 : P_STAGE2
	.bss (NOLOAD) : { *(.bss) } > stage2 : P_STAGE2
	.elf (NOLOAD) : { *(.elf) } > elf : P_NOLOAD
	__BOOTLOADER_END = .;

	. = GDTR_ADDR;
	__GDTR_START = .;
	.gdtr (NOLOAD) : {
		__GDT_PTR_LOCATION = .;
		KEEP(*(.gdtr) )
	} > gdtr : P_NOLOAD
	__GDTR_END = GDTR_ADDR + GDTR_SIZE;
	. = GDT_ADDR;
	__GDT_START = .;
	.gdt (NOLOAD) : {
		__GDT_BASE_LOCATION = .;
		KEEP(*(.gdt) )
	} > gdt : P_NOLOAD
	__GDT_END = GDT_ADDR + GDT_SIZE;

	. = MMAP_ADDR;
	.mmap (NOLOAD) : {
		__MMAP_BASE_ADDR = .;
		KEEP(*(.mmap) )
	} > mmap : P_NOLOAD

	. = IDT_ADDR;
	__IDT_START = .;
	.idt (NOLOAD) : {
		__IDT_PTR_LOCATION = .;
		__IDT_BASE_LOCATION = . + 8;
		KEEP(*(.idt) )
	} > idt : P_NOLOAD
	__IDT_END = IDT_ADDR + IDT_SIZE;

	. = MEM2_ADDR;
	.pmm (NOLOAD) : {
		__MMAP_HEAP_LOCATION = .;
		KEEP(*(.pmm) )
	} > mem2 : P_NOLOAD

	. = STACK_ADDR;
	__STACK_START = .;
	.stack (NOLOAD) : {
		__STACK_BASE_LOCATION = (. + 64K - 0x10);
	} > stack : P_NOLOAD
	__STACK_END = STACK_ADDR + STACK_SIZE;

	. = FBR_ADDR;
	.fbr (NOLOAD) : {
		__FB_ADDR = .;
		KEEP(*(.fbr) )
	} > fbr : P_NOLOAD
	. = FBR_ADDR + FBR_SIZE;
	__FB_END = .;
	.upper (NOLOAD) : {
		__UPPER_START = .;
		KEEP(*(.fbr) )
	} > upper : P_NOLOAD
	. = UPPR_ADDR + UPPR_SIZE;
	__UPPER_END = .;
}
