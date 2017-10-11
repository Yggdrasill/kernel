#include "idt.h"
#include "stdint.h"

struct idt_ptr *idt_init(void)
{
  unsigned char *arr_limit;
  unsigned char *arr_base;

  uint32_t base;
  uint16_t limit;

  struct idt_ptr *idtr;

  idtr = (struct idt_ptr *)IDT_PTR_LOCATION;

  limit = sizeof(struct idt_entry) * IDT_ENTRY_NUM - 1;
  base = (uint32_t)IDT_BASE_OFFSET;

  arr_limit = (unsigned char *)&limit;
  arr_base = (unsigned char *)&base;

  idtr->limit_0 = arr_limit[0];
  idtr->limit_8 = arr_limit[1];

  idtr->base_0 = arr_base[0];
  idtr->base_8 = arr_base[1];
  idtr->base_16 = arr_base[2];
  idtr->base_24 = arr_base[3];

  return idtr;
}

void idt_set_entry(struct idt_entry *entry,
                   void *target_offset,
                   void (*idt_handler)(void),
                   uint16_t select,
                   unsigned char flags)
{
  intptr_t raw_ptr;
  unsigned char *offset;
  unsigned char *selector;

  raw_ptr = (intptr_t)idt_handler;
  offset = (unsigned char *)&raw_ptr;
  selector = (unsigned char *)&select;

  entry->offset_0 = offset[0];
  entry->offset_8 = offset[1];
  entry->offset_16 = offset[2];
  entry->offset_24 = offset[3];

  entry->selector_0 = selector[0];
  entry->selector_8 = selector[1];

  entry->zero = 0;
  entry->flags = flags;

  return;
}

void idt_install(struct idt_ptr *idtr)
{
  __asm__ volatile(
    "mov  eax, %0;"
    "lidt [eax];" : : "m"(idtr)
  );

  return;
}
