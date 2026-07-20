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

#include <stdint.h>
#include <string.h>

#include <libk/util.h>

extern size_t *get_stack_base(void);

void stack_trace(void)
{
    uintptr_t *ebp;
    uintptr_t *eip;
    uintptr_t  addr;

    ebp = get_stack_base();
    puts("stack trace:");
    do {
        eip = (ebp + 1);
        puthex(eip, sizeof(eip), 0);
        /* Educated guess */
        if(*(uint8_t *)((*eip) - 5) == 0xE8) {
            addr = (*eip) - 5;
            putchar(' ');
            putchar('[');
            puthex(&addr, sizeof(eip), 0);
            putchar(']');
        }
        putchar('\n');

    } while((ebp = (uintptr_t *)*(ebp)));
}
