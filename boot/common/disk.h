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

#ifndef DISK_H
#define DISK_H

#include <stddef.h>
#include <stdint.h>

struct disk_info {
    uint16_t nr_cylinders;
    uint8_t  nr_heads;
    uint8_t  nr_sectors;
    uint8_t  nr_drives;
};

#define DISK_CYLINDERS_OFFSET (offsetof(struct disk_info, nr_cylinders))
#define DISK_HEADS_OFFSET     (offsetof(struct disk_info, nr_heads))
#define DISK_SECTORS_OFFSET   (offsetof(struct disk_info, nr_sectors))
#define DISK_DRIVES_OFFSET    (offsetof(struct disk_info, nr_drives))

#endif
