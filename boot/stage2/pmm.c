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
#include <string.h>

#define PMM_ALIGN (2 << 11)
#define PMM_MASK  (~(PMM_ALIGN - 1))

#define MMAP_END_ADDR(x) ((x)->base + (x)->size)

static uint64_t pmm_memory_sum(struct e820_info *info)
{
    uint64_t aligned_base;
    uint64_t aligned_end;
    uint64_t aligned_sum;
    size_t   i;

    aligned_sum = 0;
    for(i = 0; i < info->nr_entries; i++) {
        if(info->base[i].type != MMAP_USABLE) continue;
        aligned_base = (info->base[i].base + PMM_ALIGN - 1) & PMM_MASK;
        aligned_end  = MMAP_END_ADDR(info->base + i) & PMM_MASK;
        if(aligned_end > aligned_base) {
            aligned_sum += aligned_end - aligned_base;
        }
    }

    return aligned_sum;
}

int pmm_init(struct e820_info *info)
{
    uint64_t memory;

    memory = pmm_memory_sum(info);
    puts("Available memory:");
    puthex(&memory, sizeof(memory), 0);
    puts(" bytes");

    return 0;
}
