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
#include <string.h>
#include <limits.h>
#include <libk/util.h>

#define PMM_ALIGN (1ULL << 12)
#define PMM_MASK  (~(PMM_ALIGN - 1ULL))

#define MMAP_END_ADDR(x) ((x)->base + (x)->size)

#define ALIGN_UP(x)   (((x) + PMM_ALIGN - 1) & PMM_MASK)
#define ALIGN_DOWN(x) ((x) & PMM_MASK)

static uint64_t pmm_memory_sum(
    struct e820_info *info,
    pmm_bitmap *pmm,
    const size_t pmm_max)
{
    uint64_t align_base;
    uint64_t align_end;
    uint64_t align_blocks;
    uint64_t total_blocks;
    uint64_t usable_blocks;
    uint64_t i, j, k, l;
    uint8_t  usable;

    const uint64_t MAX_BLOCKS = pmm_max * sizeof(*pmm) * CHAR_BIT;

    memset(pmm, 0xFF, sizeof(*pmm) * pmm_max);

    total_blocks  = 0;
    usable_blocks = 0;
    for(i = 0, j = 0; i < info->nr_entries; i++) {
        usable = info->base[i].type == MMAP_USABLE;

        align_base    = info->base[i].base;
        align_base    = usable ? ALIGN_UP(align_base) : ALIGN_DOWN(align_base);
        align_end     = MMAP_END_ADDR(info->base + i);
        align_end     = usable ? ALIGN_DOWN(align_end) : ALIGN_UP(align_end);
        align_blocks  = align_end > align_base ? (align_end - align_base) / PMM_ALIGN : 0;
        total_blocks  = total_blocks + align_blocks;

        if(!usable) continue;
        usable_blocks = usable_blocks + align_blocks;
        if(total_blocks < MAX_BLOCKS) {
            j = (align_base / PMM_ALIGN) / (sizeof(*pmm) * CHAR_BIT);
            k = (align_base / PMM_ALIGN) % (sizeof(*pmm) * CHAR_BIT);
            l = (align_base / PMM_ALIGN);
            while(j < pmm_max && l++ < total_blocks) {
                pmm[j] &= ~((pmm_bitmap)1 << k);
                if(++k >= sizeof(*pmm) * CHAR_BIT) {
                    k = 0;
                    j++;
                }
            }
        }
    }

    return usable_blocks;
}

extern pmm_bitmap pmm_initial[PMM_INIT_ENTRIES];

int pmm_init(struct e820_info *info)
{
    pmm_bitmap *pmm;
    uint64_t memory;

    pmm = pmm_initial;
    memory = pmm_memory_sum(info, pmm, PMM_INIT_ENTRIES) * PMM_ALIGN;
    puts("usable memory discovered:");
    puthex(&memory, sizeof(memory), 1);
    puts(" bytes");

    return 0;
}
