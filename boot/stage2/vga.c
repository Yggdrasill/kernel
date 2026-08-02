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

#include "vga.h"
#include "stage2.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

static int16_t *fb;

void fb_init(int16_t *buf)
{
    memset(buf, 0x00, 0xFA0);
    fb = buf;

    return;
}

void putchar(char ch)
{
    int16_t *vga;

    static size_t y;
    static size_t x;

    if(x >= 80 || ch == '\n') {
        x = 0;
        y++;
    }
    if(y >= 25) y = 0;

    vga = fb + (y * 80) + x;

    if(ch == '\n') {
        memsetw(vga, 0x0720, 80 - x);
        return;
    }

    *vga = 0x0700 | ch;
    x++;

    return;
}

void puthex(void *hex, size_t n, uint8_t cut)
{
    char *hex_array;
    char  chars[2];

    size_t i, j;

    hex_array = (char *)hex;
    putchar('0');
    putchar('x');

    for(i = n, j = i - 1; i > 0; i--, j--) {
        chars[0] = (hex_array[j] & 0xF0) >> 4;
        chars[1] = hex_array[j] & 0x0F;
        chars[0] = (char)(chars[0] + (chars[0] >= 0x0A ? 'A' - 0x0A : '0'));
        chars[1] = (char)(chars[1] + (chars[1] >= 0x0A ? 'A' - 0x0A : '0'));

        if(i <= 1 || !cut || chars[0] != '0' || chars[1] != '0') {
            putchar(chars[0]);
            putchar(chars[1]);
            cut = 0;
        }
    }

    return;
}

void puts(const char *str)
{
    while(*str) {
        putchar(*str);
        str++;
    }
    putchar('\n');
}
