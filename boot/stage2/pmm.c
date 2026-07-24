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

#include "pmm.h"

#include <libk/internal/mmap.h>
#include <libk/util.h>
#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <string.h>

#define bits_sizeof(x) (sizeof(x) * CHAR_BIT)

#define PMM_ALIGN (1ULL << 12)
#define PMM_MASK  (~(PMM_ALIGN - 1ULL))

#define PMM_MAX(a, b) ((a) > (b) ? (a) : (b))

static SAFE_ADD_IMPLEMENT(uint64, uint64_t, UINT64_MAX)

static inline uint64_t pmm_align_up(uint64_t align)
{
    if(safe_add_uint64(&align, align, PMM_ALIGN - 1)) {
        panic("pmm_align_up: integer overflow!");
    }
    return align & PMM_MASK;
}

static inline uint64_t pmm_align_down(uint64_t align)
{
    return align & PMM_MASK;
}

static uint64_t
pmm_memory_sum(struct e820_info *info, struct pmm_bitmap *pmm)
{
    struct e820_map *entry;

    uint64_t  align_base;
    uint64_t  align_end;
    uint64_t  align_blocks;
    uint64_t  total_blocks;
    uint64_t  usable_blocks;
    uint64_t  i, j, k, l;
    size_t   *bitmap;
    size_t    pmm_max;
    uint8_t   usable;

    if(!info || !pmm || !pmm->bitmap) panic("pmm: NULL pointer!");

    bitmap  = pmm->bitmap;
    pmm_max = pmm->max_entries * bits_sizeof(*pmm->bitmap);
    memset(bitmap, 0xFF, sizeof(*bitmap) * pmm->max_entries);

    align_end     = 0;
    total_blocks  = 0;
    usable_blocks = 0;

    j = 0;
    for(i = 0; i < info->nr_entries; i++) {
        entry  = info->base + i;
        usable = entry->type == MMAP_USABLE;

        align_base = entry->base;
        align_base =
            usable ? pmm_align_up(align_base) : pmm_align_down(align_base);
        align_base = PMM_MAX(align_base, align_end);

        align_end = entry->base;
        if(safe_add_uint64(&align_end, entry->base, entry->size)) {
            panic("pmm_memory_sum: integer overflow!");
        }

        align_end =
            usable ? pmm_align_down(align_end) : pmm_align_up(align_end);
        align_end = PMM_MAX(align_base, align_end);

        align_blocks = (align_end - align_base) / PMM_ALIGN;
        total_blocks = total_blocks + align_blocks;

        if(!usable) continue;
        usable_blocks = usable_blocks + align_blocks;

        j = (align_base / PMM_ALIGN) / bits_sizeof(*bitmap);
        k = (align_base / PMM_ALIGN) % bits_sizeof(*bitmap);
        l = (align_base / PMM_ALIGN);
        while(l < pmm_max && l < total_blocks) {
            bitmap[j] &= ~((size_t)1 << k);
            if(++k >= bits_sizeof(*bitmap)) {
                k = 0;
                j++;
            }
            l++;
        }
    }

    /* memory limit 16TiB with 4K pages */
    if((align_end / (bits_sizeof(*bitmap) * PMM_ALIGN)) > SIZE_MAX) {
        panic("pmm_memory_sum: too many memory blocks!");
    }
    pmm->nr_entries = (size_t)(align_end / (bits_sizeof(*bitmap) * PMM_ALIGN));
    if(pmm->nr_entries > pmm->max_entries) pmm->nr_entries = pmm->max_entries;
    return usable_blocks;
}

extern size_t pmm_initial[PMM_INIT_ENTRIES];

int pmm_init(struct e820_info *info)
{
    struct pmm_bitmap pmm;

    uint64_t memory;

    pmm.bitmap = pmm_initial;
    pmm.nr_entries = 0;
    pmm.max_entries = PMM_INIT_ENTRIES;
    memory = pmm_memory_sum(info, &pmm) * PMM_ALIGN;
    puts("usable memory discovered:");
    puthex(&memory, sizeof(memory), 1);
    puts(" bytes");
    puthex(&pmm.nr_entries, sizeof(pmm.nr_entries), 1);
    puts(" entries");

    return 0;
}
