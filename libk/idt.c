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

#include <libk/idt.h>
#include <libk/irq.h>

extern void exception_unknown(void);
extern void exception_0x00(void);
extern void exception_0x01(void);
extern void exception_0x02(void);
extern void exception_0x03(void);
extern void exception_0x04(void);
extern void exception_0x05(void);
extern void exception_0x06(void);
extern void exception_0x07(void);
extern void exception_0x08(void);
extern void exception_0x09(void);
extern void exception_0x0A(void);
extern void exception_0x0B(void);
extern void exception_0x0C(void);
extern void exception_0x0D(void);
extern void exception_0x0E(void);
extern void exception_0x10(void);
extern void exception_0x11(void);
extern void exception_0x12(void);
extern void exception_0x13(void);
extern void exception_0x14(void);
extern void exception_0x15(void);

extern void irq_0x00(void);
extern void irq_0x01(void);
extern void irq_0x02(void);
extern void irq_0x03(void);
extern void irq_0x04(void);
extern void irq_0x05(void);
extern void irq_0x06(void);
extern void irq_0x07(void);
extern void irq_0x08(void);
extern void irq_0x09(void);
extern void irq_0x0A(void);
extern void irq_0x0B(void);
extern void irq_0x0C(void);
extern void irq_0x0D(void);
extern void irq_0x0E(void);
extern void irq_0x0F(void);
extern void irq_wrapper(void);

void idt_init(struct idt_info *info)
{
    struct idt_ptr   *idtr;
    struct idt_entry *base;
    unsigned char    *arr_limit;
    unsigned char    *arr_base;

    uint16_t limit;

    idtr  = info->idtr;
    limit = sizeof(struct idt_entry) * IDT_MAX_ENTRIES - 1;
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

    while(!idt_entry_add(info, &exception_unknown, 0x08, 0x8E));

    idt_install(info);
    return;
}

size_t idt_entries_nr(struct idt_info *info)
{
    return info->nr_entries;
}

size_t idt_entries_max(struct idt_info *info)
{
    return info->max_nr_entries;
}

/* TODO: Don't uncritically accept input and overwrite in-use entries. */

int idt_entry_set(
    struct idt_info *info,
    void             (*idt_handler)(void),
    size_t           at_offset,
    uint16_t         select,
    uint8_t          flags)
{
    struct idt_entry *entry;
    unsigned char    *offset;
    unsigned char    *selector;
    intptr_t          raw_ptr;
    size_t            nr_entries;
    size_t            index;
    size_t            bit;
    uint32_t          bitmap;

    if(at_offset >= IDT_MAX_ENTRIES) return -1;
    nr_entries = info->nr_entries;
    index      = at_offset / (sizeof(info->present[0]) * CHAR_BIT);
    bit        = at_offset % (sizeof(info->present[0]) * CHAR_BIT);
    bitmap     = info->present[index] & (1U << bit);

    nr_entries = !bitmap ? nr_entries + 1 : nr_entries;
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

    return 0;
}

/* TODO: Caller needs a descriptor to the new IDT entry it just got. */

int idt_entry_add(
    struct idt_info *info,
    void             (*idt_handler)(void),
    uint16_t         select,
    uint8_t          flags)
{
    /* TODO: Implement bitmap search for empty entries when remove added. */
    return idt_entry_set(info, idt_handler, info->nr_entries, select, flags);
}

/* This is disgusting, I know, but also necessary */

void exception_idt_init(struct idt_info *info)
{
    idt_entry_set(info, &exception_0x00, 0x00, 0x08, 0x8E);
    idt_entry_set(info, &exception_0x01, 0x01, 0x08, 0x8E);
    idt_entry_set(info, &exception_0x02, 0x02, 0x08, 0x8E);
    idt_entry_set(info, &exception_0x03, 0x03, 0x08, 0x8E);
    idt_entry_set(info, &exception_0x04, 0x04, 0x08, 0x8E);
    idt_entry_set(info, &exception_0x05, 0x05, 0x08, 0x8E);
    idt_entry_set(info, &exception_0x06, 0x06, 0x08, 0x8E);
    idt_entry_set(info, &exception_0x07, 0x07, 0x08, 0x8E);
    idt_entry_set(info, &exception_0x08, 0x08, 0x08, 0x8E);
    idt_entry_set(info, &exception_0x09, 0x09, 0x08, 0x8E);
    idt_entry_set(info, &exception_0x0A, 0x0A, 0x08, 0x8E);
    idt_entry_set(info, &exception_0x0B, 0x0B, 0x08, 0x8E);
    idt_entry_set(info, &exception_0x0C, 0x0C, 0x08, 0x8E);
    idt_entry_set(info, &exception_0x0D, 0x0D, 0x08, 0x8E);
    idt_entry_set(info, &exception_0x0E, 0x0E, 0x08, 0x8E);
    idt_entry_set(info, &exception_unknown, 0x0F, 0x08, 0x8E);
    idt_entry_set(info, &exception_0x10, 0x10, 0x08, 0x8E);
    idt_entry_set(info, &exception_0x11, 0x11, 0x08, 0x8E);
    idt_entry_set(info, &exception_0x12, 0x12, 0x08, 0x8E);
    idt_entry_set(info, &exception_0x13, 0x13, 0x08, 0x8E);
    idt_entry_set(info, &exception_0x14, 0x14, 0x08, 0x8E);
    idt_entry_set(info, &exception_0x15, 0x15, 0x08, 0x8E);

    return;
}

void irq_idt_init(struct idt_info *info)
{
    idt_entry_set(info, &irq_0x00, IRQ0_BASE_PM, 0x08, 0x8E);
    idt_entry_set(info, &irq_0x01, IRQ0_BASE_PM + 1, 0x08, 0x8E);
    idt_entry_set(info, &irq_0x02, IRQ0_BASE_PM + 2, 0x08, 0x8E);
    idt_entry_set(info, &irq_0x03, IRQ0_BASE_PM + 3, 0x08, 0x8E);
    idt_entry_set(info, &irq_0x04, IRQ0_BASE_PM + 4, 0x08, 0x8E);
    idt_entry_set(info, &irq_0x05, IRQ0_BASE_PM + 5, 0x08, 0x8E);
    idt_entry_set(info, &irq_0x06, IRQ0_BASE_PM + 6, 0x08, 0x8E);
    idt_entry_set(info, &irq_0x07, IRQ0_BASE_PM + 7, 0x08, 0x8E);

    idt_entry_set(info, &irq_0x08, IRQ1_BASE_PM, 0x08, 0x8E);
    idt_entry_set(info, &irq_0x09, IRQ1_BASE_PM + 1, 0x08, 0x8E);
    idt_entry_set(info, &irq_0x0A, IRQ1_BASE_PM + 2, 0x08, 0x8E);
    idt_entry_set(info, &irq_0x0B, IRQ1_BASE_PM + 3, 0x08, 0x8E);
    idt_entry_set(info, &irq_0x0C, IRQ1_BASE_PM + 4, 0x08, 0x8E);
    idt_entry_set(info, &irq_0x0D, IRQ1_BASE_PM + 5, 0x08, 0x8E);
    idt_entry_set(info, &irq_0x0E, IRQ1_BASE_PM + 6, 0x08, 0x8E);
    idt_entry_set(info, &irq_0x0F, IRQ1_BASE_PM + 7, 0x08, 0x8E);

    return;
}
