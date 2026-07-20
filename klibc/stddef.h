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

#ifndef STDDEF_H
#define STDDEF_H

#if defined(__has_builtin)
    #if __has_builtin(__builtin_offsetof)
        #define offsetof(type, member) (__builtin_offsetof(type, member))
    #else
        #define offsetof(type, member) ((size_t)&(((type *)(0))->member))
    #endif
#else
    #define offsetof(type, member) ((size_t)&(((type *)(0))->member))
#endif

#define NULL ((void *)0)

#ifdef __i386__

typedef unsigned long size_t;
typedef signed long   ptrdiff_t;

typedef int wchar_t;

#endif /* __i386__ */

#endif /* STDDEF_H */
