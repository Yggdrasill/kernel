#include <stdint.h>
#include <string.h>
#include <sort.h>
#include <mmap.h>

#define MMAP_END_ADDR(x) ( (x)->base + (x)->size)

int mmap_cmp(const void *p1, const void *p2)
{
    return ( (struct e820_map *)p1) ->base > ( (struct e820_map *)p2)->base;
}

struct e820_map *mmap_compare_type(struct e820_map *p1, struct e820_map *p2)
{
    if(!p1 || !p2) return NULL;

    return p1->type > p2->type ? p1 : p2;
}

int mmap_match_type(struct e820_map *p1, struct e820_map *p2)
{
    return p1->type != p2->type;
}

int mmap_addr_overlap(struct e820_map *p1, struct e820_map *p2)
{
    return MMAP_END_ADDR(p1) - 1 < p2->base;
}

int mmap_match_entry(struct e820_map *p1, struct e820_map *p2)
{
    return p1->base != p2->base || p1->size != p2->size;
}

struct e820_map *mmap_merge(struct e820_map *p1, struct e820_map *p2)
{
    struct e820_map *rv;

    if(!p1 || !p2) return NULL;
    if(MMAP_END_ADDR(p1) < p2->base) return NULL;
    if(mmap_match_type(p1, p2) && mmap_match_entry(p1, p2) ) return NULL;

    rv = p1;
    if(!mmap_match_type(p1, p2) ) {
        p1->size = p1->size + p2->size - (MMAP_END_ADDR(p1) - p2->base);
        p2->size = 0;
    } else {
        rv = mmap_compare_type(p1, p2);
        p1 = p1 != rv ? p1 : p2;
        p1->size = 0;
    }

    return rv;
}

void mmap_split(struct e820_map *dst, 
        struct e820_map *p1, 
        struct e820_map *p2)
{
    struct e820_map *good;
    struct e820_map *bad;
    size_t size;
    if(MMAP_END_ADDR(p1) - 1 < p2->base || !mmap_match_type(p1, p2) ) return;

    bad = mmap_compare_type(p1, p2);
    good = p1 != bad ? p1 : p2;
    if(bad->base <= good->base) {
        memcpy(dst, bad, sizeof(*dst) );
        size = MMAP_END_ADDR(good) - MMAP_END_ADDR(bad);
        size = size <= good->size ? size : 0;
        good->base = MMAP_END_ADDR(bad);
        good->size = size;
    } else {
        memcpy(dst, good, sizeof(*dst) );
        dst->size = bad->base - good->base;
        good->base = MMAP_END_ADDR(bad);
        size = good->size - dst->size - bad->size;
        good->size = size <= good->size ? size : 0;
        dst++;
        memcpy(dst, bad, sizeof(*dst) );
    }
    
    bad->size = 0;

    return;
}

size_t mmap_sanitize(struct e820_map *mmap, int nmemb)
{
    struct e820_map *overlap_map[2 * MMAP_MAX_ENTRIES];
    struct e820_map clean_map[MMAP_MAX_ENTRIES];
    struct e820_map new_map[MMAP_MAX_ENTRIES];
    int overlaps;
    int clean;
    int i;
    int j;
    int k;

    if(!mmap || !nmemb) return nmemb;

    /*
     * Construct an overlap map in order to know which conflicts to resolve.
     * E820 entries can overlap in arbitrary ways, and so the method to sanitize
     * the map will follow these steps:
     * -    create an overlap map
     * -    merge all overlapping entries of the same type
     * -    resolve overlaps of different type by splitting
     * -    merge overlap_map and mmap into new_map
     * -    copy new_map to mmap
     */

    overlaps = 0;
    for(j = nmemb - 1; j >= 0; j--) {
        if(!(mmap + j)->size) continue;
        for(i = j - 1; i >= 0; i--) {
            if(!(mmap + i)->size) continue;
            if(MMAP_END_ADDR(mmap + i) >= (mmap + j)->base) {
                overlap_map[overlaps++] = mmap + i;
                overlap_map[overlaps++] = mmap + j;
            }
        }
    }

    for(i = 0; i < overlaps; i += 2) {
        if(mmap_merge(overlap_map[i], overlap_map[i + 1]) ) {
            for(j = i - 1; j >= 0; j--) {
                if(overlap_map[j] != overlap_map[i + 1]) continue;
                overlap_map[j] = overlap_map[i];
            }
        }
    }

    clean = 0;
    for(i = overlaps - 1; i > 0; i -= 2) {
        if(!overlap_map[i - 1]->size || !overlap_map[i]->size) continue;
        mmap_split(&clean_map[clean], overlap_map[i - 1], overlap_map[i]);
        clean += 2;
    }

    i = 0;
    j = 0;
    k = 0;
    while(i < nmemb || j < clean) {
        while(i < nmemb && !mmap[i].size) i++;
        while(j < clean && !clean_map[j].size) j++;
        if(i < nmemb && (j >= clean || mmap[i].base < clean_map[j].base) ) {
            memcpy(&new_map[k], &mmap[i], sizeof(new_map[i]) );
            i++;
            k++;
        } else if(j < clean && 
                (i >= nmemb || clean_map[j].base < mmap[i].base) ) {
            memcpy(&new_map[k], &clean_map[j], sizeof(new_map[j]) );
            j++;
            k++;
        }
    }

    memcpy(mmap, new_map, sizeof(*mmap) * k); 

    return k;
}

int mmap_init(struct e820_map *mmap, int nmemb)
{
    /* E820 map may be unordered */
    isort(mmap, nmemb, sizeof(*mmap), mmap_cmp);
    nmemb = mmap_sanitize(mmap, nmemb);

    return 0;
}
