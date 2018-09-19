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

#ifndef KBD_H
#define KBD_H

/* Unprintable ASCII keys */

enum ASCII {
  ASCII_NUL,
  ASCII_SOH,
  ASCII_STX,
  ASCII_ETX,
  ASCII_EOT,
  ASCII_ENQ,
  ASCII_ACK,
  ASCII_BEL,
  ASCII_BS,
  ASCII_HT,
  ASCII_LF,
  ASCII_VT,
  ASCII_FF,
  ASCII_CR,
  ASCII_SO,
  ASCII_SI,
  ASCII_DLE,
  ASCII_DC1,
  ASCII_DC2,
  ASCII_DC3,
  ASCII_DC4,
  ASCII_NAK,
  ASCII_SYN,
  ASCII_ETB,
  ASCII_CAN,
  ASCII_EM,
  ASCII_SUB,
  ASCII_ESC,
  ASCII_FS,
  ASCII_GS,
  ASCII_RS,
  ASCII_US
};

struct keycodes {
  unsigned char key;
  unsigned char shift_key;
  unsigned char scan_code;
  unsigned char printable;
};

void kbd_write_wait(void);
void kbd_read_wait(void);
void kbd_disable(void);
void kbd_enable(void);
void kbd_input(void);

#endif
