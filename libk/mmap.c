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

#include <math.h>
#include <sort.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <libk/internal/mmap.h>
#include <libk/mmap.h>
#include <libk/util.h>

/* clang-format off */
static ISORT_IMPLEMENT(mmap, struct e820_event)
static SAFE_ADD_IMPLEMENT(uint64, uint64_t, UINT64_MAX)

static int mmap_cmp(const struct e820_event *p1, const struct e820_event *p2)
{
    if(p1->addr == p2->addr) {
        if(p1->base == p2->base) return 0;
        return p1->base;
    }
    return (p1->addr > p2->addr) - (p1->addr < p2->addr);
}

/*
 * Compare type precedence and pick the worst type. The standard types
 * are:
 *
 * MMAP_USABLE           = 1
 * MMAP_RESERVED         = 2
 * MMAP_ACPI_RECLAIMABLE = 3
 * MMAP_ACPI_NVS         = 4 (non-volatile storage)
 * MMAP_BAD_MEMORY       = 5
 *
 * There are also a few custom types:
 *
 * MMAP_ATTR_INVALID     = 0xFC,
 * MMAP_ATTR_NVS         = 0xFD,
 * MMAP_BOOT_RECLAIMABLE = 0xFE,
 * MMAP_FRAMEBUFFER      = 0xFF,
 *
 * Types 2, 4, and 5 of the standard types are totally unusable and 
 * should not be touched. Type 3 may be reclaimed after reading the
 * ACPI tables.
 *
 * Types 0xFB, 0xFD, 0xFF of the custom types are absolutely not usable
 * either. Type 0xFC is reclaimed if and only if the BIOS is producing
 * nothing but invalid attributes for MMAP_USABLE, and 0xFE is
 * reclaimable after the bootloader is done.
 *
 * The precedence order is the following:
 *
 * 0xFF > 5 > 4 > 0xFC > 0xFD > 2 > 3 > 0xFE > 1
 *
 * It may not be desired to always reclaim usable memory.
 */

static const uint32_t mmap_types[] = {
    MMAP_FRAMEBUFFER,
    MMAP_BAD_MEMORY,
    MMAP_ACPI_NVS,
    MMAP_ATTR_INVALID,
    MMAP_ATTR_NVS,
    MMAP_RESERVED,
    MMAP_ACPI_RECLAIMABLE,
    MMAP_BOOT_RECLAIMABLE,
    MMAP_USABLE,
};

#define MMAP_NR_TYPES (sizeof(mmap_types) / sizeof(mmap_types[0]))

static uint32_t mmap_convert_type(const uint32_t type)
{
    switch(type) {
        case MMAP_FRAMEBUFFER: return 0;
        case MMAP_BAD_MEMORY: return 1;
        case MMAP_ACPI_NVS: return 2;
        case MMAP_ATTR_INVALID: return 3;
        case MMAP_ATTR_NVS: return 4;
        default:
        case MMAP_RESERVED: return 5;
        case MMAP_ACPI_RECLAIMABLE: return 6;
        case MMAP_BOOT_RECLAIMABLE: return 7;
        case MMAP_USABLE: return 8;
    }
}

static uint32_t mmap_map_attribute(const uint32_t type, const uint32_t attrib)
{
    if(attrib & 0x2) return MMAP_ATTR_NVS;
    return attrib & 0x1 ? type : MMAP_ATTR_INVALID;
}

int mmap_transform_map(struct e820_events *event_info, const struct e820_info *src_info)
{
    struct e820_event *events;
    struct e820_map   *src;

    uint64_t end;
    uint32_t type;
    size_t   nr_invalid;
    size_t   i, j;

    int ignore_attr;

    if(!event_info || !src_info || !event_info->events || !src_info->base) {
        return -1;
    }

    const size_t max_nr_events = event_info->max_nr_events;

    src    = src_info->base;
    events = event_info->events;

    /*
     * Some BIOSes always return with the valid bit clear. Others don't bother
     * with the attribute field at all. SeaBIOS is of the latter type. The E820
     * routine sets the valid bit for the latter case, but other BIOSes clear
     * it. If all the E820 entries are MMAP_ATTR_INVALID type, then ignore the
     * attribute field.
     */
    ignore_attr = 0;
    nr_invalid  = 0;
    for(i = 0; i < src_info->nr_entries; i++) {
        type = mmap_map_attribute(src[i].type, src[i].attrib);
        if(type == MMAP_ATTR_INVALID) nr_invalid++;
    }
    if(nr_invalid >= i) ignore_attr = 1;

    end  = 0;
    j    = event_info->nr_events;
    for(i = 0; i < src_info->nr_entries && j < max_nr_events; i++) {
        if(!src[i].size) continue;

        if(!ignore_attr) type = mmap_map_attribute(src[i].type, src[i].attrib);
        else type = src[i].type;

        if(safe_add_uint64(&end, src[i].base, src[i].size)) return -2;
        /* base field is in sort order */
        events[j] = (struct e820_event) {
            .addr = src[i].base,
            .type = type,
            .id   = (uint8_t)j,
            .base = MMAP_EVENT_BASE,
        };
        events[j + 1] = (struct e820_event) {
            .addr = end,
            .type = type,
            .id   = (uint8_t)j,
            .base = MMAP_EVENT_END,
        };
        j += 2;
    }

    event_info->nr_events = j;

    isort_mmap(events, event_info->nr_events, mmap_cmp);

    if(i < src_info->nr_entries && j >= max_nr_events) return -2;

    return 0;
}

int mmap_sanitize(struct e820_info *dst_info, const struct e820_events *event_info)
{
    struct e820_event *events;
    struct e820_map   *dst;

    uint64_t prev_addr;
    uint32_t type;
    uint32_t prev_type;
    uint32_t effective_type;
    uint32_t active_types[MMAP_NR_TYPES] = {0};

    size_t nr_entries;
    size_t i, j;

    if(!dst_info || !dst_info->base || !event_info || !event_info->events) {
        return -1;
    }
    if(!event_info->nr_events) return -2;

    const size_t max_nr_entries = dst_info->max_nr_entries;

    dst    = dst_info->base;
    events = event_info->events;

    prev_addr = events[0].addr;
    prev_type = events[0].type;

    effective_type = mmap_convert_type(prev_type);
    active_types[effective_type] = 1;

    nr_entries  = 0;
    for(i = 1; nr_entries < max_nr_entries && i < event_info->nr_events; i++) {
        effective_type = mmap_convert_type(events[i].type);
        if(events[i].base == MMAP_EVENT_BASE) active_types[effective_type]++;
        else active_types[effective_type]--;

        effective_type = mmap_convert_type(MMAP_RESERVED);
        for(j = 0; j < MMAP_NR_TYPES; j++) {
            if(active_types[j] > 0) {
                effective_type = j;
                break;
            }
        }
        type = mmap_types[effective_type];

        if(type != prev_type || i >= event_info->nr_events - 1) {
            dst[nr_entries] = (struct e820_map) {
                .base = prev_addr,
                .size = events[i].addr - prev_addr,
                .type = prev_type,
                .attrib = 1,
            };
            nr_entries += (events[i].addr - prev_addr) > 0;
            prev_addr = events[i].addr;
            prev_type = type;
        }
    }

    dst_info->nr_entries = nr_entries;
    if(nr_entries >= max_nr_entries && i < event_info->nr_events) return -3;

    return 0;
}

char *mmap_sanitize_error(int status)
{
    switch(status) {
        case -1: return "E820san: Null pointer argument";
        case -2: return "E820san: Empty input event list";
        case -3: return "E820san: Insufficient capacity";
    }
    return "E820san: Unknown error";
}
