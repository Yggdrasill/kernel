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

#define NULL (void *)0

# ifdef __i386__

    typedef char                int8_t;
    typedef short               int16_t;
    typedef long                int32_t;
    typedef long long           int64_t;

    typedef unsigned char       uint8_t;
    typedef unsigned short      uint16_t;
    typedef unsigned long       uint32_t;
    typedef unsigned long long  uint64_t;

    typedef long                ssize_t;
    typedef unsigned long       size_t;

    typedef long                intptr_t;
    typedef unsigned long       uintptr_t;

# endif

#endif
