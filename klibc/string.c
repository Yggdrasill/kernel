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
#include <string.h>

void *memset(void *s, int c, size_t n)
{
    unsigned char *ptr;

    size_t i;

    i   = 0;
    ptr = (unsigned char *)s;
    while(i < n) {
        *(ptr + i) = (unsigned char)c;
        i++;
    }

    return s;
}

void *memsetw(int16_t *s, int16_t c, size_t n)
{
    int16_t *ptr;
    size_t   i;

    i   = 0;
    ptr = s;
    while(i < n) {
        *(ptr + i) = (unsigned char)c;
        i++;
    }

    return s;
}

void *memcpy(void *restrict s1, const void *restrict s2, size_t n)
{
    unsigned char       *p1;
    const unsigned char *p2;

    size_t i;

    i  = 0;
    p1 = (unsigned char *)s1;
    p2 = (const unsigned char *)s2;
    while(i < n) {
        *(p1 + i) = *(p2 + i);
        i++;
    }

    return s1;
}

void *memmove(void *s1, const void *s2, size_t n)
{
    unsigned char       *p1;
    const unsigned char *p2;

    size_t i;

    if(s1 == s2 || !n) goto memmove_exit;

    i  = 0;
    p1 = (unsigned char *)s1;
    p2 = (const unsigned char *)s2;

    if((uintptr_t)p1 < (uintptr_t)p2) {
        while(i < n) {
            *(p1 + i) = *(p2 + i);
            i++;
        }
    } else {
        i = n;
        while(i != 0) {
            *(p1 + i - 1) = *(p2 + i - 1);
            i--;
        }
    }

memmove_exit:
    return s1;
}

int memcmp(const void *s1, const void *s2, size_t n)
{
    const unsigned char *p1;
    const unsigned char *p2;

    size_t i;
    int    rv;

    i  = 0;
    rv = 0;
    p1 = (const unsigned char *)s1;
    p2 = (const unsigned char *)s2;
    while(i < n) {
        rv = *(p1 + i) - *(p2 + i);
        if(rv != 0) break;
        i++;
    }

    return rv;
}

size_t strlen(const char *str)
{
    size_t i;
    i = 0;
    while(*(str + i)) i++;
    return i;
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

    vga = (int16_t *)&FB_ADDR + (y * 80) + x;

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
