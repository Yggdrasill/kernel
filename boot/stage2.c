int main(void)
{
  __asm__ volatile(
    "cli;"
    "hlt;"
  );

  return 0;
}
