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

#include <list.h>
#include <stddef.h>

struct list_root *list_init(struct list_root *root)
{
    if(!root) return NULL;

    *root = (struct list_root){
        .head       = NULL,
        .tail       = NULL,
        .nr_entries = 0,
    };

    return root;
}

struct list_node *list_next(const struct list_node *node)
{
    if(!node) return NULL;
    return node->next;
}

struct list_node *list_prev(const struct list_node *node)
{
    if(!node) return NULL;
    return node->prev;
}

struct list_node *list_traverse(const struct list_root *root, const size_t n)
{
    struct list_node *node;

    size_t i;

    if(!root) return NULL;
    if(n >= root->nr_entries) return NULL;

    node = root->head;
    for(i = 0; node && i < n; i++) node = node->next;

    return node;
}

struct list_node *
list_traverse_rev(const struct list_root *root, const size_t n)
{
    struct list_node *node;

    size_t i;

    if(!root) return NULL;
    if(n >= root->nr_entries) return NULL;

    node = root->tail;
    for(i = 0; node && i < n; i++) node = node->prev;

    return node;
}

struct list_node *list_push(struct list_root *root, struct list_node *node)
{
    if(!root || !node) return NULL;

    if(root->tail) root->tail->next = node;
    else root->head = node;
    node->prev = root->tail;
    node->next = NULL;
    root->nr_entries++;

    return root->tail = node;
}

struct list_node *list_pop(struct list_root *root)
{
    struct list_node *node;

    if(!root || !root->tail) return NULL;

    node       = root->tail;
    root->tail = node->prev;
    if(root->tail) root->tail->next = NULL;
    else root->head = NULL;
    root->nr_entries--;

    return node;
}

struct list_node *list_insert_pre(
    struct list_root *root, struct list_node *node, struct list_node *pre)
{
    if(!root || !node || !pre) return NULL;

    if(pre != root->head) pre->prev->next = node;
    else root->head = node;
    node->next = pre;
    node->prev = pre->prev;
    pre->prev  = node;
    root->nr_entries++;

    return node;
}

struct list_node *list_insert_post(
    struct list_root *root, struct list_node *node, struct list_node *post)
{
    if(!root || !node || !post) return NULL;

    if(post != root->tail) post->next->prev = node;
    else root->tail = node;
    node->next = post->next;
    node->prev = post;
    post->next = node;
    root->nr_entries++;

    return node;
}

struct list_node *list_delete(struct list_root *root, struct list_node *node)
{
    if(!root || !node) return NULL;

    if(node != root->head) node->prev->next = node->next;
    else root->head = node->next;
    if(node != root->tail) node->next->prev = node->prev;
    else root->tail = node->prev;
    root->nr_entries--;

    return node;
}

struct list_node *
list_insert_at(struct list_root *root, struct list_node *node, const size_t n)
{
    struct list_node *pre;

    if(!root) return NULL;

    if(n == root->nr_entries) return list_push(root, node);

    pre = list_traverse(root, n);
    return list_insert_pre(root, node, pre);
}

struct list_node *list_delete_at(struct list_root *root, const size_t n)
{
    struct list_node *node;

    node = list_traverse(root, n);
    return list_delete(root, node);
}
