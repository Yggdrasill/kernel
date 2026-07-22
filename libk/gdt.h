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

#ifndef GDT_H
#define GDT_H

#include <stddef.h>
#include <stdint.h>

enum GDT_ACCESS {
    GDT_ACCESSED   = 1,
    GDT_RW         = 1 << 1,
    GDT_DC         = 1 << 2,
    GDT_EXEC       = 1 << 3,
    GDT_SEGMENT    = 1 << 4,
    GDT_PRIV_RING0 = 0 << 5,
    GDT_PRIV_RING1 = 1 << 5,
    GDT_PRIV_RING2 = 2 << 5,
    GDT_PRIV_RING3 = 3 << 5,
    GDT_PRESENT    = 1 << 7
};

enum GDT_FLAGS {
    GDT_RESERVED = 0,
    GDT_LONG     = 1 << 1,
    GDT_BITS_32  = 1 << 2,
    GDT_GRAN     = 1 << 3
};

struct gdt_ptr;
struct gdt_entry;
struct gdt_info;

extern void gdt_install(struct gdt_info *);

void gdt_init(struct gdt_info *);
int  gdt_entry_add(struct gdt_info *, void *, uint32_t, uint8_t, uint8_t);

#endif
