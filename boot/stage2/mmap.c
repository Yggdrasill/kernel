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

#include <libk/internal/mmap.h>
#include <libk/mmap.h>
#include <libk/util.h>
#include <string.h>

#include "mmap.h"

#define MMAP_REGION_SIZE(start, end) ((uintptr_t)&end - (uintptr_t)&start)

extern char __BIOS_START;
extern char __BIOS_END;
extern char __BOOTLOADER_START;
extern char __BOOTLOADER_END;

extern char __PREALLOC_START;
extern char __PREALLOC_END;
extern char __STACK_START;
extern char __STACK_END;

extern char __UPPER_START;
extern char __UPPER_END;

/*
 * As in every other source file: This type of global state is bootloader only.
 */

extern struct e820_map __mmap_old_map[MMAP_MAX_ENTRIES];
extern struct e820_map __mmap_new_map[MMAP_MAX_ENTRIES];

static struct e820_map *const old_map = __mmap_old_map;
static struct e820_map *const new_map = __mmap_new_map;

static size_t mmap_clobber(struct e820_info *info)
{
    struct e820_map *mmap;
    size_t           nr_entries;

    mmap       = info->base;
    nr_entries = info->nr_entries;

    mmap[nr_entries++] = (struct e820_map){
        .base   = (uintptr_t)&__BIOS_START,
        .size   = (uintptr_t)&__BIOS_END - (uintptr_t)&__BIOS_START,
        .type   = MMAP_RESERVED,
        .attrib = 0,
    };
    mmap[nr_entries++] = (struct e820_map){
        .base   = (uintptr_t)&__BOOTLOADER_START,
        .size   = (uintptr_t)&__BOOTLOADER_END - (uintptr_t)&__BOOTLOADER_START,
        .type   = MMAP_BOOTLOADER_RECLAIMABLE,
        .attrib = 0,
    };
    mmap[nr_entries++] = (struct e820_map){
        .base   = (uintptr_t)&__PREALLOC_START,
        .size   = MMAP_REGION_SIZE(__PREALLOC_START, __PREALLOC_END),
        .type   = MMAP_BOOTLOADER_RECLAIMABLE,
        .attrib = 0,
    };
    mmap[nr_entries++] = (struct e820_map){
        .base   = (uintptr_t)&__STACK_START,
        .size   = MMAP_REGION_SIZE(__STACK_START, __STACK_END),
        .type   = MMAP_BOOTLOADER_RECLAIMABLE,
        .attrib = 0,
    };
    mmap[nr_entries++] = (struct e820_map){
        .base   = (uintptr_t)&FB_ADDR,
        .size   = (uintptr_t)&FB_END - (uintptr_t)&FB_ADDR,
        .type   = MMAP_FRAMEBUFFER,
        .attrib = 0,
    };
    mmap[nr_entries++] = (struct e820_map){
        .base   = (uintptr_t)&__UPPER_START,
        .size   = (uintptr_t)&__UPPER_END - (uintptr_t)&__UPPER_START,
        .type   = MMAP_RESERVED,
        .attrib = 0,
    };

    info->nr_entries = nr_entries;
    return nr_entries;
}

static void mmap_print(struct e820_info *info)
{
    size_t i;

    for(i = 0; i < info->nr_entries; i++) {
        puthex(&info->base[i].base, sizeof(info->base[i].base), 0);
        putchar(' ');
        puthex(&info->base[i].size, sizeof(info->base[i].size), 0);
        putchar(' ');
        puthex(&info->base[i].type, sizeof(info->base[i].type), 0);
        putchar('\n');
    }

    return;
}

static struct e820_info old_mmap_info = {
    .base           = old_map,
    .nr_entries     = 0,
    .max_nr_entries = MMAP_MAX_ENTRIES,
};

static struct e820_info new_mmap_info = {
    .base           = new_map,
    .nr_entries     = 0,
    .max_nr_entries = MMAP_MAX_ENTRIES,
};

struct e820_info *boot_mmap_init(void)
{
    struct e820_info *info;

    info = &old_mmap_info;
    mmap_clobber(info);

    return info;
}

struct e820_info *boot_mmap_setup(struct e820_info *old_info)
{
    struct e820_info *new_info;

    char *error;
    int   status;

    new_info = &new_mmap_info;
    status   = mmap_sanitize(new_info, old_info);
    if(status) {
        error = mmap_sanitize_error(status);
        panic(error);
    }
//    mmap_print(new_info);
    return new_info;
}
