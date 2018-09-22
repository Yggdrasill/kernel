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

/*
 * KNOWN BUGS:
 *
 * - If an extended scan code and a regular scan code are sent at the same time,
 *   the driver may end up handling the key presses incorrectly.
 * - A malformed input can be sent to make the machine hang.
 * - A malformed input can be sent to make memory accesses that shouldn't be
 *   allowed.
 *
 * All of these bugs will be fixed in time.
 *
 */

static int state;

const unsigned char kbd_scan_table[] = {
  0x00,         KCODE_F9,     0x00,         KCODE_F5,     /* 0x00 - 0x03 */
  KCODE_F3,     KCODE_F1,     KCODE_F2,     KCODE_F12,    /* 0x04 - 0x07 */
  0x00,         KCODE_F10,    KCODE_F8,     KCODE_F6,     /* 0x08 - 0x0B */
  KCODE_F4,     KCODE_TAB,    KCODE_TILDE,  0x00,         /* 0x0C - 0x0F */
  0x00,         KCODE_LALT,   KCODE_LSHIFT, 0x00,         /* 0x10 - 0x13 */
  KCODE_LCTRL,  KCODE_Q,      KCODE_ONE,    0x00,         /* 0x14 - 0x17 */
  0x00,         0x00,         KCODE_Z,      KCODE_S,      /* 0x18 - 0x1B */
  KCODE_A,      KCODE_W,      KCODE_TWO,    0x00,         /* 0x1C - 0x1F */
  0x00,         KCODE_C,      KCODE_X,      KCODE_D,      /* 0x20 - 0x23 */
  KCODE_E,      KCODE_FOUR,   KCODE_THREE,  0x00,         /* 0x24 - 0x27 */
  0x00,         KCODE_SPACE,  KCODE_V,      KCODE_F,      /* 0x28 - 0x2B */
  KCODE_T,      KCODE_R,      KCODE_FIVE,   0x00,         /* 0x2C - 0x2F */
  0x00,         KCODE_N,      KCODE_B,      KCODE_H,      /* 0x30 - 0x33 */
  KCODE_G,      KCODE_Y,      KCODE_SIX,    0x00,         /* 0x34 - 0x37 */
  0x00,         0x00,         KCODE_M,      KCODE_J,      /* 0x38 - 0x3B */
  KCODE_U,      KCODE_SEVEN,  KCODE_EIGHT,  0x00,         /* 0x3C - 0x3F */
  0x00,         KCODE_COMMA,  KCODE_K,      KCODE_I,      /* 0x40 - 0x43 */
  KCODE_O,      KCODE_ZERO,   KCODE_NINE,   0x00,         /* 0x44 - 0x47 */
  0x00,         KCODE_DOT,    KCODE_FSLASH, KCODE_L,      /* 0x48 - 0x4B */
  KCODE_SCOLON, KCODE_P,      KCODE_DASH,   0x00,         /* 0x4C - 0x4F */
  0x00,         0x00,         KCODE_APO,    0x00,         /* 0x50 - 0x53 */
  KCODE_LSQBRK, KCODE_EQUAL,  0x00,         0x00,         /* 0x54 - 0x57 */
  KCODE_CAPS,   KCODE_RSHIFT, KCODE_RET,    KCODE_RSQBRK, /* 0x58 - 0x5B */
  0x00,         KCODE_BSLASH, 0x00,         0x00,         /* 0x5C - 0x5F */
  0x00,         0x00,         0x00,         0x00,         /* 0x60 - 0x63 */
  0x00,         0x00,         KCODE_BCKSP,  0x00,         /* 0x64 - 0x67 */
  0x00,         KCODE_NONE,   0x00,         KCODE_NFOUR,  /* 0x68 - 0x6B */
  KCODE_NSEVEN, 0x00,         0x00,         0x00,         /* 0x6C - 0x6F */
  KCODE_NZERO,  KCODE_NPOINT, KCODE_NTWO,   KCODE_NFIVE,  /* 0x70 - 0x73 */
  KCODE_NSIX,   KCODE_NEIGHT, KCODE_ESC,    KCODE_NLOCK,  /* 0x74 - 0x77 */
  KCODE_F11,    KCODE_NADD,   KCODE_NTHREE, KCODE_NSUB,   /* 0x78 - 0x7B */
  KCODE_NMUL,   KCODE_NNINE,  KCODE_SCRLK,  0x00,         /* 0x7C - 0x7F */
  0x00,         0x00,         0x00,         KCODE_F7      /* 0x80 - 0x83 */
};

const unsigned char kbd_ext_scan_table[] = {
  0x00,         0x00,         0x00,         0x00,         /* 0x00 - 0x03 */
  0x00,         0x00,         0x00,         0x00,         /* 0x04 - 0x07 */
  0x00,         0x00,         0x00,         0x00,         /* 0x08 - 0x0B */
  0x00,         0x00,         0x00,         0x00,         /* 0x0C - 0x0F */
  KCODE_MULTI,  KCODE_RALT,   0x00,         0x00,         /* 0x10 - 0x13 */
  KCODE_RCTRL,  KCODE_MULTI,  0x00,         0x00,         /* 0x14 - 0x17 */
  KCODE_MULTI,  0x00,         0x00,         0x00,         /* 0x18 - 0x1B */
  0x00,         0x00,         0x00,         KCODE_LMOD,   /* 0x1C - 0x1F */
  KCODE_MULTI,  KCODE_MULTI,  0x00,         KCODE_MULTI,  /* 0x20 - 0x23 */
  0x00,         0x00,         0x00,         KCODE_RMOD,   /* 0x24 - 0x27 */
  KCODE_MULTI,  0x00,         0x00,         KCODE_MULTI,  /* 0x28 - 0x2B */
  0x00,         0x00,         0x00,         KCODE_MENU,   /* 0x2C - 0x2F */
  KCODE_MULTI,  0x00,         KCODE_MULTI,  0x00,         /* 0x30 - 0x33 */
  KCODE_MULTI,  0x00,         0x00,         KCODE_ACPI,   /* 0x34 - 0x37 */
  KCODE_MULTI,  0x00,         KCODE_MULTI,  KCODE_MULTI,  /* 0x38 - 0x3B */
  0x00,         0x00,         0x00,         KCODE_ACPI,   /* 0x3C - 0x3F */
  KCODE_MULTI,  0x00,         0x00,         0x00,         /* 0x40 - 0x43 */
  KCODE_MULTI,  0x00,         0x00,         0x00,         /* 0x44 - 0x47 */
  KCODE_MULTI,  0x00,         KCODE_NDIV,   0x00,         /* 0x48 - 0x4B */
  0x00,         KCODE_MULTI,  0x00,         0x00,         /* 0x4C - 0x4F */
  KCODE_MULTI,  0x00,         0x00,         0x00,         /* 0x50 - 0x53 */
  0x00,         0x00,         0x00,         0x00,         /* 0x54 - 0x57 */
  0x00,         0x00,         KCODE_NRET,   0x00,         /* 0x58 - 0x5B */
  0x00,         0x00,         KCODE_ACPI,   0x00,         /* 0x5C - 0x5F */
  0x00,         0x00,         0x00,         0x00,         /* 0x60 - 0x63 */
  0x00,         0x00,         0x00,         0x00,         /* 0x64 - 0x67 */
  0x00,         KCODE_END,    0x00,         KCODE_LARROW, /* 0x68 - 0x6B */
  KCODE_HOME,   0x00,         0x00,         0x00,         /* 0x6C - 0x6F */
  KCODE_INS,    KCODE_DEL,    KCODE_DARROW, 0x00,         /* 0x70 - 0x73 */
  0x00,         0x00,         0x00,         0x00,         /* 0x74 - 0x77 */
  0x00,         0x00,         KCODE_PGDN,   0x00,         /* 0x78 - 0x7B */
  0x00,         KCODE_PGUP,   0x00,         0x00          /* 0x7C - 0x7F */
};

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

void kbd_init(void)
{
  unsigned char kbd;

  kbd_disable();
  kbd_write_wait();
  outb(0x64, 0x20);
  kbd_read_wait();
  kbd = inb(0x60) ^ (1 << 6);
  kbd_write_wait();
  outb(0x64, 0x60);
  outb(0x60, kbd);
  kbd_enable();

  return;
}


void kbd_ext_mb_prtsc(void)
{
  static int rx_bytes;
  unsigned char sc;

  sc = inb(0x60);
  if(sc != 0xE0 && sc != 0x12 && sc != 0x7C && sc != 0xF0) {
    rx_bytes = 0;
    state = KBSTATE_NONE;
  } else {
    rx_bytes++;
  }

  if(state == KBSTATE_PRTSCMK && rx_bytes == 2) {
    puthex(KCODE_PRTSC);
    state = KBSTATE_NONE;
    rx_bytes = 0;
  } else if(state == KBSTATE_PRTSCBRK && rx_bytes == 3) {
    puthex(KCODE_PRTSC);
    state = KBSTATE_NONE;
    rx_bytes = 0;
  }

  return;
}

void kbd_ext_mb_pause(void)
{
  static int rx_bytes;
  unsigned char sc;

  sc = inb(0x60);
  if(sc != 0xE1 && sc != 0x14 && sc != 0x77 && sc != 0xF0) {
    rx_bytes = 0;
    state = KBSTATE_NONE;
  } else {
    rx_bytes++;
  }

  if(rx_bytes == 7) {
    puthex(KCODE_PAUSE);
    state = KBSTATE_NONE;
    rx_bytes = 0;
  }

  return;
}

void kbd_ext_mb_input(void)
{
  switch(state) {
    case KBSTATE_PRTSCMK:
    case KBSTATE_PRTSCBRK:
      kbd_ext_mb_prtsc();
      break;
    case KBSTATE_PAUSE:
      kbd_ext_mb_pause();
      break;
  }

  return;
}

void kbd_ext_input(void)
{
  unsigned char sc;

  sc = inb(0x60);
  if(sc == KBD_BRK) {
    state = KBSTATE_EXTBRK;
  } else if(state == KBSTATE_EXTBRK && sc == 0x7C) {
    state = KBSTATE_PRTSCBRK;
  } else if(state == KBSTATE_EXTBRK && sc != 0x7C) {
    puthex(kbd_ext_scan_table[sc]);
    state = KBSTATE_NONE;
  } else if(sc == 0x12) {
    state = KBSTATE_PRTSCMK;
  } else {
    puthex(kbd_ext_scan_table[sc]);
    state = KBSTATE_NONE;
  }

  return;
}

void kbd_std_input(void)
{
  unsigned char sc;

  sc = inb(0x60);
  switch(state) {
    case KBSTATE_STDMK:
      puthex(kbd_scan_table[sc]);
      break;
    case KBSTATE_STDBRK:
      puthex(kbd_scan_table[sc]);
      break;
  }

  state = KBSTATE_NONE;
  return;
}

void kbd_state(void)
{
  unsigned char sc;

  sc = inb(0x60);
  switch(sc) {
    case KBD_EXT:
      state = KBSTATE_EXT;
      break;
    case KBD_PAUSE:
      state = KBSTATE_PAUSE;
      break;
    case KBD_BRK:
      state = KBSTATE_STDBRK;
      break;
    default:
      state = KBSTATE_STDMK;
      kbd_std_input();
  }

  return;
}

void kbd_input(void)
{
  kbd_read_wait();
  switch(state & 0x70) {
    case KBSTATE_NONE:
      kbd_state();
      break;
    case KBSTATE_STD:
      kbd_std_input();
      break;
    case KBSTATE_EXT:
      kbd_ext_input();
      break;
    case KBSTATE_EXTMB:
      kbd_ext_mb_input();
      break;
    default:
      puts("Unknown state");
      state = KBSTATE_NONE;
  }

  return;
}
