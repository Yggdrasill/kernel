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

#include <libk/io.h>
#include <stdint.h>

/*
 * The file defines a standard interface for port I/O. NO OTHER source file
 * should have external linkage to port_io_write/port_io_read.
 */

extern uint32_t port_io_write(uint32_t, uint32_t, uint32_t);
extern uint32_t port_io_read(uint32_t, uint32_t);

enum PORT_IO {
    PORT_IO_BYTE  = 1,
    PORT_IO_WORD  = 1 << 1,
    PORT_IO_DWORD = 1 << 2,
    PORT_IO_WRITE = 1 << 3,
};

uint8_t port_read_byte(uint16_t port)
{
    uint8_t data;
    data = (uint8_t)port_io_read(PORT_IO_BYTE, port);
    return data;
}

uint16_t port_read_word(uint16_t port)
{
    uint16_t data;
    data = (uint16_t)port_io_read(PORT_IO_WORD, port);
    return data;
}

uint32_t port_read_dword(uint16_t port)
{
    uint32_t data;
    data = port_io_read(PORT_IO_DWORD, port);
    return data;
}

uint8_t port_write_byte(uint16_t port, uint8_t data)
{
    data = (uint8_t)port_io_write(PORT_IO_WRITE | PORT_IO_BYTE, port, data);
    return data;
}

uint16_t port_write_word(uint16_t port, uint16_t data)
{
    data = (uint16_t)port_io_write(PORT_IO_WRITE | PORT_IO_WORD, port, data);
    return data;
}

uint32_t port_write_dword(uint16_t port, uint32_t data)
{
    data = port_io_write(PORT_IO_WRITE | PORT_IO_DWORD, port, data);
    return data;
}
