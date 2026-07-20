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

#ifndef SORT_H
#define SORT_H

#include <stddef.h>
#include <stdint.h>

void isort(
    void  *base,
    size_t nmemb,
    size_t size,
    int    (*compar)(const void *, const void *));

#define ISORT_DECLARE(name, type)                                              \
	void isort_##name(                                                         \
	    type *base, size_t nmemb, int (*compar)(const type, const type));

#define ISORT_IMPLEMENT(name, type)                                            \
	void isort_##name(                                                         \
	    type *base, size_t nmemb, int (*compar)(const type, const type))       \
	{                                                                          \
		type   key;                                                            \
		size_t i, j;                                                           \
		for(i = 0; i < nmemb; i++) {                                           \
			key = base[i];                                                     \
			for(j = i; j > 0 && compar(base[j - 1], key) > 0; j--) {           \
				base[j] = base[j - 1];                                         \
			}                                                                  \
			base[j] = key;                                                     \
		}                                                                      \
		return;                                                                \
	}

#endif
