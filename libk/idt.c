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

#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <libk/idt.h>
#include <libk/internal/idt.h>

#define IDT_BITMAP_NR(x) (IDT_ENTRY_NUM / (sizeof(x) * CHAR_BIT))

/*
 * Little endian byte alignment follows. These structures are manually packed
 * because it avoids using compiler pragmas. I do not know of any x86 compiler
 * that would pad a struct that contains only byte-aligned types.
 *
 * Both of these structs will be 8-byte aligned manually.
 */

struct idt_ptr {
    unsigned char limit_0;
    unsigned char limit_8;
    unsigned char base_0;
    unsigned char base_8;
    unsigned char base_16;
    unsigned char base_24;
};

struct idt_entry {
    unsigned char offset_0;
    unsigned char offset_8;
    unsigned char selector_0;
    unsigned char selector_8;
    unsigned char zero;
    unsigned char flags;
    unsigned char offset_16;
    unsigned char offset_24;
};

struct idt_info {
    struct idt_ptr   *idtr;
    struct idt_entry *entries;
    size_t            max_nr_entries;
    size_t            nr_entries;
    uint32_t          present[IDT_BITMAP_NR(uint32_t)];
};

/*
 * This global state is a bootloader-only construct and will not be allowed
 * within the actual kernel.
 */

extern struct idt_ptr   __IDT_PTR_LOCATION;
extern struct idt_entry __IDT_BASE_LOCATION;
static struct idt_info  idt_info;

struct idt_info *idt_info_init(void)
{
    idt_info = (struct idt_info){
        .idtr           = &__IDT_PTR_LOCATION,
        .entries        = &__IDT_BASE_LOCATION,
        .max_nr_entries = IDT_ENTRY_NUM,
        .nr_entries     = 0,
    };
    return &idt_info;
}

struct idt_info *idt_init(void)
{
    struct idt_ptr   *idtr;
    struct idt_entry *base;
    struct idt_info  *info;
    unsigned char    *arr_limit;
    unsigned char    *arr_base;

    uint16_t limit;

    info = idt_info_init();

    idtr  = info->idtr;
    limit = sizeof(struct idt_entry) * IDT_ENTRY_NUM - 1;
    base  = info->entries;

    arr_limit = (unsigned char *)&limit;
    arr_base  = (unsigned char *)&base;

    idtr->limit_0 = arr_limit[0];
    idtr->limit_8 = arr_limit[1];

    idtr->base_0  = arr_base[0];
    idtr->base_8  = arr_base[1];
    idtr->base_16 = arr_base[2];
    idtr->base_24 = arr_base[3];

    memset(info->present, 0, sizeof(info->present));
    idt_install(info);

    return info;
}

size_t idt_entries_nr(struct idt_info *info)
{
    return info->nr_entries;
}

size_t idt_entries_max(struct idt_info *info)
{
    return info->max_nr_entries;
}

size_t idt_entry_set(
    struct idt_info *info,
    void             (*idt_handler)(void),
    uint16_t         select,
    uint8_t          flags,
    uint8_t          at_offset)
{
    struct idt_entry *entry;
    unsigned char    *offset;
    unsigned char    *selector;
    intptr_t          raw_ptr;
    size_t            nr_entries;
    uint32_t          bitmap;
    uint8_t           index;
    uint8_t           bit;

    nr_entries = info->nr_entries;
    index      = at_offset / (sizeof(info->present[0]) * CHAR_BIT);
    bit        = at_offset % (sizeof(info->present[0]) * CHAR_BIT);
    bitmap     = info->present[index] & (1U << bit);

    if(!bitmap) nr_entries++;
    if(nr_entries > IDT_ENTRY_NUM) goto idt_set_exit;
    info->present[index] |= 1U << bit;

    entry    = info->entries + at_offset;
    raw_ptr  = (intptr_t)idt_handler;
    offset   = (unsigned char *)&raw_ptr;
    selector = (unsigned char *)&select;

    entry->offset_0  = offset[0];
    entry->offset_8  = offset[1];
    entry->offset_16 = offset[2];
    entry->offset_24 = offset[3];

    entry->selector_0 = selector[0];
    entry->selector_8 = selector[1];

    entry->zero  = 0;
    entry->flags = flags;

    info->nr_entries = nr_entries;
idt_set_exit:
    return info->nr_entries;
}

size_t idt_entry_add(
    struct idt_info *info,
    void             (*idt_handler)(void),
    uint16_t         select,
    uint8_t          flags)
{
    return idt_entry_set(info, idt_handler, select, flags, info->nr_entries);
}
