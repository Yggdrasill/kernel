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

int mmap_cmp(const void *p1, const void *p2)
{
    return ( (struct e820_map *)p1)->base > ( (struct e820_map *)p2)->base;
}

int mmap_bad_type(size_t type)
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

struct e820_map *mmap_compare_type(struct e820_map *p1, struct e820_map *p2)
{
    if(!mmap_bad_type(p1->type) && mmap_bad_type(p2->type) ) {
        return p2;
    } else if(mmap_bad_type(p1->type) && !mmap_bad_type(p2->type)) {
        return p1;
    }
    return p1->type > p2->type ? p1 : p2;
}

int mmap_merge(struct e820_map *p1, struct e820_map *p2)
{
    if(!p1 || !p2 || p1->type != p2->type) return 0;

    p1->size = MMAP_END_ADDR(p2) - p1->base;
    p2->size = 0;

    return 1;
}

int mmap_split(struct e820_map *dst, 
        struct e820_map *p1, 
        struct e820_map *p2,
        const int nmemb)
{
    struct e820_map *good;
    struct e820_map *bad;
    size_t old_size;
    size_t split_size;
    int split;
    if(!p1 || !p2 || MMAP_END_ADDR(p1) - 1 < p2->base) return 0;

    bad = mmap_compare_type(p1, p2);
    good = p1 != bad ? p1 : p2;

    split = nmemb;

    /*
     * The entries are processed in descending order, and so if bad->base is
     * less than good->base, the entry must have been fully processed.
     * Therefore, it should be marked as zero size and skipped.
     */

    old_size = good->size;
    good->size = bad->base <= good->base ? 0 : bad->base - good->base;

    /* 
     * We still need a valid map, so the good entry is always updated. If
     * there's no space left, don't split off and simply return.
     */

    if(nmemb >= MMAP_MAX_ENTRIES) goto fail;

    split_size = MMAP_END_ADDR(good) - MMAP_END_ADDR(bad); 
    split_size = split_size <= old_size ? split_size : 0;
    if(split_size > 0) {
        *(dst + split++) = (struct e820_map){
                MMAP_END_ADDR(bad),
                split_size,
                good->type,
                good->attrib
        };
    }

    return split - nmemb;

fail:
    /*
     * TODO: Replace with some sort of actual bug/warn call.
     */
    puts("WARN: E820 map exhausted.");

    return 0;
}


__attribute__((__section__(".mmap")))
struct e820_map __mmap_old_map[MMAP_MAX_ENTRIES];
__attribute__((__section__(".mmap")))
struct e820_map __mmap_new_map[MMAP_MAX_ENTRIES];
static struct e820_map *const old_map = __mmap_old_map;
static struct e820_map *const new_map = __mmap_new_map;
static int old_nmemb;
static int new_nmemb;

int mmap_sanitize(struct e820_map **mmap, int nmemb)
{
    struct e820_map *overlap_map[2 * MMAP_MAX_ENTRIES];
    struct e820_map dirty_map[MMAP_MAX_ENTRIES];

    int overlaps;
    int split;
    int i;
    int j;
    int k;

    if(!mmap || !nmemb) return nmemb;

    /*
     * Preserve the old memory map and sort the dirty map. The E820 memory map
     * may be unordered, and our algorithm assumes that it is. Insertion sort is
     * chosen because in all likelihood the map is already ordered, meaning the
     * scenario is most likely the best case of O(n). In any case, the map
     * should be small enough that insertion sort still has reasonably good
     * performance. 
     */
    
    memcpy(dirty_map, *mmap, sizeof(*dirty_map) * nmemb); 
    isort(dirty_map, nmemb, sizeof(*dirty_map), mmap_cmp);

    /*
     * Construct an overlap map in order to know which conflicts to resolve.
     * E820 entries can overlap in arbitrary ways, and so the method to sanitize
     * the map will follow these steps:
     * -    create an overlap map
     * -    merge all overlapping entries of the same type
     * -    on successful merge, update overlap map with new merge
     * -    resolve overlaps of different type by splitting
     * -    insert valid dirty entries into new map
     */

    overlaps = 0;
    for(j = nmemb - 1; j >= 0; j--) {
        if(!(dirty_map + j)->size) continue;
        for(i = j - 1; i >= 0; i--) {
            if(!(dirty_map + i)->size) continue;
            if(MMAP_END_ADDR(dirty_map + i) >= (dirty_map + j)->base) {
                overlap_map[overlaps++] = dirty_map + i;
                overlap_map[overlaps++] = dirty_map + j;
            }
        }
    }

    /*
     * Now that an overlap map has been created, entries of the same type that
     * overlap are merged into single entries. The merge always happens into the
     * entry with the lower base address. The higher base address entry is
     * marked as consumed by setting its size field to zero. On a successful
     * merge, all other overlaps are updated to point to the new merge.
     */

    for(i = 0; i < overlaps; i += 2) {
        if(!mmap_merge(overlap_map[i], overlap_map[i + 1]) ) continue;
        for(j = i - 1; j >= 0; j--) {
            if(overlap_map[j] != overlap_map[i + 1]) continue;
            overlap_map[j] = overlap_map[i];
        }
    }

    /*
     * Since there are no longer overlapping entries of the same type, the
     * splitting can begin. The following loop fills out the dirty map with
     * entries that have been split off.
     */

    split = nmemb;
    for(i = 0; i < overlaps; i += 2) {
        if(!overlap_map[i]->size || !overlap_map[i + 1]->size) continue;
        split += mmap_split(&dirty_map[split],
                overlap_map[i],
                overlap_map[i + 1],
                split);
    }

    /*
     * Insert dirty map into new map in ascending order.
     */

    i = 0;
    j = split - 1;
    k = 0;
    while(i < nmemb || j >= nmemb) {
        while(i < nmemb && !dirty_map[i].size) i++;
        while(j >= nmemb && !dirty_map[j].size) j--;
        if(i < nmemb && (j < nmemb || dirty_map[i].base < dirty_map[j].base) ) {
            new_map[k++] = dirty_map[i++];
        } else if(j >= nmemb && 
                (i >= nmemb || dirty_map[j].base < dirty_map[i].base) ) {
            new_map[k++] = dirty_map[j--];
        }
    }

    *mmap = new_map;

    return k;
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

int mmap_init(struct e820_map *mmap, int nmemb)
{
    uint64_t base;
    uint64_t size;
    enum MMAP_TYPES type;

    old_nmemb = nmemb;

    /* 
     * Preserve the original memory map.
     */

    memcpy(new_map, old_map, sizeof(*new_map) * nmemb);
    mmap = new_map;

    base = (size_t)&__bios_start;
    size = &__bios_end - &__bios_start;
    type = MMAP_RESERVED;
    mmap[nmemb++] = (struct e820_map) { base, size, type, 0 }; 
    base = (size_t)&__bootloader_start; 
    size = &__bootloader_end - &__bootloader_start;
    type = MMAP_BOOTLOADER_RECLAIMABLE;
    mmap[nmemb++] = (struct e820_map) { base, size, type, 0 }; 
    base = (size_t)old_map;
    size = MMAP_TABLE_SIZE;
    mmap[nmemb++] = (struct e820_map) { base, size, type, 0 };
    base = (size_t)new_map;
    mmap[nmemb++] = (struct e820_map) { base, size, type, 0 };

    new_nmemb = mmap_sanitize(&mmap, nmemb);
    nmemb = new_nmemb;
    mmap_print(mmap, nmemb);

    return 0;
}
