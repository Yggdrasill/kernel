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

#define PMM_BITS (bits_sizeof(*((struct pmm_bitmap *)0)->bitmap))

#define PMM_MAX(a, b) ((a) > (b) ? (a) : (b))
#define PMM_MIN(a, b) ((a) < (b) ? (a) : (b))

struct e820_usable {
    struct e820_map entries[MMAP_MAX_ENTRIES];
    size_t          nr_entries;
    size_t          max_nr_entries;
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

static inline uint64_t
pmm_align_base(const uint64_t base, const uint64_t end, const uint32_t type)
{
    uint64_t aligned;
    aligned = type == MMAP_USABLE ? pmm_align_up(base) : pmm_align_down(base);
    return PMM_MAX(aligned, end);
}

static inline uint64_t
pmm_align_end(const uint64_t base, const uint64_t end, const uint32_t type)
{
    uint64_t aligned;
    aligned = type == MMAP_USABLE ? pmm_align_down(end) : pmm_align_up(end);
    return PMM_MAX(base, aligned);
}

static uint64_t
pmm_parse_e820(struct e820_info *info, struct e820_usable *usable)
{
    struct e820_map *e820;

    uint64_t align_base;
    uint64_t align_end;
    size_t   i, j;

    if(!info || !usable) panic("pmm_parse_e820: NULL pointer!");

    align_end = 0;

    j = 0;
    for(i = 0; i < info->nr_entries; i++) {
        e820 = info->base + i;

        align_base = pmm_align_base(e820->base, align_end, e820->type);
        if(align_base != align_end) {
            panic("pmm_parse_e820: non-contiguous memory map!");
        }

        align_end = e820->base;
        if(safe_add_uint64(&align_end, e820->base, e820->size)) {
            panic("pmm_parse_e820: integer overflow!");
        }
        align_end = pmm_align_end(align_base, align_end, e820->type);

        if(e820->type == MMAP_USABLE && j < usable->max_nr_entries) {
            usable->entries[j++] = (struct e820_map){
                .base   = align_base,
                .size   = align_end - align_base,
                .type   = e820->type,
                .attrib = e820->attrib,
            };
        }
    }
    usable->nr_entries = j;

    /* detected memory limit 16TiB with 4K pages */
    if((align_end / PMM_ALIGN) > SIZE_MAX) {
        panic("pmm_parse_e820: too many memory blocks!");
    }

    return align_end / PMM_ALIGN;
}

static size_t
pmm_init_bitmap(struct pmm_bitmap *pmm, struct e820_usable *usable)
{
    struct e820_map *e820;

    uint64_t base;
    uint64_t end;
    uint64_t entry;
    uint64_t bit;
    uint64_t j;
    size_t   i;
    size_t   free_blocks;
    size_t  *bitmap;

    if(!pmm || !pmm->bitmap || !usable) panic("pmm_init_bitmap: NULL pointer!");
    if(pmm->max_entries > PMM_MAX_ENTRIES) {
        panic("pmm_init_bitmap: max_entries exceeds limit!");
    }

    const uint64_t bits_max = pmm->max_entries * PMM_BITS;

    free_blocks = 0;
    if(!pmm->max_entries) goto pmm_init_bitmap_exit;

    bitmap = pmm->bitmap;
    memset(bitmap, 0xFF, sizeof(*bitmap) * pmm->max_entries);

    for(i = 0; i < usable->nr_entries; i++) {
        e820 = usable->entries + i;
        base = e820->base;
        end  = e820->base + e820->size;

        j     = (base / PMM_ALIGN);
        entry = (base / PMM_ALIGN) / PMM_BITS;
        bit   = (base / PMM_ALIGN) % PMM_BITS;
        while(j < bits_max && j < (end / PMM_ALIGN)) {
            bitmap[entry] &= ~(size_t)(1UL << bit);
            if(++bit >= PMM_BITS) {
                bit = 0;
                entry++;
            }
            j++;
            free_blocks++;
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
    return free_blocks * PMM_ALIGN;
}

static inline size_t pmm_select_entry(uintptr_t addr)
{
    return (size_t)(addr / (uintptr_t)(PMM_ALIGN * PMM_BITS));
}

static inline size_t pmm_select_bit(uintptr_t addr)
{
    return (size_t)((addr / (uintptr_t)PMM_ALIGN) % (uintptr_t)PMM_BITS);
}

static inline uintptr_t pmm_calc_addr(size_t entry, size_t bit)
{
    return (uintptr_t)((entry * PMM_BITS + bit) * (size_t)PMM_ALIGN);
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
    while(bit < PMM_BITS && bitmap & ((size_t)1 << bit)) bit++;
    return bit < PMM_BITS ? bit : ~0U;
}

static size_t pmm_block_contiguous_free(const size_t bitmap, size_t bit)
{
    size_t i;
    i = 0;
    while(bit < PMM_BITS && !(bitmap & ((size_t)1 << bit))) {
        i += (bitmap & ((size_t)1 << bit)) == 0;
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
    if(n <= free) addr = pmm_calc_addr(entry, bit);
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

static void pmm_set_n_used(
    const struct pmm_bitmap *pmm, const size_t n, const uintptr_t addr)
{
    size_t entry;
    size_t bit;
    size_t i;

    entry = pmm_select_entry(addr);
    bit   = pmm_select_bit(addr);
    for(i = 0; i < n; i++) {
        pmm->bitmap[entry] |= ((size_t)1 << bit);
        if(++bit >= PMM_BITS) {
            bit = 0;
            entry++;
        }
    }

    return;
}

static void pmm_set_n_free(
    const struct pmm_bitmap *pmm, const size_t n, const uintptr_t addr)
{
    size_t entry;
    size_t bit;
    size_t i;

    entry = pmm_select_entry(addr);
    bit   = pmm_select_bit(addr);
    for(i = 0; i < n; i++) {
        pmm->bitmap[entry] &= ~((size_t)1 << bit);
        if(++bit >= PMM_BITS) {
            bit = 0;
            entry++;
        }
    }

    return;
}

static void
pmm_print_map(struct pmm_bitmap *pmm, const size_t start, const size_t end)
{
    size_t i;
    for(i = start; i < end; i++) {
        puthex(&pmm->bitmap[i], sizeof(*pmm->bitmap), 0);
        putchar('\n');
    }
    return;
}

static struct pmm_bitmap *pmm = NULL;

void *pmm_alloc(size_t size)
{
    uintptr_t addr;
    size_t    alloc_blocks;
    size_t    alloc_bytes;

    if(!pmm || !size) return NULL;
    if(size > SIZE_MAX / (size_t)PMM_ALIGN) return NULL;

    alloc_blocks = size / PMM_ALIGN;
    alloc_blocks = alloc_blocks + ((size % PMM_ALIGN) > 0);
    alloc_bytes  = alloc_blocks * PMM_ALIGN;

    if(pmm->available < alloc_bytes) return NULL;

    addr = pmm_find_n_free(pmm, alloc_blocks);
    if(!addr) return NULL;

    pmm_set_n_used(pmm, alloc_blocks, addr);
    pmm->available -= alloc_bytes;
    return (void *)addr;
}

void pmm_free(void *p, size_t size)
{
    uintptr_t addr;
    size_t    alloc_blocks;

    if(!pmm || !p || !size) return;
    if(size > SIZE_MAX / (size_t)PMM_ALIGN) return;

    alloc_blocks = size / (size_t)PMM_ALIGN;
    alloc_blocks += (size % (size_t)PMM_ALIGN) > 0;

    addr = (uintptr_t)p;
    pmm_set_n_free(pmm, alloc_blocks, addr);
    pmm->available += (alloc_blocks * (size_t)PMM_ALIGN);

    return;
}

extern size_t pmm_initial[PMM_INIT_ENTRIES];

int pmm_init(struct e820_info *info)
{
    struct e820_usable usable;
    struct pmm_bitmap  initial;
    struct pmm_bitmap *new;

    uint64_t blocks;
    size_t   d_available;
    size_t   entries;

    usable = (struct e820_usable){
        .nr_entries     = 0,
        .max_nr_entries = MMAP_MAX_ENTRIES,
    };

    initial = (struct pmm_bitmap){
        .bitmap      = pmm_initial,
        .nr_entries  = 0,
        .max_entries = PMM_INIT_ENTRIES,
    };
    pmm = &initial;

    blocks  = pmm_parse_e820(info, &usable);
    entries = (size_t)(blocks / PMM_BITS);
    entries += (size_t)((blocks % PMM_BITS) > 0);

    pmm->nr_entries = PMM_MIN(entries, pmm->max_entries);
    pmm->available  = pmm_init_bitmap(pmm, &usable);
    d_available     = pmm->available;

    puthex(&blocks, sizeof(blocks), 1);
    puts(" 4K blocks discovered");

    new = pmm_alloc(sizeof(*new));
    if(!new) {
        panic("pmm: no space to allocate new pmm!");
    }

    new->max_entries = PMM_MAX_ENTRIES;
    new->nr_entries  = PMM_MIN(entries, PMM_MAX_ENTRIES);
    new->bitmap      = pmm_alloc(sizeof(*new->bitmap) * new->max_entries);
    d_available -= pmm->available;
    if(!new->bitmap) {
        panic("pmm: no space to allocate new bitmap!");
    }

    new->available = pmm_init_bitmap(new, &usable);
    entries        = PMM_MIN(PMM_INIT_ENTRIES, entries);

    memcpy(new->bitmap, pmm->bitmap, sizeof(*pmm->bitmap) * entries);
    pmm = new;
    pmm_free(initial.bitmap, sizeof(*initial.bitmap) * PMM_INIT_ENTRIES);
    pmm->available -= d_available;

    puthex(&pmm->available, sizeof(pmm->available), 1);
    puts(" bytes available");

    return 0;
}
