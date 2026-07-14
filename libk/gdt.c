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

#define GDT_MAX_ENTRIES 8192

enum GDT_ACCESS {
	GDT_ACCESSED   = 1,
	GDT_RW         = 1 << 1,
	GDT_DC         = 1 << 2,
	GDT_EXEC       = 1 << 3,
	GDT_SEGMENT    = 1 << 4,
	GDT_PRIV_RING0 = 0 << 5,
	GDT_PRIV_RING1 = 1 << 5,
	GDT_PRIV_RING2 = 2 << 5,
	GDT_PRIV_RING3 = 3 << 5,
	GDT_PRESENT    = 1 << 7
};

enum GDT_FLAGS {
	GDT_RESERVED = 0,
	GDT_LONG     = 1 << 1,
	GDT_BITS_32  = 1 << 2,
	GDT_GRAN     = 1 << 3
};

struct gdt_ptr {
	uint8_t size_0;
	uint8_t size_8;
	uint8_t base_0;
	uint8_t base_8;
	uint8_t base_16;
	uint8_t base_24;
};

struct gdt_entry {
	uint8_t limit_0;
	uint8_t limit_8;
	uint8_t base_0;
	uint8_t base_8;
	uint8_t base_16;
	uint8_t access;
	uint8_t limit_flags;
	uint8_t base_24;
};

struct gdt_info {
	struct gdt_ptr   *gdtr;
	struct gdt_entry *entries;
	size_t            max_nr_entries;
	size_t            nr_entries;
};

extern struct gdt_ptr   __GDT_PTR_LOCATION;
extern struct gdt_entry __GDT_BASE_LOCATION;
static struct gdt_info  gdt_info;

void gdt_install(struct gdt_ptr *gdtr)
{
	__asm__ volatile("mov  eax, %0;"
	                 "lgdt [eax];"
	                 :
	                 : "m"(gdtr));

	return;
}

struct gdt_info *gdt_info_init(void)
{
	gdt_info = (struct gdt_info){
	    .gdtr           = &__GDT_PTR_LOCATION,
	    .entries        = &__GDT_BASE_LOCATION,
	    .max_nr_entries = GDT_MAX_ENTRIES,
	    .nr_entries     = 0,
	};
	return &gdt_info;
}

void gdt_entry_add(
    struct gdt_info *info,
    void            *base,
    uint32_t         limit,
    uint8_t          access,
    uint8_t          flags)
{
	struct gdt_ptr   *gdtr;
	struct gdt_entry *entry;
	uintptr_t         gdt_base_addr;
	size_t            size;
	size_t            index;

	if(limit > 0xFFFFF) goto fail;

	gdtr = info->gdtr;

	gdt_base_addr = (uintptr_t)gdtr->base_24 << 24 |
	                (uintptr_t)gdtr->base_16 << 16 |
	                (uintptr_t)gdtr->base_8 << 8 | (uintptr_t)gdtr->base_0;
	entry         = (struct gdt_entry *)gdt_base_addr;

	size = ((uint16_t)gdtr->size_8 << 8 | (uint16_t)gdtr->size_0) + 1;
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
	gdtr->size_0 = ((unsigned char *)&size)[0];
	gdtr->size_8 = ((unsigned char *)&size)[1];
fail:
	return;
}

void gdt_default_entries_add(struct gdt_info *info)
{
	uint8_t access;
	uint8_t flags;

	access = GDT_PRESENT | GDT_SEGMENT | GDT_RW | GDT_ACCESSED;
	flags  = GDT_GRAN | GDT_BITS_32;

	gdt_entry_add(info, 0, 0xFFFFF, access | GDT_EXEC, flags);
	gdt_entry_add(info, 0, 0xFFFFF, access, flags);

	return;
}

struct gdt_info *gdt_init(void)
{
	struct gdt_ptr   *gdtr;
	struct gdt_entry *base;
	struct gdt_info  *info;
	uint16_t          gdt_size;

	info = gdt_info_init();
	gdtr = info->gdtr;
	base = info->entries;

	/* NULL GDT entry */
	gdt_size = sizeof(*base) - 1;

	gdtr->size_0  = ((unsigned char *)&gdt_size)[0];
	gdtr->size_8  = ((unsigned char *)&gdt_size)[1];
	gdtr->base_0  = ((unsigned char *)&base)[0];
	gdtr->base_8  = ((unsigned char *)&base)[1];
	gdtr->base_16 = ((unsigned char *)&base)[2];
	gdtr->base_24 = ((unsigned char *)&base)[3];

	/* Setup of NULL GDT entry */
	memset(base, 0, sizeof(*base));
	gdt_default_entries_add(info);
	gdt_install(info->gdtr);

	return info;
}
