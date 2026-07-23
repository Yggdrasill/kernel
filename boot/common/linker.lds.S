/*
 * MBR bootloader, currently unnamed
 * Copyright (C) 2017  Yggdrasill <kaymeerah@lambda.is>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA.
 *
 */

#include <libk/internal/gdt.h>
#include <libk/internal/idt.h>
#include <libk/internal/mmap.h>

#ifdef LD_BOOT_STAGE1
#define STAGE1_PHDR   P_STAGE1
#define STAGE2_PHDR   NONE
#define STAGE1_NOLOAD
#define STAGE2_NOLOAD (NOLOAD)
OUTPUT_FORMAT(binary);
#elif  LD_BOOT_STAGE2
#include <build/generated/mmap_generated.h>
#define STAGE1_PHDR   NONE
#define STAGE2_PHDR   P_STAGE2
#define STAGE1_NOLOAD (NOLOAD)
#define STAGE2_NOLOAD
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
#ifdef LD_BOOT_STAGE2
PRE_ADDR   = 0x17800;
PRE_SIZE   = (0x18000 - PRE_ADDR) + ABI_LINK_TIME_TOTAL;
/*
GDTR_ADDR  = 0x17800;
GDTR_SIZE  = 0x6;
GDT_ADDR   = 0x18000;
GDT_SIZE   = 64K;
MMAP_ADDR  = 0x28000;
MMAP_SIZE  = 64K;
IDT_ADDR   = 0x38000;
IDT_SIZE   = 4K;
*/
MEM2_ADDR  = PRE_ADDR + PRE_SIZE;
#endif
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
    bios(rx)     :   org = BIOS_ADDR,  len = BIOS_SIZE
    mem0(rw)     :   org = MEM0_ADDR,  len = MEM0_SIZE
    entry(rw)    :   org = ENTR_ADDR,  len = ENTR_SIZE
    boot(rw)     :   org = BOOT_ADDR,  len = BOOT_SIZE
    mbr(rw)      :   org = MBR_ADDR,   len = MBR_SIZE
    mem1(rx)     :   org = MEM1_ADDR,  len = MEM1_SIZE
    stage2(rx)   :   org = ST2_ADDR,   len = ST2_SIZE
    elf(rw)      :   org = ELF_ADDR,   len = ELF_SIZE
#ifdef LD_BOOT_STAGE2
    prealloc(rw) :   org = PRE_ADDR,   len = PRE_SIZE
/*
    gdt(rw)      :   org = GDT_ADDR,   len = GDT_SIZE
    idt(rw)      :   org = IDT_ADDR,   len = IDT_SIZE
    mmap(rw)     :   org = MMAP_ADDR,  len = MMAP_SIZE
 */
    mem2(rx)     :   org = MEM2_ADDR,  len = STACK_ADDR - (PRE_ADDR + PRE_SIZE)
#endif
    stack(rw)    :   org = STACK_ADDR, len = STACK_SIZE
    ebda(rx)     :   org = EBDA_ADDR,  len = EBDA_SIZE
    vga(rw)      :   org = VGA_ADDR,   len = VGA_SIZE
    fbr(rw)      :   org = FBR_ADDR,   len = FBR_SIZE
    upper(rx)    :   org = UPPR_ADDR,  len = UPPR_SIZE
}

PHDRS {
#ifdef LD_BOOT_STAGE1
    P_STAGE1 PT_LOAD;
#elif defined(LD_BOOT_STAGE2)
    P_STAGE2 PT_LOAD;
#endif
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
    .boot STAGE1_NOLOAD : {
        KEEP(*(.boot))
        KEEP(*(.boot.util))
        KEEP(*(.boot.rodata))
    } > boot : STAGE1_PHDR
    PROVIDE(__BOOT_SIZE = BOOT_ADDR - ENTR_ADDR);
    . = MBR_ADDR;
    .mbr STAGE1_NOLOAD : {
        KEEP(*(.mbr))
    } > mbr : STAGE1_PHDR

    . = MEM1_ADDR;
    .stage15 STAGE1_NOLOAD : {
        __STAGE15_LOAD_ADDR = ABSOLUTE(.);
        __STAGE15_LOAD_SEG = ABSOLUTE((__STAGE15_LOAD_ADDR >> 4) & 0xF000);
        __STAGE15_LOAD_OFF = ABSOLUTE(__STAGE15_LOAD_ADDR & 0xFFFF);
        KEEP(*(.stage15))
        KEEP(*(.stage15.data))
        KEEP(*(.stage15.rodata))
        KEEP(*(.stage15.bss))
    } > mem1 : STAGE1_PHDR

    . = ST2_ADDR;
#ifdef LD_BOOT_STAGE2
    stage2.o = .;
    /DISCARD/ : {
        *(.elf_init)
    }
#endif
    ._init STAGE2_NOLOAD : {
#ifdef LD_BOOT_STAGE1
        *(.elf_init)
#endif
        *(._init)
    } > stage2 : STAGE2_PHDR
    .text STAGE2_NOLOAD : {
        *(.text)
        *(.text.*)
    } > stage2 : STAGE2_PHDR
    .data STAGE2_NOLOAD : {
        *(.data)
        *(.data.*)
    } > stage2 : STAGE2_PHDR
    .rodata STAGE2_NOLOAD : {
        *(.rodata)
        *(.rodata.*)
    } > stage2 : STAGE2_PHDR
    .bss (NOLOAD) : {
        *(.bss)
        *(.bss.*)
    } > stage2 : STAGE2_PHDR
    . = ELF_ADDR;
    .elf (NOLOAD) : {
        *(.elf)
    } > elf : NONE
    __BOOTLOADER_END = .;
#ifdef LD_BOOT_STAGE2
    . = PRE_ADDR;
    __PREALLOC_START = .;
    .regs (NOLOAD) : {
        __GDTR_DATA = .;
        . += ABI_GDT_PTR_SIZE;
        __IDTR_DATA = .; 
        . += ABI_IDT_PTR_SIZE;
    } > prealloc : NONE
    .gdt ALIGN(0x1000) (NOLOAD) : {
        __GDT_ENTRIES = .;
    } > prealloc : NONE

    . += ABI_GDT_ENTRY_SIZE * GDT_MAX_ENTRIES;
    .idt ALIGN(0x1000) (NOLOAD) : {
        __IDT_ENTRIES = .;
    } > prealloc : NONE

    . += ABI_IDT_ENTRY_SIZE * IDT_MAX_ENTRIES; 
    .mmap ALIGN(0x1000) (NOLOAD) : {
        __mmap_old_map = .;
        . += ABI_MMAP_TABLE_SIZE;
        __mmap_new_map = .;
        . += ABI_MMAP_TABLE_SIZE;
    } > prealloc : NONE

    .pmm ALIGN(0x1000) (NOLOAD) : {
        pmm_initial = .;
    } > mem2 : NONE
    . += ABI_PMM_BM_PREALLOC;
    __PREALLOC_END = .;

    . = STACK_ADDR;
    __STACK_START = .;
    .stack (NOLOAD) : {
        __STACK_BASE_LOCATION = (. + 64K - 0x10);
    } > stack : NONE
    __STACK_END = STACK_ADDR + STACK_SIZE;

    . = FBR_ADDR;
    .fbr (NOLOAD) : {
        __FB_ADDR = .;
    } > fbr : NONE
    . = FBR_ADDR + FBR_SIZE;
    __FB_END = .;
    .upper (NOLOAD) : {
        __UPPER_START = .;
    } > upper : NONE
    . = UPPR_ADDR + UPPR_SIZE;
    __UPPER_END = .;
#endif
}
