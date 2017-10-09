#include "string.h"

int main(void)
{
  clear();

  puts("Hello world!");

  __asm__ volatile(
    "cli;"
    "hlt;"
  );

  return 0;
}
