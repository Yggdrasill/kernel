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

#include "io.h"

unsigned char inb(uint16_t port)
{
    unsigned char data;

    data = 0;

    __asm__ volatile(
            "inb  %0, %1;"
            : "=r"(data)
            : "Nd"(port)
            );

    return data;
}

void outb(uint16_t port, unsigned char data)
{
    __asm__ volatile(
            "outb %0, %1;"
            :
            : "dx"(port), "eax"(data)
            );

    return;
}
