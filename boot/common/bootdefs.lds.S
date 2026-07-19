#include <libk/internal/gdt.h>
#include <libk/internal/idt.h>
#include <libk/internal/mmap.h>

#ifdef LD_BOOT_STAGE1
OUTPUT_FORMAT(binary);
#elif  LD_BOOT_STAGE2
#include <build/generated/mmap_generated.h>
OUTPUT_FORMAT(elf32-i386);
ENTRY(__start);
#endif

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

PHDRS {
	P_NOLOAD PT_NULL;
	P_STAGE1 PT_LOAD;
	P_STAGE2 PT_LOAD;
}

SECTIONS {
	. = BIOS_ADDR;
	__BIOS_START = .;
	. = BIOS_SIZE;
	__BIOS_END = .;

	. = ENTR_ADDR;
	__BOOT_ENTRY = .;

	. = BOOT_ADDR;
	__BOOT_ADDR = .;
	__BOOTLOADER_START = .;
#ifdef LD_BOOT_STAGE1
	.boot : {
		*(.boot)
		*(.boot.util)
		*(.boot.rodata)
	} > boot : P_STAGE1
	PROVIDE(__BOOT_SIZE = BOOT_ADDR - ENTR_ADDR);
	. = MBR_ADDR;
	.mbr : {
		*(.mbr)
	} > mbr : P_STAGE1

	. = MEM1_ADDR;
	.stage15 : {
		__STAGE15_LOAD_ADDR = ABSOLUTE(.);
		__STAGE15_LOAD_SEG = ABSOLUTE((__STAGE15_LOAD_ADDR >> 4) & 0xF000);
		__STAGE15_LOAD_OFF = ABSOLUTE(__STAGE15_LOAD_ADDR & 0xFFFF);
		*(.stage15)
		*(.stage15.data)
		*(.stage15.rodata)
		*(.stage15.bss)
	} > mem1 : P_NOLOAD

	. = ST2_ADDR;
	.stage2.bss (NOLOAD) : {
		*(.stage2.bss)
	} > stage2
	.elf (NOLOAD) : {
		*(.elf)
	} > elf : P_NOLOAD
#elif LD_BOOT_STAGE2 
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
#endif
	__BOOTLOADER_END = .;
#if LD_BOOT_STAGE2
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
		__mmap_old_map = .;
		. += MMAP_TABLE_SIZE;
		__mmap_new_map = .;
		. += MMAP_TABLE_SIZE;
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
#endif
}
