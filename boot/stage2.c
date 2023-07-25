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

#include "string.h"
#include "idt.h"
#include "irq.h"
#include "interrupt.h"
#include "mmap.h"

#ifdef TEST_MMAP
struct e820_map test_map[MMAP_MAX_ENTRIES] = {
    {0, 0x7F00, 1, 0},
    {0, 0x200, 2, 0},
    {0x600, 0x200, 5, 0},
    {0x1000, 0x200, 2, 0},
    {0x7E00, 0x8000, 1, 0},
    {0x50000, 0x8000, 1, 0},
    {0x57000, 0x2500, 2, 0}
};
#endif

int main(struct e820_map *start, struct e820_map *end)
{
    struct idt_ptr *idtp;
    struct idt_entry *entries;

    entries = (void *)&__IDT_BASE_LOCATION;

    memsetw((int16_t *)&FB_ADDR, 0x0720, 0x7D0);

    puts("Hello world!");

#ifdef TEST_MMAP
    struct e820_map dummy = {0};
    int i;
    for(i = 0; i < MMAP_MAX_ENTRIES; i++) {
        if(!memcmp(&test_map[i], &dummy, sizeof(*test_map) ) ) break;
    }
    mmap_init(test_map, i);
#else
    mmap_init(start, end - start);
#endif

    idtp = idt_init();
    idt_install(idtp);

    exception_idt_init(entries);

    irq_init();
    irq_idt_init(entries + 0x20);
    irq_mask_all();
    irq_unmask(IRQ_KEYBOARD);

    __asm__ volatile(
            "sti;"
            );

    for(;;) {
        __asm__ volatile(
                "hlt;"
                );
    }

    __asm__ volatile(
            "cli;"
            "hlt;"
            );
}
