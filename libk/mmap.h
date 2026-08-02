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

#define MMAP_MAX_ENTRIES 128

#if !defined(LD_BOOT_STAGE1) && !defined(LD_BOOT_STAGE2)

    #include <stddef.h>
    #include <stdint.h>

enum MMAP_TYPES {
    MMAP_USABLE           = 1,
    MMAP_RESERVED         = 2,
    MMAP_ACPI_RECLAIMABLE = 3,
    MMAP_ACPI_NVS         = 4,
    MMAP_BAD_MEMORY       = 5,
    MMAP_ATTR_INVALID     = 0xFC,
    MMAP_ATTR_NVS         = 0xFD,
    MMAP_BOOT_RECLAIMABLE = 0xFE,
    MMAP_FRAMEBUFFER      = 0xFF,
};

struct e820_map {
    uint64_t base;
    uint64_t size;
    uint32_t type;
    uint32_t attrib;
};

struct e820_info {
    struct e820_map *base;
    size_t           nr_entries;
    size_t           max_nr_entries;
};

struct e820_event {
    uint64_t addr;
    uint32_t type;
    int8_t   base;
};

struct e820_events {
    struct e820_event *events;
    uint16_t           nr_events;
    uint16_t           max_nr_events;
};

    #define MMAP_ENTRY_SIZE  (sizeof(struct e820_map))
    #define MMAP_BASE_OFFSET (offsetof(struct e820_map, base))
    #define MMAP_SIZE_OFFSET (offsetof(struct e820_map, size))
    #define MMAP_TYPE_OFFSET (offsetof(struct e820_map, type))
    #define MMAP_ATTR_OFFSET (offsetof(struct e820_map, attrib))

    #define INFO_BASE_OFFSET   (offsetof(struct e820_info, base))
    #define INFO_NR_ENT_OFFSET (offsetof(struct e820_info, nr_entries))
    #define INFO_MAX_NR_OFFSET (offsetof(struct e820_info, max_nr_entries))

    #define MMAP_EVENT_BASE -1
    #define MMAP_EVENT_END  1

int   mmap_transform_map(struct e820_events *, const struct e820_info *);
int   mmap_sanitize(struct e820_info *, const struct e820_events *);
char *mmap_sanitize_error(int);

#endif

#endif
