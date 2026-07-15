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

#include <mmap.h>
#include <sort.h>
#include <stdint.h>
#include <string.h>

#define MMAP_MAX_ENTRIES 128

#define MMAP_TABLE_SIZE              sizeof(struct e820_map) * MMAP_MAX_ENTRIES
#define MMAP_END_ADDR(x)             ((x)->base + (x)->size)
#define MMAP_REGION_SIZE(start, end) ((uintptr_t)&end - (uintptr_t)&start)

enum MMAP_TYPES {
	MMAP_USABLE = 1,
	MMAP_RESERVED,
	MMAP_ACPI_RECLAIMABLE,
	MMAP_ACPI_NVS,
	MMAP_BAD_MEMORY,
	MMAP_BOOTLOADER_RECLAIMABLE,
	MMAP_FRAMEBUFFER
};

struct e820_map {
	uint64_t base;
	uint64_t size;
	uint32_t type;
	uint32_t attrib;
};

struct e820_info {
	struct e820_map *base;
	size_t           nr_entries;
	size_t           max_nr_entries;
};

struct e820_point {
	struct e820_map *entry;
	uint64_t         addr;
};

extern char __BIOS_START;
extern char __BIOS_END;
extern char __BOOTLOADER_START;
extern char __BOOTLOADER_END;

extern char __GDTR_START;
extern char __GDTR_END;
extern char __GDT_START;
extern char __GDT_END;
extern char __IDT_START;
extern char __IDT_END;
extern char __STACK_START;
extern char __STACK_END;

extern char __UPPER_START;
extern char __UPPER_END;

int mmap_is_base(struct e820_point *p)
{
	return p->addr == p->entry->base;
}

int mmap_cmp(const void *p1, const void *p2)
{
	struct e820_point *pp1;
	struct e820_point *pp2;
	pp1 = (struct e820_point *)p1;
	pp2 = (struct e820_point *)p2;
	if(pp1->addr == pp2->addr) return mmap_is_base(pp1) ? -1 : 1;
	return (pp1->addr > pp2->addr) - (pp1->addr < pp2->addr);
}

int mmap_bad_type(uint32_t type)
{
	switch(type) {
		case MMAP_USABLE:
		case MMAP_ACPI_RECLAIMABLE:
		case MMAP_BOOTLOADER_RECLAIMABLE: return 0;
		default: return 1;
	}
}

uint32_t mmap_compare_type(const uint32_t t1, const uint32_t t2)
{
	if(!mmap_bad_type(t1) && mmap_bad_type(t2)) return t2;
	if(mmap_bad_type(t1) && !mmap_bad_type(t2)) return t1;
	return t1 > t2 ? t1 : t2;
}

__attribute__((
    __section__(".mmap"))) struct e820_map __mmap_old_map[MMAP_MAX_ENTRIES];
__attribute__((
    __section__(".mmap"))) struct e820_map __mmap_new_map[MMAP_MAX_ENTRIES];

static struct e820_map *const old_map = __mmap_old_map;
static struct e820_map *const new_map = __mmap_new_map;

/*
 * mmap_sanitize() is roughly based on the Linux implementation, using a
 * line-sweeping type algorithm to resolve overlapping memory regions.
 */

struct e820_info mmap_sanitize(
    struct e820_map *dst,
    struct e820_map *src,
    const uint32_t   nr_entries,
    const uint32_t   dst_max_entries)
{
	struct e820_point e820_points[2 * MMAP_MAX_ENTRIES];
	struct e820_info  info;

	struct e820_map *overlap_map[MMAP_MAX_ENTRIES];

	struct e820_point *prev_point;

	uint32_t type;
	uint32_t prev_type;
	uint32_t attrib;
	uint32_t prev_attrib;

	const size_t NR_POINTS = 2 * nr_entries;

	size_t new_nr_entries;
	size_t nr_overlaps;
	size_t i, j;

	info = (struct e820_info){
	    .base           = dst,
	    .nr_entries     = 0,
	    .max_nr_entries = MMAP_MAX_ENTRIES,
	};

	if(!nr_entries || dst_max_entries > MMAP_MAX_ENTRIES) return info;

	/*
	 * Break down the E820 structure into a sorted list of points that can be
	 * traversed. From these points the memory map will be rebuilt from scratch,
	 * since it can be messy when the BIOS provides it.
	 */

	j = 0;
	i = 0;
	for(i = 0; i < nr_entries; i++) {
		e820_points[j++] = (struct e820_point){src + i, src[i].base};
		e820_points[j++] =
		    (struct e820_point){src + i, src[i].base + src[i].size};
	}
	isort(e820_points, NR_POINTS, sizeof(*e820_points), mmap_cmp);

	new_nr_entries             = 0;
	nr_overlaps                = 0;
	prev_point                 = e820_points;
	prev_type                  = prev_point->entry->type;
	prev_attrib                = prev_point->entry->attrib;
	overlap_map[nr_overlaps++] = prev_point->entry;

	for(i = 1; i < NR_POINTS && new_nr_entries < dst_max_entries; i++) {

		/*
		 * Build a map of possibly overlapping entries. If the point is a base
		 * address entry, add it to the overlap map. If not, remove it.
		 */

		if(mmap_is_base(e820_points + i)) {
			overlap_map[nr_overlaps++] = e820_points[i].entry;
		} else {
			for(j = 0; j < nr_overlaps; j++) {
				if(overlap_map[j] == e820_points[i].entry) {
					overlap_map[j] = overlap_map[--nr_overlaps];
					break;
				}
			}
		}

		/*
		 * Compare type precedence and pick the worst type. It is roughly in the
		 * order of greatest type. the possible types are:
		 *
		 * MMAP_USABLE = 1
		 * MMAP_RESERVED = 2
		 * MMAP_ACPI_RECLAIMABLE = 3
		 * MMAP_ACPI_NVS = 4 (non-volatile storage)
		 * MMAP_BAD_MEMORY = 5
		 *
		 * There are also a few custom types:
		 *
		 * MMAP_BOOTLOADER_RECLAIMABLE = 6
		 * MMAP_FRAMEBUFFER = 7
		 *
		 * Type 2-5 inclusive, and type 7, are all considered unusable memory
		 * and should not be touched. All the usable types have precedence of
		 * the greatest type, so the type precedence is ultimately:
		 *
		 * 7 > 5 > 4 > 2 > 6 > 3 > 1
		 *
		 * It may not be desired to always reclaim usable memory, hence why they
		 * have greater precedence than type 1.
		 */

		type   = nr_overlaps > 0 ? overlap_map[0]->type : MMAP_RESERVED;
		attrib = nr_overlaps > 0 ? overlap_map[0]->attrib : 0;
		for(j = 1; j < nr_overlaps; j++) {
			type = mmap_compare_type(overlap_map[j]->type, type);
			if(type == overlap_map[j]->type) attrib = overlap_map[j]->attrib;
		}

		/*
		 * Reconstruct the map from the previous entry if type changes, or if we
		 * are at the last element of the array.
		 */

		if(type != prev_type || attrib != prev_attrib || i == NR_POINTS - 1) {
			dst[new_nr_entries] = (struct e820_map){
			    prev_point->addr,
			    e820_points[i].addr - prev_point->addr,
			    prev_type,
			    prev_attrib,
			};
			prev_point  = e820_points + i;
			prev_type   = type;
			prev_attrib = attrib;
			new_nr_entries += dst[new_nr_entries].size > 0;
		}
	}

	info = (struct e820_info){
	    .base           = dst,
	    .nr_entries     = new_nr_entries,
	    .max_nr_entries = MMAP_MAX_ENTRIES,
	};

	return info;
}

void mmap_print(struct e820_info *info)
{
	size_t i;

	for(i = 0; i < info->nr_entries; i++) {
		puthex(info->base[i].base);
		putchar(' ');
		puthex(info->base[i].size);
		putchar(' ');
		puthex(info->base[i].type);
		putchar('\n');
	}

	return;
}

int mmap_clobber(struct e820_info *info)
{
	struct e820_map *mmap;
	size_t           nr_entries;

	mmap       = info->base;
	nr_entries = info->nr_entries;

	mmap[nr_entries++] = (struct e820_map){
	    .base   = (uintptr_t)&__BIOS_START,
	    .size   = (uintptr_t)&__BIOS_END - (uintptr_t)&__BIOS_START,
	    .type   = MMAP_RESERVED,
	    .attrib = 0,
	};
	mmap[nr_entries++] = (struct e820_map){
	    .base   = (uintptr_t)&__BOOTLOADER_START,
	    .size   = (uintptr_t)&__BOOTLOADER_END - (uintptr_t)&__BOOTLOADER_START,
	    .type   = MMAP_BOOTLOADER_RECLAIMABLE,
	    .attrib = 0,
	};
	mmap[nr_entries++] = (struct e820_map){
	    .base   = (uintptr_t)&__GDTR_START,
	    .size   = MMAP_REGION_SIZE(__GDTR_START, __GDTR_END),
	    .type   = MMAP_BOOTLOADER_RECLAIMABLE,
	    .attrib = 0,
	};
	mmap[nr_entries++] = (struct e820_map){
	    .base   = (uintptr_t)&__GDT_START,
	    .size   = MMAP_REGION_SIZE(__GDT_START, __GDT_END),
	    .type   = MMAP_BOOTLOADER_RECLAIMABLE,
	    .attrib = 0,
	};
	mmap[nr_entries++] = (struct e820_map){
	    .base   = (uintptr_t)old_map,
	    .size   = MMAP_TABLE_SIZE,
	    .type   = MMAP_BOOTLOADER_RECLAIMABLE,
	    .attrib = 0,
	};
	mmap[nr_entries++] = (struct e820_map){
	    .base   = (uintptr_t)new_map,
	    .size   = MMAP_TABLE_SIZE,
	    .type   = MMAP_BOOTLOADER_RECLAIMABLE,
	    .attrib = 0,
	};
	mmap[nr_entries++] = (struct e820_map){
	    .base   = (uintptr_t)&__IDT_START,
	    .size   = MMAP_REGION_SIZE(__IDT_START, __IDT_END),
	    .type   = MMAP_BOOTLOADER_RECLAIMABLE,
	    .attrib = 0,
	};
	mmap[nr_entries++] = (struct e820_map){
	    .base   = (uintptr_t)&__STACK_START,
	    .size   = MMAP_REGION_SIZE(__STACK_START, __STACK_END),
	    .type   = MMAP_BOOTLOADER_RECLAIMABLE,
	    .attrib = 0,
	};
	mmap[nr_entries++] = (struct e820_map){
	    .base   = (uintptr_t)&FB_ADDR,
	    .size   = (uintptr_t)&FB_END - (uintptr_t)&FB_ADDR,
	    .type   = MMAP_FRAMEBUFFER,
	    .attrib = 0,
	};
	mmap[nr_entries++] = (struct e820_map){
	    .base   = (uintptr_t)&__UPPER_START,
	    .size   = (uintptr_t)&__UPPER_END - (uintptr_t)&__UPPER_START,
	    .type   = MMAP_RESERVED,
	    .attrib = 0,
	};

	info->nr_entries = nr_entries;
	return nr_entries;
}

/*
 * As in every other source file: This type of global state is bootloader only.
 */

static struct e820_info mmap_info;

struct e820_info *mmap_info_init(void)
{
	struct e820_info *info;

	info = &mmap_info;

	*info = (struct e820_info){
	    .base = old_map, .nr_entries = 0, .max_nr_entries = MMAP_MAX_ENTRIES};

	return info;
}

struct e820_info *mmap_init(void)
{
	struct e820_info *info;

	info = mmap_info_init();
	mmap_clobber(info);

	return info;
}

struct e820_info *mmap_setup(struct e820_info *info)
{
	struct e820_info new_info;
	struct e820_map *old;
	old      = info->base;
	new_info = mmap_sanitize(new_map, old, info->nr_entries, MMAP_MAX_ENTRIES);
	memcpy(info, &new_info, sizeof(*info));
	mmap_print(info);
	return info;
}
