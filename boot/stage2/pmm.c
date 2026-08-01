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

#define PMM_LOG   (12)
#define PMM_ALIGN (1ULL << PMM_LOG)
#define PMM_MASK  (~(PMM_ALIGN - 1ULL))
#define PMM_BITS  (bits_sizeof(*((struct pmm_bitmap *)0)->bitmap))

#define PMM_1M_ADDR    (1 << 20)
#define PMM_MAX_BLOCKS ((SIZE_MAX >> PMM_LOG) + 1)

#define PMM_MAX(a, b) ((a) > (b) ? (a) : (b))
#define PMM_MIN(a, b) ((a) < (b) ? (a) : (b))

struct e820_usable {
    struct e820_map entries[MMAP_MAX_ENTRIES];
    size_t          nr_entries;
    size_t          max_nr_entries;
};

struct pmm_range {
    size_t start;
    size_t end;
};

struct pmm_map {
    struct pmm_range *usable;
    struct pmm_range *unusable;

    uint8_t nr_usable;
    uint8_t nr_unusable;
    uint8_t max_nr_usable;
    uint8_t max_nr_unusable;
};

static void *pmm_alloc(size_t);
static void  pmm_free(void *, size_t);

/* clang-format off */
static SAFE_ADD_IMPLEMENT(uint64, uint64_t, UINT64_MAX)

static inline uint64_t pmm_align_up(uint64_t align)
/* clang-format on */
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

static inline int pmm_is_unaligned(uint64_t align)
{
    return align & (PMM_ALIGN - 1);
}

static inline uint64_t
pmm_align_base(uint64_t base, uint64_t end, uint32_t type)
{
    uint64_t aligned;
    aligned = type == MMAP_USABLE ? pmm_align_up(base) : pmm_align_down(base);
    return PMM_MAX(aligned, end);
}

static inline uint64_t pmm_align_end(uint64_t base, uint64_t end, uint32_t type)
{
    uint64_t aligned;
    aligned = type == MMAP_USABLE ? pmm_align_down(end) : pmm_align_up(end);
    return PMM_MAX(base, aligned);
}

static uint64_t
pmm_parse_e820(struct pmm_map *map, const struct e820_info *info)
{
    struct pmm_range *usable;
    struct pmm_range *unusable;

    uint64_t align_base;
    uint64_t align_end;
    size_t   end;
    size_t   i;
    uint8_t  j, k;
    uint8_t  usable_type;

    if(!info || !map || !map->usable || !map->unusable) {
        panic("pmm_parse_e820: NULL pointer!");
    }

    usable    = map->usable;
    unusable  = map->unusable;
    align_end = 0;

    j = 0;
    k = 0;
    for(i = 0; i < info->nr_entries; i++) {
        const struct e820_map *e820 = info->base + i;

        usable_type = e820->type == MMAP_USABLE;
        align_base  = pmm_align_base(e820->base, align_end, e820->type);

        align_end = e820->base;
        if(safe_add_uint64(&align_end, e820->base, e820->size)) {
            panic("pmm_parse_e820: integer overflow!");
        }
        align_end = pmm_align_end(align_base, align_end, e820->type);

        if(align_base > SIZE_MAX) continue;

        end = (size_t)(align_end / PMM_ALIGN);
        if(align_end >= SIZE_MAX) end = PMM_MAX_BLOCKS;

        if(usable_type && j < map->max_nr_usable) {
            usable[j] = (struct pmm_range){
                .start = (size_t)(align_base / PMM_ALIGN),
                .end   = end,
            };
            if(align_end > align_base) j++;
        } else if(!usable_type && k < map->max_nr_unusable) {
            unusable[k] = (struct pmm_range){
                .start = (size_t)(align_base / PMM_ALIGN),
                .end   = end,
            };
            if(align_end > align_base) k++;
        }
    }
    map->nr_usable   = j;
    map->nr_unusable = k;

    /* Detected memory limit 16TiB with 4K pages, just truncate */
    if((align_end / PMM_ALIGN) > SIZE_MAX) align_end = SIZE_MAX * PMM_ALIGN;

    return align_end / PMM_ALIGN;
}

static size_t pmm_init_bitmap(struct pmm_bitmap *pmm)
{
    struct pmm_map   *map;
    struct pmm_range *usable;

    size_t *bitmap;
    size_t  block;
    size_t  end;
    size_t  entry;
    size_t  bit;
    size_t  free_blocks;
    size_t  i;

    if(!pmm || !pmm->bitmap || !pmm->map || !pmm->map->usable) {
        panic("pmm_init_bitmap: NULL pointer!");
    }

    if(pmm->max_entries > PMM_MAX_ENTRIES) {
        panic("pmm_init_bitmap: max_entries exceeds limit!");
    }

    const size_t max_blocks = pmm->max_entries * PMM_BITS;

    map    = pmm->map;
    usable = map->usable;

    block       = 0;
    free_blocks = 0;

    bitmap = pmm->bitmap;
    memset(bitmap, 0xFF, sizeof(*bitmap) * pmm->max_entries);

    if(!max_blocks || !map->nr_usable || usable[0].start >= max_blocks) {
        goto pmm_init_bitmap_exit;
    }

    i = 0;
    do {
        block = usable[i].start;
        end   = usable[i].end;
        while(block < max_blocks && block < end) {
            entry = block / (size_t)PMM_BITS;
            bit   = block % (size_t)PMM_BITS;
            bitmap[entry] &= ~((size_t)1 << bit);
            free_blocks++;
            block++;
        }
    } while(++i < map->nr_usable && usable[i].start < max_blocks);

    /*
     * The zero block is used as a null pointer in allocation code, so
     * explicitly reserve it in case the E820 sanitizer didn't.
     */
    if(!(pmm->bitmap[0] & 1)) free_blocks--;
    pmm->bitmap[0] |= 1;

pmm_init_bitmap_exit:
    pmm->usable_end = block;
    pmm->nr_entries = block / PMM_BITS + ((block % PMM_BITS) > 0);
    pmm->available  = free_blocks;
    return free_blocks;
}

static size_t pmm_block_search_empty(
    const struct pmm_bitmap *pmm, const size_t start, const size_t end)
{
    size_t block;
    size_t entry;
    size_t bit;

    entry = start / PMM_BITS;
    if(!pmm->bitmap[entry]) return start;

    block = start + PMM_BITS;
    while(block < end) {
        entry = block / PMM_BITS;
        if(!pmm->bitmap[entry]) break;
        block = (entry + 1) * PMM_BITS;
    }
    if(block >= end) return 0;

    while(block > start) {
        entry = (block - 1) / PMM_BITS;
        bit   = (block - 1) % PMM_BITS;
        if(pmm->bitmap[entry] & ((size_t)1 << bit)) break;
        block--;
    }

    return block;
}

static size_t pmm_block_search_free(
    const struct pmm_bitmap *pmm, size_t block, const size_t end)
{
    size_t entry;
    size_t bit;

    while(block < end) {
        entry = block / PMM_BITS;
        if(pmm->bitmap[entry] != SIZE_MAX) break;
        block = (entry + 1) * PMM_BITS;
    }

    while(block < end) {
        entry = block / PMM_BITS;
        bit   = block % PMM_BITS;
        if(!(pmm->bitmap[entry] & ((size_t)1 << bit))) break;
        block++;
    }

    if(block >= end) return 0;
    return block;
}

static size_t pmm_find_n_free(
    const struct pmm_bitmap *pmm,
    size_t                   block,
    const size_t             end,
    const size_t             n)
{
    size_t empty;
    size_t nr_free;
    size_t entry;
    size_t bit;
    size_t i;

    empty   = 0;
    nr_free = 0;
    while(block < end) {
        if(n >= 2 * PMM_BITS) empty = pmm_block_search_empty(pmm, block, end);
        if(!empty) block = pmm_block_search_free(pmm, block, end);
        else block = empty;

        if(!block) return 0;

        nr_free = 0;
        for(i = block; n > nr_free && i < end; i++) {
            entry = i / PMM_BITS;
            bit   = i % PMM_BITS;
            if((pmm->bitmap[entry] & ((size_t)1 << bit))) break;
            nr_free++;
        }
        if(nr_free >= n) break;
        block = i;
    }
    if(n > nr_free) return 0;
    return block;
}

static void
pmm_set_n_used(struct pmm_bitmap *pmm, size_t block, const size_t end)
{
    size_t entry;
    size_t bit;

    while(block < end) {
        entry = block / PMM_BITS;
        bit   = block % PMM_BITS;
        pmm->bitmap[entry] |= ((size_t)1 << bit);
        block++;
    }

    return;
}

static int
pmm_set_n_free(struct pmm_bitmap *pmm, size_t block, const size_t end)
{
    size_t entry;
    size_t bit;

    entry = block / PMM_BITS;
    bit   = block % PMM_BITS;
    if(!(pmm->bitmap[entry] & ((size_t)1 << bit))) return -1;

    while(block < end) {
        entry = block / PMM_BITS;
        bit   = block % PMM_BITS;
        pmm->bitmap[entry] &= ~((size_t)1 << bit);
        block++;
    }

    return 0;
}

#if 0

static void
pmm_print_map(const struct pmm_bitmap *pmm, size_t entry, const size_t end)
{
    while(entry < end) {
        puthex(&pmm->bitmap[entry], sizeof(*pmm->bitmap), 0);
        putchar('\n');
        entry++;
    }
    return;
}

#endif

static int
pmm_search_map(const struct pmm_map *map, const size_t start, const size_t end)
{
    int i;
    for(i = 0; i < map->nr_unusable; i++) {
        if(map->unusable[i].start == map->unusable[i].end) continue;
        if(start < map->unusable[i].end && end > map->unusable[i].start) {
            return i;
        }
    }
    return -1;
}

static int pmm_reclaim_unusable(struct pmm_map *map, void *p, const size_t size)
{
    struct pmm_range *old;
    struct pmm_range *new;

    size_t p_off;
    size_t s_off;
    size_t from;
    size_t to;
    int    i;

    if(!map || !map->unusable) return -1;
    if(!size) return 0;

    from  = (uintptr_t)p / PMM_ALIGN;
    p_off = (uintptr_t)p % PMM_ALIGN;
    s_off = size % PMM_ALIGN;
    to    = from + size / (size_t)PMM_ALIGN;
    to += ((p_off + s_off) % PMM_ALIGN) > 0;
    to += (p_off + s_off) / PMM_ALIGN;

    old = map->unusable;

    i = pmm_search_map(map, from, to);
    if(i < 0) return 0;

    /* Zero-sized entries are fine. */
    if(from <= old[i].start && to >= old[i].end) {
        old[i].start = old[i].end;
        return 0;
    }

    if(to >= old[i].end) {
        old[i].end = from;
        return 0;
    }

    if(from <= old[i].start && to > old[i].start) {
        old[i].start = to;
        return 0;
    }

    if(map->nr_unusable == map->max_nr_unusable) return -2;

    new = pmm_alloc(sizeof(*old) * (map->nr_unusable + 1));
    if(!new) return -3;

    memcpy(new, old, sizeof(*old) * (size_t)i);
    memcpy(
        new + i + 2,
        old + i + 1,
        sizeof(*old) * (map->nr_unusable - (size_t)i - 1));

    new[i] = (struct pmm_range){
        .start = old[i].start,
        .end   = from,
    };
    new[i + 1] = (struct pmm_range){
        .start = to,
        .end   = old[i].end,
    };

    pmm_free(old, sizeof(*old) * map->nr_unusable);
    map->unusable = new;
    map->nr_unusable++;

    return 0;
}

static struct pmm_bitmap *pmm = NULL;

void *pmm_alloc_range(size_t size, const uintptr_t base, const uintptr_t end)
{
    uintptr_t addr;
    size_t    free;
    size_t    alloc;
    size_t    from;
    size_t    to;

    if(!pmm) {
        panic("pmm_alloc_range: memory manager uninitialised!");
    }
    if(!size || end <= base) return NULL;

    alloc = size / PMM_ALIGN;
    alloc = alloc + ((size % PMM_ALIGN) > 0);

    from = base / (size_t)PMM_ALIGN + ((base % (size_t)PMM_ALIGN) > 0);
    to   = end / (size_t)PMM_ALIGN + (end == UINTPTR_MAX ? 1 : 0);
    to   = PMM_MIN(to, pmm->usable_end);

    if(from + alloc > pmm->usable_end) return NULL;
    if(pmm->available < alloc) return NULL;

    free = pmm_find_n_free(pmm, from, to, alloc);
    if(!free) return NULL;

    pmm_set_n_used(pmm, free, free + alloc);
    pmm->available -= alloc;

    addr = (uintptr_t)(free * PMM_ALIGN);
    return (void *)addr;
}

void *pmm_alloc(size_t size)
{
    void *ret;

    ret = pmm_alloc_range(size, PMM_1M_ADDR, UINTPTR_MAX);
    if(!ret) ret = pmm_alloc_range(size, 0, UINTPTR_MAX);
    return ret;
}

void pmm_free(void *p, size_t size)
{
    size_t block;
    size_t alloc;

    if(!size) return;

    if(!pmm || !pmm->map || !pmm->map->unusable) {
        panic("pmm_free: memory manager uninitialised!");
    }
    if(!p) panic("pmm_free: NULL pointer!");

    /*
     * Definitely broken state if the input is an unaligned pointer, since alloc
     * only returns aligned ones. Since there's no memory protection the panic
     * is mandatory.
     */
    if(pmm_is_unaligned((uintptr_t)p)) {
        panic("pmm_free: unaligned pointer!");
    }

    block = (uintptr_t)p / PMM_ALIGN;
    alloc = size / PMM_ALIGN;
    alloc = alloc + ((size % PMM_ALIGN) > 0);

    if(block + alloc > pmm->usable_end) {
        panic("pmm_free: attempt to free past available memory!");
    }

    if(pmm_search_map(pmm->map, block, block + alloc) >= 0) {
        panic("pmm_free: attempt to free reserved region!");
    }

    if(pmm_set_n_free(pmm, block, block + alloc)) {
        panic("pmm_free: double-free");
    }

    pmm->available += alloc;

    return;
}

extern size_t pmm_initial[PMM_INIT_ENTRIES];

static struct pmm_bitmap *pmm_init_new(const struct pmm_bitmap *old)
{
    struct pmm_bitmap *new;
    struct pmm_range  *usable;
    struct pmm_range  *unusable;

    size_t entries;

    if(!old) panic("pmm_init_new: NULL pointer!");

    new = pmm_alloc(sizeof(*new));
    if(!new) panic("pmm: no space to allocate new pmm!");

    new->max_entries = PMM_MAX_ENTRIES;
    new->bitmap      = pmm_alloc(sizeof(*new->bitmap) * new->max_entries);
    if(!new->bitmap) panic("pmm: no space to allocate new bitmap!");

    new->map = pmm_alloc(sizeof(*new->map));
    if(!new->map) panic("pmm: no space to allocate new memory map!");

    new->map->nr_usable       = old->map->nr_usable;
    new->map->max_nr_usable   = MMAP_MAX_ENTRIES;
    new->map->nr_unusable     = old->map->nr_unusable;
    new->map->max_nr_unusable = MMAP_MAX_ENTRIES;

    new->map->usable   = pmm_alloc(sizeof(*usable) * new->map->nr_usable);
    new->map->unusable = pmm_alloc(sizeof(*unusable) * new->map->nr_unusable);
    if(!new->map->usable || !new->map->unusable) {
        panic("pmm: no space to allocate memory extents!");
    }

    memcpy(
        new->map->usable,
        old->map->usable,
        sizeof(*usable) * new->map->nr_usable);
    memcpy(
        new->map->unusable,
        old->map->unusable,
        sizeof(*unusable) * new->map->nr_unusable);

    pmm_init_bitmap(new);
    entries = PMM_MIN(old->nr_entries, new->nr_entries);
    memcpy(new->bitmap, old->bitmap, sizeof(*old->bitmap) * entries);

    return new;
}

int pmm_init(const struct e820_info *info)
{
    struct pmm_bitmap initial;
    struct pmm_range  usable[MMAP_MAX_ENTRIES];
    struct pmm_range  unusable[MMAP_MAX_ENTRIES];
    struct pmm_map    map;

    size_t d_available;

    map = (struct pmm_map){
        .usable          = usable,
        .unusable        = unusable,
        .nr_usable       = 0,
        .nr_unusable     = 0,
        .max_nr_usable   = MMAP_MAX_ENTRIES,
        .max_nr_unusable = MMAP_MAX_ENTRIES,
    };

    initial = (struct pmm_bitmap){
        .map         = &map,
        .bitmap      = pmm_initial,
        .nr_entries  = 0,
        .max_entries = PMM_INIT_ENTRIES,
    };
    pmm = &initial;

    pmm_parse_e820(&map, info);
    pmm_init_bitmap(pmm);

    d_available = pmm->available;
    pmm         = pmm_init_new(&initial);
    d_available -= initial.available;

    if(!pmm_reclaim_unusable(pmm->map, pmm_initial, sizeof(pmm_initial))) {
        pmm_free(pmm_initial, sizeof(pmm_initial));
    } else {
        puts("WARN pmm_init: cannot free initial bitmap");
    }
    pmm->available -= d_available;

    return 0;
}
