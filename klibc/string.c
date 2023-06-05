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

void memsetw(void *s, int16_t c, size_t n)
{
  char *p;
  size_t i;

  p = (char *)s;

  for(i = 0; i < n; i += 2) {
    *(p + i) = c;
  }

  return;
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
  short *vga;

  static int y;
  static int x;

  if(x >= 160 || ch == '\n') {
    x = 0;
    y++;
  }
  if(y >= 25) y = 0;
  if(ch == '\n') {
    memsetw( (void *)(0xB8000 + (y * 160) ), 0x0720, 0x50);
    return;
  }

  vga = (short *)(0xB8000 + (y * 160) + x);
  *vga = 0x0700 | ch;
  x += 2;

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
