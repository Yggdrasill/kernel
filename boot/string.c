#include "string.h"

void clear(void)
{
    __asm__ volatile(
      "push   edi;"
      "mov    eax, 0x0720;"
      "mov    ecx, 0xFA0;"
      "mov    edi, 0xB8000;"
      "rep    stosw;"
      "pop    edi;"
    );

    return;
}

unsigned long long strlen(char *str)
{
  unsigned long long retval;

  retval = 0;

  while(*str++) retval++;

  return retval;
}

void putchar(char ch)
{
  short *vga;

  static int y;
  static int x;

  if(x >= 160 || ch == '\n') {
    x = 0;
    y++;
  }
  if(y >= 25) y = 0;
  if(ch == '\n') return;

  vga = (short *)(0xB8000 + (y * 160) + x);
  *vga = 0x0700 | ch;
  x += 2;

  return;
}

void puts(char *str)
{
  while(*str) {
    putchar(*str);
    str++;
  }

  putchar('\n');
}
