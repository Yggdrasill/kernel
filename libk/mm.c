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
#define HEAP_ALLOC_MASK  (~(sizeof(struct mm_header) - 1))

#define HEADER_MAGIC 0x807FAA55

struct mm_magic {
    uintptr_t magic;
    void     *ptr;
};

struct mm_header {
    union prefix {
        struct list_node node;
        struct mm_magic  magic;
    } prefix;
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
    struct mm_arena  *arena;
    struct mm_header *free;

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
    list_push(&mm.memory, &free->prefix.node);
    free->arena = arena;
    free->size  = arena->size - arena->used;

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
        header = node_container(struct mm_header, node, prefix.node);
        if((uintptr_t)header > (uintptr_t)ptr) break;
        node = list_next(node);
    } while(node);

    return node;
}

void *kalloc(size_t size)
{

    struct mm_header *header;
    struct mm_header *new_header;
    struct list_node *node;
    struct list_node *new_node;
    struct mm_magic  *magic;

    void *ptr;

    node = list_peek_head(&mm.memory);
    if(!node) return NULL;
    if(size & (HEAP_ALLOC_ALIGN - 1)) {
        size = (size & HEAP_ALLOC_MASK) + HEAP_ALLOC_ALIGN;
    }
    size += sizeof(*header);

    do {
        header = node_container(struct mm_header, node, prefix.node);
        node   = list_next(node);
    } while(node && size > header->size);
    if(size > header->size) return NULL;

    node = &header->prefix.node;
    ptr  = ((char *)header + sizeof(*header));

    new_header = (struct mm_header *)((char *)header + size);
    new_node   = &new_header->prefix.node;
    list_insert_post(&mm.memory, new_node, node);
    list_delete(&mm.memory, node);
    new_header->size  = header->size - size;
    new_header->arena = header->arena;

    magic        = &header->prefix.magic;
    magic->magic = HEADER_MAGIC;
    magic->ptr   = ptr;
    header->size = size;
    mm.used += size;
    header->arena->used += size;

    return ptr;
}

void kfree(void *ptr)
{
    struct mm_header *header;
    struct list_node *node;
    struct mm_magic  *magic;

    header = (struct mm_header *)ptr - 1;
    magic  = &header->prefix.magic;
    if(magic->magic != HEADER_MAGIC || magic->ptr != ptr) {
        panic("kfree: Invalid free!");
    }

    node = mm_search_addr_lt(header);
    if(!node) list_push(&mm.memory, &header->prefix.node);
    else list_insert_pre(&mm.memory, &header->prefix.node, node);

    mm.used -= header->size;
    header->arena->used -= header->size;

    return;
}
