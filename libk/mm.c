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

#include <libk/common.h>
#include <libk/util.h>

#include <list.h>
#include <stddef.h>
#include <stdint.h>

#ifdef BOOTLOADER_COMPILE
extern void *pmm_alloc(size_t);
    #define alloc pmm_alloc
#endif

#define INIT_NR_NODES   (PAGE_ALIGN / sizeof(struct list_node) - 2)
#define HEAP_ALLOC_SIZE (256 * (1 << 10))
#define MAGIC_HEADER    0x807FAA55

struct mm_used {
    struct list_node node;
    struct mm_arena *arena;
    size_t           size;
};

struct mm_free {
    struct list_node node;
    struct mm_arena *arena;
    size_t           size;
};

struct mm_arena {
    struct list_node node;
    size_t           size;
    size_t           used;
};

struct mm_state {
    struct list_root arenas;
    struct list_root memory;
    size_t           size;
    size_t           used;
};

static struct mm_state mm;

int mm_init(const size_t available)
{
    struct mm_arena *arena;
    struct mm_free  *free;

    void *mem;

    list_init(&mm.arenas);
    list_init(&mm.memory);

    mem = alloc(HEAP_ALLOC_SIZE);
    if(!mem) return -1;

    arena = mem;
    list_push(&mm.arenas, &arena->node);
    arena->size = HEAP_ALLOC_SIZE;
    arena->used = sizeof(*arena) + sizeof(*free);

    free = (struct mm_free *)((char *)mem + sizeof(*arena));
    list_push(&mm.memory, &free->node);
    free->arena = arena;
    free->size  = arena->size - arena->used - sizeof(*free);

    mm.size = arena->size;
    mm.used = arena->size - free->size;

    return 0;
}
