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

#include "stdint.h"

#define MMAP_MAX_ENTRIES 128

enum MMAP_TYPES {
    MMAP_USABLE = 1,
    MMAP_RESERVED,
    MMAP_ACPI_RECLAIMABLE,
    MMAP_ACPI_NVS,
    MMAP_BAD_MEMORY,
    MMAP_BOOTLOADER_RECLAIMABLE,
    MMAP_FRAMEBUFFER
};

struct e820_map {
    uint64_t    base;
    uint64_t    size;
    uint32_t    type;
    uint32_t    attrib;
};


int mmap_init(struct e820_map *, int);

#endif
