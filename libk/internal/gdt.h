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

#ifndef GDT_INTERNAL_H
#define GDT_INTERNAL_H

#define GDT_MAX_ENTRIES 8192

#if !defined(LD_BOOT_STAGE1) && !defined(LD_BOOT_STAGE2)

    #include <stddef.h>
    #include <stdint.h>

struct gdt_ptr {
    uint8_t size_0;
    uint8_t size_8;
    uint8_t base_0;
    uint8_t base_8;
    uint8_t base_16;
    uint8_t base_24;
};

struct gdt_entry {
    uint8_t limit_0;
    uint8_t limit_8;
    uint8_t base_0;
    uint8_t base_8;
    uint8_t base_16;
    uint8_t access;
    uint8_t limit_flags;
    uint8_t base_24;
};

struct gdt_info {
    struct gdt_ptr   *gdtr;
    struct gdt_entry *entries;
    size_t            nr_entries;
    size_t            max_nr_entries;
};

struct gdt_entry;

    #define GDT_PTR_SIZE   (sizeof(struct gdt_ptr))
    #define GDT_ENTRY_SIZE (sizeof(struct gdt_entry))

#endif

#endif
