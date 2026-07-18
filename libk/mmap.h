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

#ifndef MMAP_H
#define MMAP_H

#include <stdint.h>
#include <stddef.h>

struct e820_info;
struct e820_map;

struct e820_info mmap_sanitize(
    struct e820_map *, struct e820_map *, const uint32_t, const uint32_t);
struct e820_info *mmap_init(void);
struct e820_info *mmap_setup(struct e820_info *);

extern const size_t __mmap_max_entries;
extern const size_t __mmap_entry_size;

extern const size_t __mmap_base_offset;
extern const size_t __mmap_size_offset;
extern const size_t __mmap_type_offset;
extern const size_t __mmap_attr_offset;

#endif
