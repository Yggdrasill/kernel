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

#ifndef STRING_H
#define STRING_H

#include "stdint.h"

extern int16_t *__FB_ADDR __attribute__((section("fbr")));
#define FB_ADDR __FB_ADDR

void memsetw(int16_t *, int16_t, size_t);
void memcpy(void *, void *, size_t);
int memcmp(const void *, const void *, size_t);
size_t strlen(char *);
void putchar(char);
void puthex(size_t);
void puts(char *);

#endif
