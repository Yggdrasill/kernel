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

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <libk/gdt.h>
#include <libk/internal/gdt.h>
#include <libk/util.h>

#define GDT_DEFAULT_CODE 0x08
#define GDT_DEFAULT_DATA 0x010

extern void gdt_segment_select(uint16_t code, uint16_t data);

int gdt_entry_add(
    struct gdt_info *info,
    void            *base,
    uint32_t         limit,
    uint8_t          access,
    uint8_t          flags)
{
    struct gdt_ptr   *gdtr;
    struct gdt_entry *entry;
    uintptr_t         gdt_base_addr;
    size_t            size;
    size_t            index;

    if(limit > 0xFFFFF) return -1;
    if(info->nr_entries + 1 > GDT_MAX_ENTRIES) return -2;

    gdtr = info->gdtr;

    gdt_base_addr = (uintptr_t)gdtr->base_24 << 24 |
                    (uintptr_t)gdtr->base_16 << 16 |
                    (uintptr_t)gdtr->base_8 << 8 | (uintptr_t)gdtr->base_0;

    entry = (struct gdt_entry *)gdt_base_addr;
    size  = (((uint16_t)gdtr->size_8 << 8) | ((uint16_t)gdtr->size_0)) + 1;

    index = size / sizeof(*entry);
    entry += index;

    entry->access  = access;
    entry->base_0  = ((unsigned char *)&base)[0];
    entry->base_8  = ((unsigned char *)&base)[1];
    entry->base_16 = ((unsigned char *)&base)[2];
    entry->base_24 = ((unsigned char *)&base)[3];
    entry->limit_0 = ((unsigned char *)&limit)[0];
    entry->limit_8 = ((unsigned char *)&limit)[1];
    entry->limit_flags =
        (flags & 0x0F) << 4 | (((unsigned char *)&limit)[2] & 0x0F);

    size         = sizeof(*entry) * (index + 1) - 1;
    gdtr->size_0 = ((unsigned char *)&size)[0];
    gdtr->size_8 = ((unsigned char *)&size)[1];

    info->nr_entries = index + 1;

    return 0;
}

int gdt_default_entries_add(struct gdt_info *info)
{
    uint8_t access;
    uint8_t flags;
    int     rv;

    access = GDT_PRESENT | GDT_SEGMENT | GDT_RW | GDT_ACCESSED;
    flags  = GDT_GRAN | GDT_BITS_32;

    rv = gdt_entry_add(info, 0, 0xFFFFF, access | GDT_EXEC, flags);
    if(rv) goto default_fail;
    rv = gdt_entry_add(info, 0, 0xFFFFF, access, flags);

default_fail:
    return rv;
}

void gdt_init(struct gdt_info *info)
{
    struct gdt_ptr   *gdtr;
    struct gdt_entry *base;
    uint16_t          gdt_size;

    if(!info) panic("GDT: NULL pointer argument!");

    gdtr = info->gdtr;
    base = info->entries;

    /* NULL GDT entry */
    gdt_size = sizeof(*base) - 1;
    info->nr_entries++;

    gdtr->size_0  = ((unsigned char *)&gdt_size)[0];
    gdtr->size_8  = ((unsigned char *)&gdt_size)[1];
    gdtr->base_0  = ((unsigned char *)&base)[0];
    gdtr->base_8  = ((unsigned char *)&base)[1];
    gdtr->base_16 = ((unsigned char *)&base)[2];
    gdtr->base_24 = ((unsigned char *)&base)[3];

    /* Setup of NULL GDT entry */
    memset(base, 0, sizeof(*base));
    gdt_default_entries_add(info);
    gdt_install(info);
    gdt_segment_select(GDT_DEFAULT_CODE, GDT_DEFAULT_DATA);
}
