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

#ifndef HEAP_H
#define HEAP_H

#include "stdint.h"

#define DEFAULT_HEAP_SIZE 8192

typedef struct heap {
    int (*compare)(void *, void *);
    int (*equals)(void *, void *);
    void *tree;
    size_t size;
    ssize_t cap;
    ssize_t nmemb;
} Heap;

ssize_t heap_sift_up(Heap *, ssize_t);
ssize_t heap_sift_down(Heap *, ssize_t);
Heap *heap_create(Heap *,
        int (*compare)(void *, void *), 
        int (*equals)(void *, void *), 
        void *, 
        size_t,
        ssize_t, 
        ssize_t);
ssize_t heap_insert(Heap *, void *);
ssize_t heap_delete(Heap *, ssize_t);
ssize_t heap_find(Heap *, ssize_t, void *);
ssize_t heap_best_fit(Heap *, ssize_t, void *);

#endif
