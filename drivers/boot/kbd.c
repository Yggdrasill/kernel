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

#include "io.h"
#include "string.h"
#include "kbd.h"

void kbd_write_wait(void)
{
  unsigned char kbd;
  do {
    kbd = inb(0x64);
  } while(kbd & 0x02);
}

void kbd_read_wait(void)
{
  unsigned char kbd;
  do {
    kbd = inb(0x64);
  } while(!(kbd & 0x01) );
}

void kbd_disable(void)
{
  kbd_write_wait();
  outb(0x64, 0xAD);
  kbd_write_wait();
  outb(0x64, 0xA7);
}

void kbd_enable(void)
{
  kbd_write_wait();
  outb(0x64, 0xAE);
  kbd_write_wait();
  outb(0x64, 0xA8);
}

void kbd_input(void)
{
  unsigned char kbd;
  unsigned int ch;

  kbd = inb(0x64);
  if(!(kbd & 0x20) ) {
    ch = inb(0x60);
    puthex(ch);
  }

  return;
}
