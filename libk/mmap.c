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

#include <stdint.h>
#include <string.h>
#include <sort.h>
#include <mmap.h>

#define MMAP_TABLE_SIZE sizeof(struct e820_map) * MMAP_MAX_ENTRIES
#define MMAP_END_ADDR(x) ( (x)->base + (x)->size)

struct e820_point {
    struct e820_map *entry;
    uint64_t addr;
};

int mmap_cmp(const void *p1, const void *p2)
{
    return ( (struct e820_point *)p1)->addr > ( (struct e820_point *)p2)->addr;
}

int mmap_bad_type(uint32_t type)
{
    switch(type) {
        case MMAP_USABLE:
        case MMAP_ACPI_RECLAIMABLE:
        case MMAP_BOOTLOADER_RECLAIMABLE:
            return 0;
        default:
            return 1;
    }
}

uint32_t mmap_compare_type(const uint32_t t1, const uint32_t t2)
{
    if(!mmap_bad_type(t1) && mmap_bad_type(t2) ) return t2;
    if(mmap_bad_type(t1) && !mmap_bad_type(t2) ) return t1;
    return t1 > t2 ? t1 : t2;
}

int mmap_is_base(struct e820_point *p)
{
    return p->addr == p->entry->base;
}

__attribute__((__section__(".mmap")))
struct e820_map __mmap_old_map[MMAP_MAX_ENTRIES];
__attribute__((__section__(".mmap")))
struct e820_map __mmap_new_map[MMAP_MAX_ENTRIES];
static struct e820_map *const old_map = __mmap_old_map;
static struct e820_map *const new_map = __mmap_new_map;
static int old_nmemb;
static int new_nmemb;

int mmap_sanitize(struct e820_map **mmap, const int nr_entries)
{
    struct e820_point e820_points[2 * MMAP_MAX_ENTRIES];

    struct e820_map *overlap_map[MMAP_MAX_ENTRIES];

    struct e820_map *pmap;
    struct e820_point *prev_point;

    uint32_t type;
    uint32_t prev_type;

    const int NR_POINTS = 2 * nr_entries;

    int new_nr_entries;
    int nr_overlaps;
    int i, j;

    pmap = *mmap;

    /*
     * Break down the E820 structure into a sorted list of points that can be
     * traversed. From these points the memory map will be rebuilt from scratch,
     * since it can be messy when the BIOS provides it.
     */
    
    j = 0;
    i = 0;
    for(i = 0; i < nr_entries; i++) {
        e820_points[j++] = (struct e820_point) { 
            pmap + i, 
            pmap[i].base
        };
        e820_points[j++] = (struct e820_point) { 
            pmap + i,
            pmap[i].base + pmap[i].size
        };
    }
    isort(e820_points, NR_POINTS, sizeof(*e820_points), mmap_cmp);

    new_nr_entries = 0;
    nr_overlaps = 0;
    prev_point = e820_points;
    prev_type = prev_point->entry->type;
    overlap_map[nr_overlaps++] = prev_point->entry;
    
    for(i = 1; i < NR_POINTS && new_nr_entries < MMAP_MAX_ENTRIES; i++) {

        /*
         * Build a map of possibly overlapping entries. If the point is a base
         * address entry, add it to the overlap map. If not, remove it.
         */

        if(mmap_is_base(e820_points + i) ) {
            overlap_map[nr_overlaps++] = e820_points[i].entry;
        } else {
            for(j = 0; j < nr_overlaps; j++) {
                if(overlap_map[j] == e820_points[i].entry) {
                    overlap_map[j] = overlap_map[--nr_overlaps];
                    break;
                }
            }
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

        type = overlap_map[0]->type;
        for(j = 1; j < nr_overlaps; j++) {
            type = mmap_compare_type(overlap_map[j]->type, type);
        }

        /*
         * Reconstruct the map from the previous entry if type changes, or if we
         * are at the last element of the array.
         */

        if(type != prev_type || i == NR_POINTS - 1) {
            new_map[new_nr_entries] = (struct e820_map) {
                prev_point->addr,
                e820_points[i].addr - prev_point->addr,
                prev_type,
                prev_point->entry->attrib
            };
            prev_point = e820_points + i;
            prev_type = type;
            new_nr_entries += new_map[new_nr_entries].size > 0;
        }
    }

    *mmap = new_map;
    return new_nr_entries;
}

void mmap_print(struct e820_map *mmap, int nmemb)
{
    for(int i = 0; i < nmemb; i++) {
        puthex(mmap[i].base);
        putchar(' ');
        puthex(mmap[i].size);
        putchar(' ');
        puthex(mmap[i].type);
        putchar('\n');
    }

    return;
}

int mmap_clobber(struct e820_map *mmap, int nmemb)
{
    uint64_t base;
    uint64_t size;
    uint64_t old_size;
    uint32_t old_type;
    enum MMAP_TYPES type;

    old_type = mmap[0].type;
    old_size = mmap[0].size;
    base = (uintptr_t)&__bios_start;
    size = (uintptr_t)&__bios_end - (uintptr_t)&__bios_start;
    type = MMAP_RESERVED;
    mmap[nmemb++] = (struct e820_map) { base, size, type, 0 }; 
    base = (uintptr_t)&__bootloader_start; 
    size = (uintptr_t)&__bootloader_end - (uintptr_t)&__bootloader_start;
    type = MMAP_BOOTLOADER_RECLAIMABLE;
    mmap[nmemb++] = (struct e820_map) { base, size, type, 0 }; 
    base = (uintptr_t)old_map;
    size = MMAP_TABLE_SIZE;
    mmap[nmemb++] = (struct e820_map) { base, size, type, 0 };
    base = (uintptr_t)new_map;
    mmap[nmemb++] = (struct e820_map) { base, size, type, 0 };
    base = base + size;
    type = old_type;
    size = old_size - base;
    mmap[nmemb++] = (struct e820_map) { base, size, type, 0 };
    base = (uintptr_t)&FB_ADDR;
    size = (uintptr_t)&FB_END - (uintptr_t)&FB_ADDR;
    type = MMAP_FRAMEBUFFER;
    mmap[nmemb++] = (struct e820_map) { base, size, type, 0 };
    base = (uintptr_t)&__upper_start;
    size = (uintptr_t)&__upper_end - (uintptr_t)&__upper_start;
    type = MMAP_RESERVED;
    mmap[nmemb++] = (struct e820_map) { base, size, type, 0 };

    return nmemb;
}

int mmap_init(struct e820_map *mmap, int nmemb)
{

    old_nmemb = nmemb;

    nmemb = mmap_clobber(mmap, nmemb);
    new_nmemb = mmap_sanitize(&mmap, nmemb);
    nmemb = new_nmemb;
    mmap_print(mmap, nmemb);

    return 0;
}
