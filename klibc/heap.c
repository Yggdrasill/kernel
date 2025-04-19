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

#include "stdint.h" 
#include "string.h"
#include "heap.h"

#define INDEX(H, x) ( (char *)H->tree + x * H->size ) 

/* TODO:
 * Fix possible stack overflow situation with tmp[H->size]
 */
#define SWAP(H, x, y) do {                  \
    char *__x;                              \
    char *__y;                              \
    char tmp[H->size];                      \
    __x = INDEX(H, x);                   \
    __y = INDEX(H, y);                   \
    memcpy(tmp, __x, H->size);              \
    memcpy(__x, __y, H->size);              \
    memcpy(__y, tmp, H->size);              \
} while(0)

#define COMPARE(H, x, y) ( H->compare(x, y) > 0)

#define EQUALS(H, x, y) ( H->equals(x, y) )

#define COMPAREQ(H, x, y) ( H->compare(x, y) > 0 || H->equals(x, y) )

ssize_t heap_sift_up(Heap *heap, ssize_t i)
{
    int j;

    if(!heap || i < 0) return -1;

    j = (i - 1) / 2;

    if(j != i && COMPARE(heap, INDEX(heap, i), INDEX(heap, j) ) ) {
        SWAP(heap, j, i);
        i = heap_sift_up(heap, j);
    }

    return i;
}

ssize_t heap_sift_down(Heap *heap, ssize_t i)
{
    ssize_t left;
    ssize_t right;
    ssize_t j;

    if(!heap || i < 0 || i >= heap->nmemb) return -1; 

    left = 2 * i + 1;
    right = 2 * i + 2;

    j = i;
    j = left < heap->nmemb && 
        COMPARE(heap, INDEX(heap, left), INDEX(heap, j) ) ? left : j;
    j = right < heap->nmemb && 
        COMPARE(heap, INDEX(heap, right), INDEX(heap, j) ) ? right : j;

    if(j != i) {
        SWAP(heap, j, i);
        j = heap_sift_down(heap, j);
    }

    return j;
}

Heap *heap_create(Heap *heap, 
        int (*compare)(void *, void *), 
        int (*equals)(void *, void *),
        void *array, 
        size_t size,
        ssize_t cap, 
        ssize_t nmemb)
{
    ssize_t i;

    if(!heap || !compare || !array) return NULL;

    heap->compare = compare;
    heap->equals = equals;
    heap->tree = array;
    heap->size = size;
    heap->cap = cap;
    heap->nmemb = nmemb;

    for(i = heap->nmemb / 2 - 1; i >= 0; i--) {
        heap_sift_down(heap, i);
    }

    return heap;
}

ssize_t heap_insert(Heap *heap, void *item)
{
    ssize_t i;

    if(!heap || heap->nmemb + 1 >= heap->cap) return -1;

    memcpy(INDEX(heap, heap->nmemb), item, heap->size);
    i = heap_sift_up(heap, heap->nmemb);
    heap->nmemb++;

    return i;
}

ssize_t heap_delete(Heap *heap, ssize_t i)
{
    if(!heap || i < 0 || i >= heap->nmemb) return -1;

    heap->nmemb--;
    SWAP(heap, i, heap->nmemb);
    i = heap_sift_down(heap, i);

    return i;
}

ssize_t heap_find(Heap *heap, ssize_t i, void *item)
{
    ssize_t k;

    if(!heap || i < 0 || i >= heap->nmemb) return -1;
    if(!heap->equals) return -2;
    if(EQUALS(heap, INDEX(heap, i), item) ) goto found;

    k = heap_find(heap, i * 2 + 1, item);
    i = k >= 0 ? k : i;
    if(EQUALS(heap, INDEX(heap, i), item) ) goto found;

    k = heap_find(heap, i * 2 + 2, item);
    i = k >= 0 && EQUALS(heap, INDEX(heap, k), item) ? k : i;

found:
    return i;
}

ssize_t heap_best_fit(Heap *heap, ssize_t i, void *item)
{
    ssize_t left;
    ssize_t right;

    if(!heap || i < 0 || i >= heap->nmemb) return -1;
    if(!heap->equals) return -2;
    if(COMPAREQ(heap, item, INDEX(heap, i) ) ) return i;

    left = heap_best_fit(heap, i * 2 + 1, item);
    right = heap_best_fit(heap, i * 2 + 2, item);
    left = left >= 0 ? left : i;
    right = right >= 0 ? right : i;
    left = COMPAREQ(heap, item, INDEX(heap, left) ) ? left : right;
    right = COMPAREQ(heap, item, INDEX(heap, right) ) ? right : left;

    i = COMPARE(heap, INDEX(heap, left), INDEX(heap, right) ) ? left : right;
    i = COMPAREQ(heap, item, INDEX(heap, i) ) ? i : -2;

    return i;
}
