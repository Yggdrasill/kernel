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

#ifndef BOOT_PMM_H
#define BOOT_PMM_H

#define PMM_INIT_ENTRIES 1024

#if !defined(LD_BOOT_STAGE1) && !defined(LD_BOOT_STAGE2)

    #include <libk/mmap.h>

struct pmm_bitmap {
    size_t *bitmap;
    size_t nr_entries;
    size_t max_entries;
    size_t available;
};

int pmm_init(struct e820_info *);

#endif

#endif
