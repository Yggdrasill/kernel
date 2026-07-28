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

#define PMM_BITS (bits_sizeof(*((struct pmm_bitmap *)0)->bitmap))

#define PMM_1M_ADDR   (1 << 20)
#define PMM_1M_BLOCK  (PMM_1M_ADDR >> PMM_LOG)
#define PMM_MAX_BLOCK (SIZE_MAX >> PMM_LOG)

#define PMM_MAX(a, b) ((a) > (b) ? (a) : (b))
#define PMM_MIN(a, b) ((a) < (b) ? (a) : (b))

struct e820_usable {
    struct e820_map entries[MMAP_MAX_ENTRIES];
    size_t          nr_entries;
    size_t          max_nr_entries;
};

struct pmm_range {
    uintptr_t start;
    uintptr_t end;
};

struct pmm_map {
    struct pmm_range *usable;
    struct pmm_range *unusable;

    uint8_t nr_usable;
    uint8_t nr_unusable;
    uint8_t max_nr_usable;
    uint8_t max_nr_unusable;
};

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

    uint64_t  align_base;
    uint64_t  align_end;
    uintptr_t open_end;
    size_t    i;
    uint8_t   j, k;
    uint8_t   usable_type;

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
        if(align_base != align_end) {
            panic("pmm_parse_e820: non-contiguous memory map!");
        }

        align_end = e820->base;
        if(safe_add_uint64(&align_end, e820->base, e820->size)) {
            panic("pmm_parse_e820: integer overflow!");
        }
        align_end = pmm_align_end(align_base, align_end, e820->type);

        if(align_base > SIZE_MAX) continue;

        open_end = align_end <= SIZE_MAX ? (uintptr_t)align_end - 1 : SIZE_MAX;
        if(usable_type && j < map->max_nr_usable) {
            usable[j] = (struct pmm_range){
                .start = (uintptr_t)align_base,
                .end   = open_end,
            };
            if(align_end > align_base) j++;
        } else if(!usable_type && k < map->max_nr_unusable) {
            unusable[k] = (struct pmm_range){
                .start = (uintptr_t)align_base,
                .end   = open_end,
            };
            if(align_end > align_base) k++;
        }
    }
    map->nr_usable   = j;
    map->nr_unusable = k;

    /* detected memory limit 16TiB with 4K pages */
    if((align_end / PMM_ALIGN) > SIZE_MAX) {
        panic("pmm_parse_e820: too many memory blocks!");
    }

    return align_end / PMM_ALIGN;
}

static size_t pmm_init_bitmap(struct pmm_bitmap *pmm)
{
    struct pmm_map   *map;
    struct pmm_range *usable;

    uintptr_t base;
    uintptr_t end;
    size_t   *bitmap;
    size_t    entry;
    size_t    bit;
    size_t    block;
    size_t    free_blocks;
    size_t    i;

    if(!pmm || !pmm->bitmap || !pmm->map || !pmm->map->usable) {
        panic("pmm_init_bitmap: NULL pointer!");
    }

    if(pmm->max_entries > PMM_MAX_ENTRIES) {
        panic("pmm_init_bitmap: max_entries exceeds limit!");
    }

    const size_t max_blocks = pmm->max_entries * PMM_BITS;

    map    = pmm->map;
    usable = map->usable;

    bit         = 0;
    entry       = 0;
    free_blocks = 0;
    if(!pmm->max_entries) goto pmm_init_bitmap_exit;

    bitmap = pmm->bitmap;
    memset(bitmap, 0xFF, sizeof(*bitmap) * pmm->max_entries);

    for(i = 0; i < map->nr_usable; i++) {
        base = usable[i].start;
        end  = usable[i].end;

        block = (base / (uintptr_t)PMM_ALIGN);
        while(block < max_blocks && block <= (end / PMM_ALIGN)) {
            entry = block / (size_t)PMM_BITS;
            bit   = block % (size_t)PMM_BITS;
            bitmap[entry] &= ~(size_t)(1UL << bit);
            free_blocks++;
            block++;
        }
    }

    /*
     * The zero block is used as a null pointer in allocation code, so
     * explicitly reserve it in case the E820 sanitizer didn't. This also
     * makes an overflow impossible when multiplying by PMM_ALIGN, as a block of
     * that size has been removed from the bitmap.
     */
    if(!(pmm->bitmap[0] & 1)) free_blocks--;
    pmm->bitmap[0] |= 1;

pmm_init_bitmap_exit:
    pmm->usable_end = entry * PMM_BITS + bit + 1;
    pmm->nr_entries = entry + ((bit % PMM_BITS) > 0);
    pmm->available  = free_blocks;
    return free_blocks;
}

/*
static inline int
pmm_chunk_not_contiguous(const struct pmm_bitmap *pmm, size_t entry)
{
    return entry < pmm->nr_entries ? (int)(pmm->bitmap[entry] & 1) : -1;
}
*/

static size_t pmm_chunk_first_free(
    const struct pmm_bitmap *pmm, size_t block, const size_t end)
{
    size_t entry;
    while(block < end) {
        entry = block / PMM_BITS;
        if(pmm->bitmap[entry] != SIZE_MAX) break;
        block = (entry + 1) * PMM_BITS;
    }
    return block;
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

static size_t pmm_find_n_free_at(
    const struct pmm_bitmap *pmm,
    size_t                   block,
    const size_t             end,
    const size_t             n)
{
    size_t nr_free;
    size_t free;
    size_t entry;
    size_t bit;

    nr_free = 0;
    entry   = block / PMM_BITS;
    bit     = block % PMM_BITS;
    while(bit < PMM_BITS && pmm->bitmap[entry] & ((size_t)1 << bit)) bit++;
    if(bit >= PMM_BITS) return 0;

    free = entry * PMM_BITS + bit;
    for(block = free; n > nr_free && block < end; block++) {
        entry = block / PMM_BITS;
        bit   = block % PMM_BITS;
        if((pmm->bitmap[entry] & ((size_t)1 << bit))) break;
        nr_free++;
    }

    if(n > nr_free) return 0;
    return free;
}

static uintptr_t pmm_find_n_free(
    const struct pmm_bitmap *pmm,
    size_t                   block,
    const size_t             end,
    const size_t             n)
{
    size_t free;

    free  = 0;
    block = pmm_chunk_first_free(pmm, block, end);
    if(block >= pmm->usable_end) return free;

    while(block < end) {
        free = pmm_find_n_free_at(pmm, block, end, n);
        if(free) break;
        block++;
    }
    return free;
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

static int pmm_invalid_free(
    const struct pmm_map *map, const uintptr_t start, const uintptr_t end)
{
    uint8_t i;
    for(i = 0; i < map->nr_unusable; i++) {
        if((start >= map->unusable[i].start && start <= map->unusable[i].end) ||
           (end >= map->unusable[i].start && end <= map->unusable[i].end)) {
            return -1;
        }
    }
    return 0;
}

static struct pmm_bitmap *pmm = NULL;

void *pmm_alloc_range(size_t size, const uintptr_t base, const uintptr_t end)
{
    uintptr_t addr;
    size_t    free;
    size_t    alloc_blocks;
    size_t    from;
    size_t    to;

    if(!pmm) {
        panic("pmm_alloc_range: memory manager uninitialised!");
    }
    if(!size || end <= base) return NULL;

    alloc_blocks = size / PMM_ALIGN;
    alloc_blocks = alloc_blocks + ((size % PMM_ALIGN) > 0);

    from = base / (size_t)PMM_ALIGN;
    to   = end / (size_t)PMM_ALIGN + (end == SIZE_MAX ? 1 : 0);
    to   = PMM_MIN(to, pmm->usable_end);

    if(from >= pmm->usable_end || (from + alloc_blocks) > pmm->usable_end) {
        return NULL;
    }

    if(pmm->available < alloc_blocks) return NULL;

    free = pmm_find_n_free(pmm, from, to, alloc_blocks);
    if(!free) return NULL;

    pmm_set_n_used(pmm, free, free + alloc_blocks);
    pmm->available -= alloc_blocks;

    addr = (uintptr_t)(free * PMM_ALIGN);
    return (void *)addr;
}

void *pmm_alloc(size_t size)
{
    void *ret;

    ret = pmm_alloc_range(size, PMM_1M_ADDR, SIZE_MAX);
    if(!ret) ret = pmm_alloc_range(size, 0, SIZE_MAX);
    return ret;
}

void pmm_free_internal(void *p, size_t size, const int override)
{
    size_t alloc_blocks;
    size_t from;
    size_t to;

    int status;

    if(!pmm || !pmm->map || !pmm->map->unusable) {
        panic("pmm_free: memory manager uninitialised!");
    }

    /*
     * Definitely broken state if the input is an unaligned pointer, since alloc
     * only returns aligned ones. Since there's no memory protection the panic
     * is mandatory.
     */
    if(!p || pmm_is_unaligned((uintptr_t)p)) {
        panic("pmm_free: unaligned pointer!");
    }

    if(!size) return;

    alloc_blocks = size / PMM_ALIGN;
    alloc_blocks = alloc_blocks + ((size % PMM_ALIGN) > 0);

    if(size > UINTPTR_MAX - (uintptr_t)p) {
        panic("pmm_free: attempt to free out of range!");
    }

    if(!override &&
       pmm_invalid_free(pmm->map, (uintptr_t)p, (uintptr_t)p + size)) {
        panic("pmm_free: attempt to free reserved region!");
    }

    from = (uintptr_t)p / (uintptr_t)PMM_ALIGN;
    to   = from + alloc_blocks;
    if(from >= pmm->usable_end || to > pmm->usable_end) {
        panic("pmm_free: attempt to free past available memory!");
    }

    /* The same goes for double-free. */
    status = pmm_set_n_free(pmm, from, to);
    if(status) {
        panic("pmm_free: double-free");
    }

    pmm->available += alloc_blocks;

    return;
}

void pmm_free(void *p, size_t size)
{
    pmm_free_internal(p, size, 0);
    return;
}

extern size_t pmm_initial[PMM_INIT_ENTRIES];

int pmm_init(const struct e820_info *info)
{
    struct pmm_bitmap  initial;
    struct pmm_range   usable[MMAP_MAX_ENTRIES];
    struct pmm_range   unusable[MMAP_MAX_ENTRIES];
    struct pmm_map     map;
    struct pmm_bitmap *new;

    uint64_t blocks;
    size_t   d_available;
    size_t   entries;

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

    blocks  = pmm_parse_e820(&map, info);
    entries = (size_t)(blocks / PMM_BITS);
    entries += (size_t)((blocks % PMM_BITS) > 0);

    pmm_init_bitmap(pmm);
    d_available = pmm->available;

    puthex(&blocks, sizeof(blocks), 1);
    puts(" 4K blocks discovered");
    puthex(&pmm->available, sizeof(pmm->available), 1);
    puts(" 4K blocks available");

    new = pmm_alloc(sizeof(*new));
    if(!new) {
        panic("pmm: no space to allocate new pmm!");
    }

    new->max_entries = PMM_MAX_ENTRIES;
    new->bitmap      = pmm_alloc(sizeof(*new->bitmap) * new->max_entries);
    if(!new->bitmap) {
        panic("pmm: no space to allocate new bitmap!");
    }

    new->map = pmm_alloc(sizeof(*new->map));
    if(!new->map) {
        panic("pmm: no space to allocate new memory map!");
    }

    new->map->nr_usable       = pmm->map->nr_usable;
    new->map->max_nr_usable   = pmm->map->nr_usable;
    new->map->nr_unusable     = pmm->map->nr_unusable;
    new->map->max_nr_unusable = pmm->map->nr_unusable;

    new->map->usable   = pmm_alloc(sizeof(*usable) * new->map->nr_usable);
    new->map->unusable = pmm_alloc(sizeof(*unusable) * new->map->nr_unusable);
    if(!new->map->usable || !new->map->unusable) {
        panic("pmm: no space to allocate memory extents!");
    }
    memcpy(
        new->map->usable,
        pmm->map->usable,
        sizeof(*usable) * new->map->nr_usable);
    memcpy(
        new->map->unusable,
        pmm->map->unusable,
        sizeof(*unusable) * new->map->nr_unusable);

    d_available -= pmm->available;
    pmm_init_bitmap(new);
    entries = PMM_MIN(pmm->nr_entries, new->nr_entries);

    memcpy(new->bitmap, pmm->bitmap, sizeof(*pmm->bitmap) * entries);
    pmm = new;

    pmm_free_internal(pmm_initial, sizeof(pmm_initial), 1);
    pmm->available -= d_available;

    puthex(&pmm->available, sizeof(pmm->available), 1);
    puts(" 4K blocks available");

    return 0;
}
