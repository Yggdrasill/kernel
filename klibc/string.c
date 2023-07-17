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

void memsetw(int16_t *s, int16_t c, size_t n)
{
    int16_t *ptr;
    size_t i;

    ptr = (int16_t *)s;

    for(i = 0; i < n; i++) {
        *(ptr + i) = c;
    }

    return;
}

void memcpy(void *dst, void *src, size_t n)
{
    void *end;

    end = (char *)dst + n;
    while( (char *)dst < (char *)end) {
        *(char *)dst = *(char *)src;
        dst = (char *)dst + 1;
        src = (char *)src + 1;
    }
}

size_t strlen(char *str)
{
    unsigned long long retval;

    retval = 0;

    while(*str++) retval++;

    return retval;
}

void putchar(char ch)
{
    int16_t *vga;

    static int y;
    static int x;

    if(x >= 80 || ch == '\n') {
        x = 0;
        y++;
    }
    if(y >= 25) y = 0;

    vga = (int16_t *)&FB_ADDR + (y * 80) + x;

    if(ch == '\n') {
        memsetw(vga, 0x0720, 80 - x);
        return;
    }

    *vga = 0x0700 | ch;
    x++;

    return;
}

void puthex(size_t hex)
{
    char *hex_array;
    char chars[2];
    size_t i, j;

    hex_array = (char *)&hex;

    putchar('0');
    putchar('x');

    for(i = sizeof(hex), j = i - 1; i > 0; i--, j--) {
        chars[0] = (hex_array[j] & 0xF0) >> 4;
        chars[1] = hex_array[j] & 0x0F;
        chars[0] += chars[0] >= 0x0A ? 'A' - 0x0A : '0';
        chars[1] += chars[1] >= 0x0A ? 'A' - 0x0A : '0';

        putchar(chars[0]);
        putchar(chars[1]);
    }

    putchar('\n');

    return;
}

void puts(char *str)
{
    while(*str) {
        putchar(*str);
        str++;
    }

    putchar('\n');
}
