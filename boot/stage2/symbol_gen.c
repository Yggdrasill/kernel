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

#include "pmm.h"

#include <libk/gdt.h>
#include <libk/idt.h>
#include <libk/mmap.h>
#include <stddef.h>
#include <stdint.h>

#define SYMBOL_ALIGN (1U << 12)
#define SYMBOL_MASK  (~(SYMBOL_ALIGN - 1))
#define ALIGN_END(x) (((x) + SYMBOL_ALIGN - 1) & SYMBOL_MASK)

static const struct pmm_bitmap pmm_bitmap_sym;

/* Export symbols to ELF file, to be used in assembly and linker scripts. */

volatile const uint32_t ABI_GDT_PTR_SIZE   = GDT_PTR_SIZE;
volatile const uint32_t ABI_GDT_ENTRY_SIZE = GDT_ENTRY_SIZE;

volatile const uint32_t ABI_IDT_PTR_SIZE   = IDT_PTR_SIZE;
volatile const uint32_t ABI_IDT_ENTRY_SIZE = IDT_ENTRY_SIZE;

volatile const uint32_t ABI_MMAP_ENTRY_SIZE = MMAP_ENTRY_SIZE;
volatile const uint32_t ABI_MMAP_TABLE_SIZE =
    MMAP_MAX_ENTRIES * MMAP_ENTRY_SIZE;
volatile const uint32_t ABI_MMAP_INFO_BASE = INFO_BASE_OFFSET;
volatile const uint32_t ABI_MMAP_INFO_NR   = INFO_NR_ENT_OFFSET;
volatile const uint32_t ABI_MMAP_INFO_MAX  = INFO_MAX_NR_OFFSET;

volatile const uint32_t ABI_PMM_BM_PREALLOC =
    PMM_INIT_ENTRIES * sizeof(*pmm_bitmap_sym.bitmap);

volatile const uint32_t ABI_LINK_TIME_TOTAL =
    ALIGN_END(GDT_ENTRY_SIZE * GDT_MAX_ENTRIES) +
    ALIGN_END(IDT_ENTRY_SIZE * IDT_MAX_ENTRIES) +
    ALIGN_END(2 * MMAP_MAX_ENTRIES * MMAP_ENTRY_SIZE) +
    ALIGN_END(PMM_INIT_ENTRIES * sizeof(pmm_bitmap_sym.bitmap));

int main(void)
{
    return 0;
}
