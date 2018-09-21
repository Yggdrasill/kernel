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

#define KCODE_MULTI 0xFE /* multimedia keys placeholder */
#define KCODE_ACPI  0xFF /* acpi keys placeholder */

/* keycodes ordered by rows, 32 max keys for each row */

enum KCODE_ROW0 {
  KCODE_ESC = 0x00,
  KCODE_F1,
  KCODE_F2,
  KCODE_F3,
  KCODE_F4,
  KCODE_F5,
  KCODE_F6,
  KCODE_F7,
  KCODE_F8,
  KCODE_F9,
  KCODE_F10,
  KCODE_F11,
  KCODE_F12,
  KCODE_PRTSC,
  KCODE_SCRLK,
  KCODE_PAUSE
};

enum KCODE_ROW1 {
  KCODE_TILDE = 0x20,
  KCODE_ONE,
  KCODE_TWO,
  KCODE_THREE,
  KCODE_FOUR,
  KCODE_FIVE,
  KCODE_SIX,
  KCODE_SEVEN,
  KCODE_EIGHT,
  KCODE_NINE,
  KCODE_ZERO,
  KCODE_DASH,
  KCODE_EQUAL,
  KCODE_BCKSP,
  KCODE_INS,
  KCODE_HOME,
  KCODE_PGUP,
  KCODE_NLOCK,
  KCODE_NDIV,
  KCODE_NMUL,
  KCODE_NSUB
};

enum KCODE_ROW2 {
  KCODE_TAB = 0x40,
  KCODE_Q,
  KCODE_W,
  KCODE_E,
  KCODE_R,
  KCODE_T,
  KCODE_Y,
  KCODE_U,
  KCODE_I,
  KCODE_O,
  KCODE_P,
  KCODE_LSQBRK,
  KCODE_RSQBRK,
  KCODE_BSLASH,
  KCODE_DEL,
  KCODE_END,
  KCODE_PGDN,
  KCODE_NSEVEN,
  KCODE_NEIGHT,
  KCODE_NNINE,
  KCODE_NADD
};

enum KCODE_ROW3 {
  KCODE_CAPS = 0x60,
  KCODE_A,
  KCODE_S,
  KCODE_D,
  KCODE_F,
  KCODE_G,
  KCODE_H,
  KCODE_J,
  KCODE_K,
  KCODE_L,
  KCODE_SCOLON,
  KCODE_APO,
  KCODE_RET,
  KCODE_NFOUR,
  KCODE_NFIVE,
  KCODE_NSIX
};

enum KCODE_ROW4 {
  KCODE_LSHIFT = 0x80,
  KCODE_Z,
  KCODE_X,
  KCODE_C,
  KCODE_V,
  KCODE_B,
  KCODE_N,
  KCODE_M,
  KCODE_COMMA,
  KCODE_DOT,
  KCODE_FSLASH,
  KCODE_RSHIFT,
  KCODE_UARROW,
  KCODE_NONE,
  KCODE_NTWO,
  KCODE_NTHREE,
  KCODE_NRET
};

enum KCODE_ROW5 {
  KCODE_LCTRL = 0xA0,
  KCODE_LMOD,
  KCODE_LALT,
  KCODE_SPACE,
  KCODE_RALT,
  KCODE_RMOD,
  KCODE_MENU,
  KCODE_RCTRL,
  KCODE_LARROW,
  KCODE_DARROW,
  KCODE_RARROW,
  KCODE_NZERO,
  KCODE_NPOINT
};

void kbd_write_wait(void);
void kbd_read_wait(void);
void kbd_disable(void);
void kbd_enable(void);
void kbd_input(void);

#endif
