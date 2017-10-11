#ifndef IDT_H
#define IDT_H

#include "stdint.h"

#define IDT_PTR_LOCATION  0x37E00
#define IDT_BASE_OFFSET   (IDT_PTR_LOCATION + 0x08)
#define IDT_ENTRY_NUM     256

/*
 * Little endian byte alignment follows. These structures are manually packed
 * because it avoids using compiler pragmas. I do not know of any x86 compiler
 * that would pad a struct that contains only byte-aligned types.
 *
 * Both of these structs will be 8-byte aligned manually.
 */

struct idt_ptr {
  unsigned char limit_0;
  unsigned char limit_8;
  unsigned char base_0;
  unsigned char base_8;
  unsigned char base_16;
  unsigned char base_24;
};

struct idt_entry {
  unsigned char offset_0;
  unsigned char offset_8;
  unsigned char selector_0;
  unsigned char selector_8;
  unsigned char zero;
  unsigned char flags;
  unsigned char offset_16;
  unsigned char offset_24;
};

struct idt_ptr *idt_init(void);
void idt_set_entry(struct idt_entry *,
                   void *,
                   void (*)(void),
                   uint16_t,
                   unsigned char);
void idt_install(struct idt_ptr *);


#endif
