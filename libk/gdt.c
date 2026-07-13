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

#include "gdt.h"
#include "stdint.h"
#include "string.h"

struct gdt_ptr *gdt_init(void)
{
	struct gdt_ptr   *gdtp;
	struct gdt_entry *base;
	uint16_t          gdt_size;

	gdtp = &__GDT_PTR_LOCATION;
	base = &__GDT_BASE_LOCATION;

	/* NULL GDT entry */
	gdt_size = sizeof(*base) - 1;

	gdtp->size_0  = ((unsigned char *)&gdt_size)[0];
	gdtp->size_8  = ((unsigned char *)&gdt_size)[1];
	gdtp->base_0  = ((unsigned char *)&base)[0];
	gdtp->base_8  = ((unsigned char *)&base)[1];
	gdtp->base_16 = ((unsigned char *)&base)[2];
	gdtp->base_24 = ((unsigned char *)&base)[3];

	/* Setup of NULL GDT entry */
	memset(base, 0, sizeof(*base));

	return gdtp;
}

void gdt_entry_add(
    struct gdt_ptr *gdtp,
    void           *base,
    uint32_t        limit,
    uint8_t         access,
    uint8_t         flags)
{
	struct gdt_entry *entry;
	uintptr_t         gdt_base_addr;
	size_t            size;
	size_t            index;

	if(limit > 0xFFFFF) goto fail;

	gdt_base_addr = (uintptr_t)gdtp->base_24 << 24 |
	                (uintptr_t)gdtp->base_16 << 16 |
	                (uintptr_t)gdtp->base_8 << 8 | (uintptr_t)gdtp->base_0;
	entry         = (struct gdt_entry *)gdt_base_addr;

	size = ((uint16_t)gdtp->size_8 << 8 | (uint16_t)gdtp->size_0) + 1;
	if(size + sizeof(*entry) > 0x10000) goto fail;

	index = size / sizeof(*entry);
	entry += index;

	entry->access  = access;
	entry->base_0  = ((unsigned char *)&base)[0];
	entry->base_8  = ((unsigned char *)&base)[1];
	entry->base_16 = ((unsigned char *)&base)[2];
	entry->base_24 = ((unsigned char *)&base)[3];
	entry->limit_0 = ((unsigned char *)&limit)[0];
	entry->limit_8 = ((unsigned char *)&limit)[1];
	entry->limit_flags =
	    (flags & 0x0F) << 4 | (((unsigned char *)&limit)[2] & 0x0F);

	size         = sizeof(*entry) * (index + 1) - 1;
	gdtp->size_0 = ((unsigned char *)&size)[0];
	gdtp->size_8 = ((unsigned char *)&size)[1];
fail:
	return;
}

void gdt_default_entries_add(struct gdt_ptr *gdtp)
{
	uint8_t access;
	uint8_t flags;

	access = GDT_PRESENT | GDT_SEGMENT | GDT_RW | GDT_ACCESSED;
	flags  = GDT_GRAN | GDT_BITS_32;

	gdt_entry_add(gdtp, 0, 0xFFFFF, access | GDT_EXEC, flags);
	gdt_entry_add(gdtp, 0, 0xFFFFF, access, flags);

	return;
}

void gdt_install(struct gdt_ptr *gdtp)
{
	__asm__ volatile("mov  eax, %0;"
	                 "lgdt [eax];"
	                 :
	                 : "m"(gdtp));

	return;
}
