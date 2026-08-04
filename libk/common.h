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

#ifndef COMMON_H
#define COMMON_H

/* Common definitions, structs etc. that are widely applicable. */

#include <limits.h>
#include <stddef.h>

#define bits_sizeof(x) (sizeof(x) * CHAR_BIT)

#define PAGE_LOG   (12)
#define PAGE_ALIGN (1ULL << PAGE_LOG)
#define PAGE_MASK  (~(PAGE_ALIGN - 1ULL))

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

struct extent {
    size_t start;
    size_t end;
};

#endif
