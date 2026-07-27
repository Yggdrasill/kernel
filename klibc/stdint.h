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

#ifndef STDINT_H
#define STDINT_H

#include <limits.h>

typedef signed char      int8_t;
typedef signed short     int16_t;
typedef signed long      int32_t;
typedef signed long long int64_t;

typedef unsigned char      uint8_t;
typedef unsigned short     uint16_t;
typedef unsigned long      uint32_t;
typedef unsigned long long uint64_t;

typedef signed long ssize_t;

typedef signed short   ptr16_t;
typedef unsigned short uptr16_t;

typedef signed long   ptr32_t;
typedef unsigned long uptr32_t;

typedef signed long   intptr_t;
typedef unsigned long uintptr_t;

#define INT8_MIN  -128
#define INT8_MAX  127
#define UINT8_MAX 255

#define INT16_MIN  -32768
#define INT16_MAX  32767
#define UINT16_MAX 65535

#define INT32_MIN  (-2147483647L - 1)
#define INT32_MAX  2147483647L
#define UINT32_MAX 4294967295U

#define INT64_MIN  (-9223372036854775807LL - 1)
#define INT64_MAX  9223372036854775807LL
#define UINT64_MAX 18446744073709551615ULL

#define SSIZE_MIN INT32_MIN
#define SSIZE_MAX INT32_MAX
#define SIZE_MAX  UINT32_MAX

#define INTPTR_MIN  INT32_MIN
#define INTPTR_MAX  INT32_MAX
#define UINTPTR_MAX UINT32_MAX

#endif
