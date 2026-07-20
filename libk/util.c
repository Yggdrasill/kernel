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

void stack_trace(void)
{
    size_t *bp;

    __asm__ volatile("mov %0, ebp;"
                     : "=r"(bp));
    puts("stack trace:");
    puthex((bp + 1), sizeof(bp), 0);
    putchar('\n');
    while((bp = (size_t *)*bp)) {
        puthex((bp + 1), sizeof(bp), 0);
        putchar('\n');
    };
}
