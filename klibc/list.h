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

#ifndef LIST_H
#define LIST_H

#include <stddef.h>

struct list_node {
    struct list_node *prev;
    struct list_node *next;
};

struct list_root {
    struct list_node *head;
    struct list_node *tail;
    size_t            nr_entries;
    size_t            max_nr_entries;
};

struct list_root *list_init(struct list_root *);
struct list_node *list_next(const struct list_node *);
struct list_node *list_prev(const struct list_node *);
struct list_node *list_traverse(const struct list_root *, const size_t n);
struct list_node *list_traverse_rev(const struct list_root *, const size_t n);
struct list_node *list_push(struct list_root *, struct list_node *);
struct list_node *list_pop(struct list_root *);
struct list_node *
list_insert_pre(struct list_root *, struct list_node *, struct list_node *pre);
struct list_node *list_insert_post(
    struct list_root *, struct list_node *, struct list_node *post);
struct list_node *list_delete(struct list_root *, struct list_node *);
struct list_node *
list_insert_at(struct list_root *, struct list_node *, const size_t n);
struct list_node *list_delete_at(struct list_root *, const size_t n);

#endif
