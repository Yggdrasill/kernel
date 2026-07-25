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

#define PMM_BITS  (bits_sizeof(*((struct pmm_bitmap *)0)->bitmap))
#define PMM_ALIGN (1ULL << 12)
#define PMM_MASK  (~(PMM_ALIGN - 1ULL))

#define PMM_MAX(a, b) ((a) > (b) ? (a) : (b))

#define PMM_ADDR(i, b) ((i * PMM_BITS + b) * (size_t)PMM_ALIGN)

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

static uint64_t pmm_memory_sum(struct e820_info *info, struct pmm_bitmap *pmm)
{
    struct e820_map *entry;

    uint64_t align_base;
    uint64_t align_end;
    uint64_t align_blocks;
    uint64_t total_blocks;
    uint64_t usable_blocks;
    uint64_t i, j, k, l;
    size_t  *bitmap;
    size_t   pmm_max;
    uint8_t  usable;

    if(!info || !pmm || !pmm->bitmap) panic("pmm: NULL pointer!");

    bitmap  = pmm->bitmap;
    pmm_max = pmm->max_entries * PMM_BITS;
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

        j = (align_base / PMM_ALIGN) / PMM_BITS;
        k = (align_base / PMM_ALIGN) % PMM_BITS;
        l = (align_base / PMM_ALIGN);
        while(l < pmm_max && l < total_blocks) {
            bitmap[j] &= ~(1UL << k);
            if(++k >= PMM_BITS) {
                k = 0;
                j++;
            }
            l++;
        }
    }
    /* memory limit 16TiB with 4K pages */
    if((align_end / (PMM_BITS * PMM_ALIGN)) > SIZE_MAX) {
        panic("pmm_memory_sum: too many memory blocks!");
    }
    pmm->nr_entries = (size_t)(align_end / (PMM_BITS * PMM_ALIGN));
    pmm->nr_entries += (size_t)(align_end % (PMM_BITS * PMM_ALIGN)) > 0;
    if(pmm->nr_entries > pmm->max_entries) pmm->nr_entries = pmm->max_entries;
    return usable_blocks;
}

/*
static inline int
pmm_chunk_not_contiguous(const struct pmm_bitmap *pmm, size_t entry)
{
    return entry < pmm->nr_entries ? (int)(pmm->bitmap[entry] & 1) : -1;
}
*/

static size_t pmm_chunk_first_free(const struct pmm_bitmap *pmm, size_t entry)
{
    while(entry < pmm->nr_entries && pmm->bitmap[entry] == SIZE_MAX) entry++;
    return entry;
}

/*
static int
pmm_chunk_first_empty(const struct pmm_bitmap *pmm, size_t entry, size_t
*result)
{
    while(entry < pmm->nr_entries && pmm->bitmap[entry] != 0) entry++;
    *result = entry;
    return entry < pmm->nr_entries ? 0 : -1;
}
*/

static size_t pmm_block_first_free(const size_t bitmap, size_t bit)
{
    while(bit < PMM_BITS && bitmap & (1UL << bit)) bit++;
    return bit < PMM_BITS ? bit : ~0U;
}

static size_t pmm_block_contiguous_free(const size_t bitmap, size_t bit)
{
    size_t i;
    i = 0;
    while(bit < PMM_BITS && !(bitmap & (1UL << bit))) {
        i += (bitmap & (1UL << bit)) == 0;
        bit++;
    }
    return i;
}

static uintptr_t
pmm_find_n_free_at(const struct pmm_bitmap *pmm, const size_t n, size_t entry)
{
    uintptr_t addr;

    size_t bitmap;
    size_t new_free;
    size_t free;
    size_t bit;
    size_t i;

    bitmap = pmm->bitmap[entry];
    bit    = pmm_block_first_free(bitmap, 0);
    if(bit >= PMM_BITS) return 0;

    addr = 0;
    while((free = pmm_block_contiguous_free(bitmap, bit)) < PMM_BITS - bit) {
        if(n <= free) goto pmm_exit_free_at;
        bit += free + 1;
    }

    for(i = entry + 1; n > free && i < pmm->nr_entries; i++) {
        bitmap   = pmm->bitmap[i];
        new_free = pmm_block_contiguous_free(bitmap, 0);
        free += new_free;
        if(n > free && new_free < PMM_BITS) break;
    }
pmm_exit_free_at:
    if(n <= free) addr = PMM_ADDR(entry, bit);
    return addr;
}

static uintptr_t pmm_find_n_free(const struct pmm_bitmap *pmm, const size_t n)
{
    uintptr_t addr;

    size_t i;

    addr = 0;
    i    = pmm_chunk_first_free(pmm, 0);
    if(i >= pmm->nr_entries) goto pmm_no_free;
    while(i < pmm->nr_entries) {
        addr = pmm_find_n_free_at(pmm, n, i);
        if(addr) break;
        i++;
    }
pmm_no_free:
    return addr;
}

void *pmm_malloc(struct pmm_bitmap *pmm, size_t size)
{
    uintptr_t addr;
    void     *ret;
    size_t    alloc_blocks;

    if(!pmm || !size) return NULL;

    alloc_blocks = size / (size_t)PMM_ALIGN;
    alloc_blocks += (size % (size_t)PMM_ALIGN) > 0;

    if((addr = pmm_find_n_free(pmm, alloc_blocks))) {
        puts("found memory");
        puthex(&addr, sizeof(addr), 0);
        putchar(' ');
        puthex(&alloc_blocks, sizeof(alloc_blocks), 0);
        putchar('\n');
    } else {
        puts("no free memory!");
    }
    halt();

    return ret;
}

void pmm_print_map(struct pmm_bitmap *pmm)
{
    size_t entries;
    entries = 10;
    /* entries = pmm->nr_entries; */
    for(size_t i = 0; i < entries; i++) {
        puthex(&pmm->bitmap[i], sizeof(*pmm->bitmap), 0);
        putchar('\n');
    }
    return;
}

extern size_t pmm_initial[PMM_INIT_ENTRIES];

int pmm_init(struct e820_info *info)
{
    struct pmm_bitmap pmm;

    uint64_t memory;

    pmm = (struct pmm_bitmap){
        .bitmap      = pmm_initial,
        .nr_entries  = 0,
        .max_entries = PMM_INIT_ENTRIES,
    };
    memory = pmm_memory_sum(info, &pmm);
    memory = memory * PMM_ALIGN;
    /*
    puts("usable memory discovered:");
    puthex(&memory, sizeof(memory), 1);
    puts(" bytes");
    puthex(&pmm.nr_entries, sizeof(pmm.nr_entries), 1);
    puts(" entries");
    */
    pmm.bitmap[0] = 0xFFFFFFFF;
    pmm.bitmap[1] = pmm.bitmap[1] | 1;
    pmm.bitmap[1] = pmm.bitmap[1] | (1UL << 16);
    pmm.bitmap[1] = pmm.bitmap[1] | (1UL << 31);
    pmm.bitmap[3] = ~0U;
    pmm_print_map(&pmm);
    pmm_malloc(&pmm, 4096 * 33);

    return 0;
}
