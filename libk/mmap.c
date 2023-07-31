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

    p1->size = p1->size + p2->size - (MMAP_END_ADDR(p1) - p2->base);
    p2->size = 0;

    return 1;
}

int mmap_split(struct e820_map *dst, 
        struct e820_map *p1, 
        struct e820_map *p2)
{
    struct e820_map *good;
    struct e820_map *bad;
    struct e820_map *ptr;
    size_t size;
    if(!p1 || !p2 || MMAP_END_ADDR(p1) - 1 < p2->base) return 0;

    ptr = dst;
    bad = mmap_compare_type(p1, p2);
    good = p1 != bad ? p1 : p2;

    size = MMAP_END_ADDR(good) - MMAP_END_ADDR(bad); 
    size = size <= good->size ? size : 0;
    if(size > 0) {
        *ptr++ = (struct e820_map){
                MMAP_END_ADDR(bad),
                size,
                good->type,
                good->attrib
        };
    }

    /*
     * The entries are processed in descending order, and so if bad->base is
     * less than good->base, the entry must have been fully processed.
     * Therefore, it should be marked as zero size and skipped.
     */
    good->size = bad->base <= good->base ? 0 : bad->base - good->base;

    return ptr - dst;
}

struct e820_map *old_map;
struct e820_map *new_map;
int old_nmemb;
int new_nmemb;

size_t mmap_sanitize(struct e820_map **mmap, int nmemb)
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
                overlap_map[i + 1]);
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

    old_map = mmap;
    old_nmemb = nmemb;
    new_map = &__mmap_new_map;

    /* 
     * Preserve the original memory map.
     */
    memcpy(new_map, old_map, sizeof(*new_map) * nmemb);
    mmap = new_map;

    base = (size_t)old_map;
    size = MMAP_TABLE_SIZE;
    type = MMAP_BOOTLOADER_RECLAIMABLE;
    mmap[nmemb++] = (struct e820_map) { base, size, type, 0 };
    base = (size_t)new_map;
    mmap[nmemb++] = (struct e820_map) { base, size, type, 0 };

    new_nmemb = mmap_sanitize(&mmap, nmemb);
    nmemb = new_nmemb;
    mmap_print(mmap, nmemb);

    return 0;
}
