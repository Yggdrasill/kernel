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

#define NEW_EVENT(start, end, event_type)                                      \
    events[nr_events] = (struct e820_event){                                   \
        .addr = (uintptr_t)&start,                                             \
        .type = event_type,                                                    \
        .base = MMAP_EVENT_BASE,                                               \
    };                                                                         \
    events[nr_events + 1] = (struct e820_event){                               \
        .addr = (uintptr_t)&end,                                               \
        .type = event_type,                                                    \
        .base = MMAP_EVENT_END,                                                \
    };                                                                         \
    nr_events += 2

static size_t mmap_clobber(struct e820_events *info)
{
    struct e820_event *events;

    uint8_t nr_events;

    events    = info->events;
    nr_events = (uint8_t)info->nr_events;

    NEW_EVENT(__BIOS_START, __BIOS_END, MMAP_RESERVED);
    NEW_EVENT(__BOOTLOADER_START, __BOOTLOADER_END, MMAP_BOOT_RECLAIMABLE);
    NEW_EVENT(__PREALLOC_START, __PREALLOC_END, MMAP_BOOT_RECLAIMABLE);
    NEW_EVENT(__STACK_START, __STACK_END, MMAP_BOOT_RECLAIMABLE);
    NEW_EVENT(FB_ADDR, FB_END, MMAP_FRAMEBUFFER);
    NEW_EVENT(__UPPER_START, __UPPER_END, MMAP_RESERVED);

    info->nr_events = nr_events;
    return nr_events;
}

static void mmap_print(struct e820_info *info)
{
    size_t i;

    for(i = 0; i < info->nr_entries; i++) {
        puthex(&info->base[i].base, sizeof(info->base[i].base), 0);
        putchar(' ');
        puthex(&info->base[i].size, sizeof(info->base[i].size), 0);
        putchar(' ');
        puthex(&info->base[i].type, sizeof(info->base[i].type), 1);
        putchar(' ');
        puthex(&info->base[i].attrib, sizeof(info->base[i].attrib), 1);
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

struct e820_info *boot_mmap_ptr(void)
{
    return &old_mmap_info;
}

struct e820_info *boot_mmap_init(struct e820_info *old_info)
{
    struct e820_event  events[2 * MMAP_MAX_ENTRIES];
    struct e820_events event_info;

    struct e820_info *new_info;

    char *error;
    int   status;

    event_info = (struct e820_events){
        .events        = events,
        .nr_events     = 0,
        .max_nr_events = 2 * MMAP_MAX_ENTRIES,
    };
    mmap_clobber(&event_info);
    if(mmap_transform_map(&event_info, old_info)) {
        panic("boot_mmap_init: Could not transform E820 memory map!");
    }

    new_info = &new_mmap_info;
    status   = mmap_sanitize(new_info, &event_info);
    if(status && status > -3) {
        error = mmap_sanitize_error(status);
        panic(error);
    } else if(status == -3) {
        puts("boot_mmap_init WARN: Exhausted capacity, E820 map truncated.");
    }
    mmap_print(new_info);

    return new_info;
}
