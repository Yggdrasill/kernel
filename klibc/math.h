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

#ifndef MATH_H
#define MATH_H

#define SAFE_ADD_DECLARE(name, type)                                           \
    int safe_add_##name(type *c, const type a, const type b);

#define SAFE_ADD_IMPLEMENT(name, type, MAX)                                    \
    int safe_add_##name(type *c, const type a, const type b)                   \
    {                                                                          \
        if(!c || a > MAX - b) return -1;                                       \
        *c = a + b;                                                            \
        return 0;                                                              \
    }

#define SAFE_SUB_DECLARE(name, type)                                           \
    int safe_sub_##name(type *c, const type a, const type b);

#define SAFE_SUB_IMPLEMENT(name, type)                                         \
    int safe_sub_##name(type *c, const type a, const type b)                   \
    {                                                                          \
        if(!c || a < b) return -1;                                             \
        *c = a - b;                                                            \
        return 0;                                                              \
    }

#endif
