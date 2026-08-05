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

/*
 * This is just a simple linked list allocator for now. Ideally the kernel would
 * use a slab allocator, or something based on rb-trees or some other
 * self-balancing BST. A linked list allocator is sufficient for now.
 */

#define HEAP_ALLOC_SIZE  (256 * (1 << 10))
#define HEAP_ALLOC_ALIGN (sizeof(struct mm_header))

struct mm_header {
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
    struct mm_header  *free;

    void *mem;

    list_init(&mm.arenas);
    list_init(&mm.memory);

    mem = alloc(HEAP_ALLOC_SIZE);
    if(!mem) return -1;

    arena = mem;
    list_push(&mm.arenas, &arena->node);
    arena->size = HEAP_ALLOC_SIZE;
    arena->used = sizeof(*arena) + sizeof(*free);

    free = (struct mm_header *)((char *)mem + sizeof(*arena));
    list_push(&mm.memory, &free->node);
    free->arena = arena;
    free->size  = arena->size - arena->used - sizeof(*free);

    mm.size = arena->size;
    mm.used = arena->size - free->size;

    return 0;
}

static struct list_node *mm_search_addr_lt(void *ptr)
{
    struct mm_header *ptr_header;
    struct mm_header *header;
    struct list_node *node;

    node = list_peek_head(&mm.memory);
    if(!node) return NULL;

    do {
        header = node_container(struct mm_header, node, node);
        node   = list_next(node);
    } while(node && (uintptr_t)header < (uintptr_t)ptr);
    node = &header->node;

    return node;
}

void *kalloc(size_t size)
{

    struct mm_header *header;
    struct mm_header *new_header;
    struct list_node *node;
    struct list_node *new_node;

    void *ptr;

    node = list_peek_head(&mm.memory);
    if(!node) return NULL;
    if(size & 0xF) size = (size & HEAP_ALLOC_ALIGN) + HEAP_ALLOC_ALIGN;
    size += sizeof(*header);

    do {
        header = node_container(struct mm_header, node, node);
        node   = list_next(node);
    } while (node && size > header->size);
    if(size > header->size) return NULL;

    node = &header->node;
    ptr  = ((char *)header + sizeof(*header));

    new_header = (struct mm_header *)((char *)header + size);
    new_node   = &new_header->node; 
    list_insert_post(&mm.memory, new_node, node);
    list_delete(&mm.memory, node);
    new_header->size  = header->size - size;
    new_header->arena = header->arena;

    header->size = size;
    mm.used += size;
    header->arena->used += size;

    return ptr;
}

void kfree(void *ptr)
{
    struct mm_header *header;
    struct list_node *node;

    header = (struct mm_header *)ptr - 1;
    node   = mm_search_addr_lt(header);
    list_insert_pre(&mm.memory, &header->node, node);

    mm.used -= header->size;
    header->arena->used -= header->size;

    return;
}
