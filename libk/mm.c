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

#define MAGIC_HEADER 0x807FAA55

#define INIT_NR_NODES   32
#define HEAP_ALLOC_SIZE (256 * (1 << 10))

enum MM_TYPES {
    MM_INTERNAL = 1,
    MM_ALLOCD   = 2,
};

struct mm_list {
    struct list_node node;
    struct extent    range;
};

struct mm_used {
    uint32_t magic;
    uint8_t  type;
    struct mm_list used;
};


struct mm_info {
    struct list_root free;
    size_t           total_size;
    size_t           used_size;
};

static struct list_root unused;
static struct list_root allocd;
static struct mm_info   mm;

void mm_init(const size_t available)
{
    struct mm_list *free;
    struct mm_list *used;
    struct mm_used *header;

    void  *mem;
    size_t list_size;
    size_t bytes;
    size_t i;

    list_init(&unused);
    list_init(&allocd);
    list_init(&mm.free);
    mm.total_size = 0;
    mm.used_size  = 0;

    list_size = sizeof(*header) + sizeof(*free) * INIT_NR_NODES;
    bytes     = HEAP_ALLOC_SIZE + list_size;
    if(available < bytes) bytes = available;
    mem = alloc(bytes);
    if(!mem) panic("mm_init: Unable to allocate initial heap!");

    header = mem;
    used   = &header->used;

    used->range.start = (size_t)((uintptr_t)mem / PAGE_ALIGN);
    used->range.end   = (size_t)((uintptr_t)mem + bytes);
    used->range.end   = (used->range.end + PAGE_ALIGN - 1) / PAGE_ALIGN;
    header->magic     = MAGIC_HEADER;
    header->type      = MM_INTERNAL;
    list_push(&allocd, &used->node);

    free = (struct mm_list *)((char *)mem + sizeof(*used));
    for(i = INIT_NR_NODES; i > 0; i--) {
        list_push(&unused, &free[i - 1].node);
    }
    free = node_container(struct mm_list, list_pop(&unused), node);

    free->range.start = (size_t)((uintptr_t)mem + list_size);
    free->range.start = (free->range.start + PAGE_ALIGN - 1) / PAGE_ALIGN;
    free->range.end   = ((uintptr_t)mem + bytes - list_size) / PAGE_ALIGN;
    list_push(&mm.free, &free->node);
}
