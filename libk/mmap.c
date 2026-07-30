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

struct e820_point {
    struct e820_map *entry;
    uint64_t         addr;
};

/* clang-format off */
static ISORT_IMPLEMENT(mmap, struct e820_point)
static SAFE_ADD_IMPLEMENT(uint64, uint64_t, UINT64_MAX)

static int mmap_is_base(const struct e820_point *p)
/* clang-format on */
{
    return p->addr == p->entry->base;
}

static int mmap_cmp(const struct e820_point *p1, const struct e820_point *p2)
{
    if(p1->addr == p2->addr) {
        if(mmap_is_base(p1) == mmap_is_base(p2)) return 0;
        else return mmap_is_base(p1) ? -1 : 1;
    }
    return (p1->addr > p2->addr) - (p1->addr < p2->addr);
}

static int mmap_bad_type(uint32_t type)
{
    switch(type) {
        case MMAP_USABLE:
        case MMAP_ACPI_RECLAIMABLE:
        case MMAP_BOOTLOADER_RECLAIMABLE: return 0;
        default: return 1;
    }
}

static uint32_t mmap_compare_type(const uint32_t t1, const uint32_t t2)
{
    if(!mmap_bad_type(t1) && mmap_bad_type(t2)) return t2;
    if(mmap_bad_type(t1) && !mmap_bad_type(t2)) return t1;
    return t1 > t2 ? t1 : t2;
}

        /*
         * Compare type precedence and pick the worst type. It is roughly in the
         * order of greatest type. the possible types are:
         *
         * MMAP_USABLE = 1
         * MMAP_RESERVED = 2
         * MMAP_ACPI_RECLAIMABLE = 3
         * MMAP_ACPI_NVS = 4 (non-volatile storage)
         * MMAP_BAD_MEMORY = 5
         *
         * There are also a few custom types:
         *
         * MMAP_BOOTLOADER_RECLAIMABLE = 6
         * MMAP_FRAMEBUFFER = 7
         *
         * Type 2-5 inclusive, and type 7, are all considered unusable memory
         * and should not be touched. All the usable types have precedence of
         * the greatest type, so the type precedence is ultimately:
         *
         * 7 > 5 > 4 > 2 > 6 > 3 > 1
         *
         * It may not be desired to always reclaim usable memory, hence why they
         * have greater precedence than type 1.
         */

char *mmap_sanitize_error(int status)
{
    switch(status) {
        case -1: return "E820san: Null pointer argument";
        case -2: return "E820san: Empty input map";
        case -3: return "E820san: Insufficient capacity";
        case -4: return "E820san: No points left to process";
        case -5: return "E820san: Output map exhausted";
    }
    return "E820san: Unknown error";
}
