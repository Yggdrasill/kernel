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

#include <stdint.h>
#include <string.h>

#include "rmode.h"

#include <boot/common/disk.h>

#include <libk/idt.h>
#include <libk/interrupt.h>
#include <libk/mmap.h>
#include <libk/util.h>

extern int32_t  __bios_mmap(struct e820_info *);
extern void     __bios_print(uint16_t, uint16_t);
extern uint32_t __chs_geometry(struct disk_info *, uint8_t);
extern uint32_t __disk_reset(uint8_t);
extern int32_t
__chs_read(uint32_t, char *, size_t, uint8_t, struct disk_info *);

extern union rmode_ret_t rmode_trampoline(void (*)(void), ...);

/*
 * bios_mmap return codes:
 * return = 0 on success
 * return = -1 when E820 is unsupported
 * return = -2 when E820 produced a malformed response
 * return = -3 when E820 produced a malformed entry size
 * return = -4 when E820 map has been exhausted
 *
 * The following actions should be performed on error returns:
 * - if return = -1, try other BIOS memory requests
 * - if return < -1, these are panic conditions, memory map cannot be trusted
 */

/* TODO: Implement other memory map functions, for now panic. */
int32_t bios_mmap(struct e820_info *mmap)
{
    union rmode_ret_t rv;
    rv = rmode_trampoline((void (*)(void))__bios_mmap, mmap);
    switch(rv.i32) {
        case 0: break;
        case -1: panic("E820: unsupported!"); break;
        case -2: panic("E820: malformed response"); break;
        case -3: panic("E820: malformed entry size"); break;
        case -4: panic("E820: map exhausted"); break;
        default: panic("E820: unknown error!");
    }
    return rv.i32;
}

void bios_print(char *str, size_t len)
{
    rmode_trampoline((void (*)(void))__bios_print, str, len);
    return;
}

uint32_t bios_disk_geometry(struct disk_info *disk, uint8_t id)
{
    union rmode_ret_t rv;
    rv = rmode_trampoline((void (*)(void))__chs_geometry, disk, id);
    puthex(&rv.u32, sizeof(rv.u32), 1);
    putchar('\n');
    return rv.u32;
}

uint32_t bios_disk_reset(uint8_t id)
{
    union rmode_ret_t rv;
    rv = rmode_trampoline((void (*)(void))__disk_reset, id);
    puthex(&rv.u32, sizeof(rv.u32), 1);
    putchar('\n');
    return rv.u32;
}

int32_t bios_chs_read(
    char             *buffer,
    uint32_t          lba,
    size_t            size,
    uint8_t           drive,
    struct disk_info *disk)
{
    union rmode_ret_t rv;
    rv = rmode_trampoline(
        (void (*)(void))__chs_read, buffer, lba, size, drive, disk);
    return 0;
}
