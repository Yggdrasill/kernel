OUTPUT_FORMAT(elf32-i386);
ENTRY(__start);

SECTIONS {
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
