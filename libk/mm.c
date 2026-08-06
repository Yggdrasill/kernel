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
#include <math.h>
#include <stddef.h>
#include <stdint.h>

#ifdef BOOTLOADER_COMPILE
extern void *pmm_alloc(size_t);
extern void  pmm_free(void *, size_t);
    #define palloc pmm_alloc
    #define pfree  pmm_free
#endif

SAFE_ADD_IMPLEMENT(size, size_t, SIZE_MAX)

/*
 * This is just a simple linked list allocator for now. Ideally the kernel would
 * use a slab allocator, or something based on rb-trees or some other
 * self-balancing BST. A linked list allocator is sufficient for now.
 */

#define HEAP_ALLOC_SIZE (256UL * (1 << 10))
#define HEAP_ALLOC_MASK (~(HEAP_ALLOC_SIZE - 1))
#define HEAP_ALIGN_SIZE (sizeof(struct mm_header))
#define HEAP_ALIGN_MASK (~(HEAP_ALIGN_SIZE - 1))

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

static struct list_node *mm_search_addr_gt(void *ptr)
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

static struct list_node *mm_alloc_arena(size_t size)
{
    struct list_node *node;
    struct mm_arena  *arena;
    struct mm_header *free;

    size_t pad;
    void  *mem;

    if(safe_add_size(&size, size, sizeof(*arena))) return NULL;
    pad  = size & (HEAP_ALLOC_SIZE - 1) ? HEAP_ALLOC_SIZE : 0;
    size = size & HEAP_ALLOC_MASK;
    if(safe_add_size(&size, size, pad)) return NULL;

    mem = palloc(size);
    if(!mem) return NULL;

    arena = mem;
    list_push(&mm.arenas, &arena->node);
    arena->size = size;
    arena->used = sizeof(*arena);

    free = (struct mm_header *)((char *)mem + sizeof(*arena));
    node = mm_search_addr_gt(free);
    if(!node) list_push(&mm.memory, &free->prefix.node);
    else list_insert_pre(&mm.memory, &free->prefix.node, node);
    free->arena = arena;
    free->size  = arena->size - sizeof(*arena);

    mm.size += size;
    mm.used += sizeof(*arena);

    return &free->prefix.node;
}

static void mm_free_arena(struct mm_arena *arena)
{
    if(!arena) return;
    list_delete(&mm.arenas, &arena->node);
    pfree(arena, arena->size);
}

static struct list_node *mm_merge_free(struct mm_header *header)
{
    struct list_node *prev;
    struct list_node *node;
    struct mm_header *new_header;

    void *end;

    prev = &header->prefix.node;
    node = list_prev(prev);
    while(node) {
        new_header = node_container(struct mm_header, node, prefix.node);
        end        = ((char *)new_header + new_header->size);
        if(end != header || header->arena != new_header->arena) break;
        header = new_header;
        prev   = node;
        node   = list_prev(&header->prefix.node);
    }

    header = node_container(struct mm_header, prev, prefix.node);
    node   = list_next(prev);
    while(node) {
        new_header = node_container(struct mm_header, node, prefix.node);
        end        = ((char *)header + header->size);
        if(end != new_header || header->arena != new_header->arena) break;
        header->size += new_header->size;
        node = list_next(list_delete(&mm.memory, node));
    }

    return prev;
}

void *kalloc(size_t size)
{
    struct mm_header *header;
    struct mm_header *new_header;
    struct list_node *node;
    struct list_node *new_node;
    struct mm_magic  *magic;

    size_t pad;
    void  *ptr;

    if(!size) return NULL;

    pad = size & (HEAP_ALIGN_SIZE - 1) ? HEAP_ALIGN_SIZE : 0;
    pad += sizeof(*header);
    size = size & HEAP_ALIGN_MASK;
    if(safe_add_size(&size, size, pad)) return NULL;

    node = list_peek_head(&mm.memory);
    while(node) {
        header = node_container(struct mm_header, node, prefix.node);
        if(size <= header->size) break;
        node = list_next(node);
    }
    if(!node) {
        if(!(node = mm_alloc_arena(size))) return NULL;
        header = node_container(struct mm_header, node, prefix.node);
    }

    ptr = ((char *)header + sizeof(*header));
    if(header->size - size >= sizeof(*header) + HEAP_ALIGN_SIZE) {
        new_header        = (struct mm_header *)((char *)header + size);
        new_node          = &new_header->prefix.node;
        new_header->size  = header->size - size;
        new_header->arena = header->arena;
        list_insert_post(&mm.memory, new_node, node);
    } else {
        size = header->size;
    }

    list_delete(&mm.memory, node);
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

    if(!ptr) return;

    header = (struct mm_header *)ptr - 1;
    magic  = &header->prefix.magic;
    /* Probably not the best for the kernel, but for now... */
    if(magic->magic != HEADER_MAGIC || magic->ptr != ptr) {
        panic("kfree: Invalid free!");
    }

    node = mm_search_addr_gt(header);
    if(!node) list_push(&mm.memory, &header->prefix.node);
    else list_insert_pre(&mm.memory, &header->prefix.node, node);

    mm.used -= header->size;
    header->arena->used -= header->size;
    node = mm_merge_free(header);
    if(header->arena->used == sizeof(*header->arena)) {
        list_delete(&mm.memory, node);
        mm_free_arena(header->arena);
    }

    return;
}
