OUTPUT_FORMAT(elf32-i386);
ENTRY(__start);

BIOS_ADDR  = 0x0;
BIOS_SIZE  = 0x500;
MEM0_ADDR  = 0x500;
MEM0_SIZE  = 0x7700;
ENTR_ADDR  = 0x7C00;
ENTR_SIZE  = 0x200;
BOOT_ADDR  = 0x7E00;
BOOT_SIZE  = 0x1BE;
MBR_ADDR   = 0x7FBE;
MBR_SIZE   = 0x42;
MEM1_ADDR  = 0x8000;
MEM1_SIZE  = 0x600;
ST2_ADDR   = 0x8600;
ST2_SIZE   = 30K;
ELF_ADDR   = 0x10000;
ELF_SIZE   = 30K;
GDTR_ADDR  = 0x17800;
GDTR_SIZE  = 0x6;
GDT_ADDR   = 0x18000;
GDT_SIZE   = 64K;
MMAP_ADDR  = 0x28000;
MMAP_SIZE  = 64K;
IDT_ADDR   = 0x38000;
IDT_SIZE   = 4K;
MEM2_ADDR  = 0x39000;
MEM2_SIZE  = 220K;
STACK_ADDR = 0x70000;
STACK_SIZE = 64K;
EBDA_ADDR  = 0x80000;
EBDA_SIZE  = 128K;
VGA_ADDR   = 0xA0000;
VGA_SIZE   = 96K;
FBR_ADDR   = 0xB8000;
FBR_SIZE   = 32K;
UPPR_ADDR  = 0xC0000;
UPPR_SIZE  = 256K;

MEMORY {
	bios(rx)   :   org = BIOS_ADDR,  len = BIOS_SIZE
	mem0(rw)   :   org = MEM0_ADDR,  len = MEM0_SIZE
	entry(rw)  :   org = ENTR_ADDR,  len = ENTR_SIZE
	boot(rw)   :   org = BOOT_ADDR,  len = BOOT_SIZE
	mbr(rw)    :   org = MBR_ADDR,   len = MBR_SIZE
	mem1(rx)   :   org = MEM1_ADDR,  len = MEM1_SIZE
	stage2(rx) :   org = ST2_ADDR,   len = ST2_SIZE
	elf(rw)    :   org = ELF_ADDR,   len = ELF_SIZE
	gdtr(rw)   :   org = GDTR_ADDR,  len = GDTR_SIZE
	gdt(rw)    :   org = GDT_ADDR,   len = GDT_SIZE
	idt(rw)    :   org = IDT_ADDR,   len = IDT_SIZE
	mmap(rw)   :   org = MMAP_ADDR,  len = MMAP_SIZE
	mem2(rx)   :   org = MEM2_ADDR,  len = MEM2_SIZE
	stack(rw)  :   org = STACK_ADDR, len = STACK_SIZE
	ebda(rx)   :   org = EBDA_ADDR,  len = EBDA_SIZE
	vga(rw)    :   org = VGA_ADDR,   len = VGA_SIZE
	fbr(rw)    :   org = FBR_ADDR,   len = FBR_SIZE
	upper(rx)  :   org = UPPR_ADDR,  len = UPPR_SIZE
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
	.boot (NOLOAD) : {
		*(.boot)
		*(.boot.util)
		*(.boot.rodata)
	} > boot
	PROVIDE(__BOOT_SIZE = BOOT_ADDR - ENTR_ADDR);
	. = MBR_ADDR;
	.mbr (NOLOAD) : { *(.mbr) } > mbr

	. = MEM1_ADDR;
	.stage15 (NOLOAD) : {
		__STAGE15_LOAD_ADDR = .;
		*(.stage15)
	} > mem1

	. = ST2_ADDR;
	stage2.o = .;
	._init : { *(._init) } > stage2
	.text : { *(.text) } > stage2
	.data : { *(.data) } > stage2
	.rodata : { *(.rodata) } > stage2
	.bss (NOLOAD) : { *(.bss) } > stage2
	.elf (NOLOAD) : { *(.elf) } > elf
	__bootloader_end = .;
	. = GDTR_ADDR;
	.gdtr (NOLOAD) : {
		__GDT_PTR_LOCATION = .;
		KEEP(*(.gdtr) )
	} > gdtr
	. = GDT_ADDR;
	.gdt (NOLOAD) : {
		__GDT_BASE_LOCATION = .;
		KEEP(*(.gdt) )
	} > gdt
	. = MMAP_ADDR;
	.mmap (NOLOAD) : {
		__MMAP_BASE_ADDR = .;
		KEEP(*(.mmap) )
	} > mmap
	. = IDT_ADDR;
	.idt (NOLOAD) : {
		__IDT_PTR_LOCATION = .;
		__IDT_BASE_LOCATION = . + 8;
		KEEP(*(.idt) )
	  } > idt
	. = MEM2_ADDR;
	.pmm (NOLOAD) : {
		__MMAP_HEAP_LOCATION = .;
		KEEP(*(.pmm) )
	} > mem2
	. = STACK_ADDR;
	.stack (NOLOAD) : {
		__STACK_BASE_LOCATION = (. + 64K - 0x10);
	} > stack
	. = FBR_ADDR;
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
