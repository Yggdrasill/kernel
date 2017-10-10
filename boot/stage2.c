#include "string.h"

/*
 * This is the entry point for stage 2. Do note that there should never, ever be
 * any function in this file except main(). The reason for this is that the
 * compiler may reorder functions at will, and forcing the linker to always keep
 * main() on top of the flat binary to ensure the correct entry point is used
 * has turned out to be either impossible or difficult without using
 * GCC-specific pragmas, which I don't want to do. The linker instead puts this
 * file on top of the text segment, which is very easy to make it do, and since
 * main() is the only function here it should always be the entry point.
 *
 * main() should *never* return. There is no return pointer on the stack, and so
 * if it ever returns it could turn out to be very ugly. If execution needs to
 * stop the programmer should disable interrupts and use inline asm for the hlt
 * instruction.
 *
 * Note that all __asm__ should be followed by the volatile keyword to prevent
 * the compiler from optimising it out of the code.
 *
 * System state at this point:
 *
 * - The A20 gate should be enabled before entering main()
 * - The IDT should be installed and it should be zero length at address zero
 * - The GDT should be installed and have a flat memory map, both the code and
 *   data descriptor
 * - Protected mode should be enabled
 * - The EFLAGS register should be cleared
 * - The PIC should have all interrupts masked
 *
 * What should be done in stage 2:
 *
 * - The bootloader should set up a relatively usable IDT, although the kernel
 *   will very likely want to replace it later. This is needed because we want
 *   the keyboard to work.
 * - The bootloader should unmask some of the interrupts on the 8259 PICs, for
 *   example the keyboard interrupt.
 * - It should find the boot partition (partition table should be in memory at
 *   0x7DBE) and read the hard drive.
 * - The partition should be an ext2 partition and the bootloader should read
 *   it, find a configuration file which will tell it what the kernel image is
 *   called and what it should put on the kernel command line.
 * - It should not set video mode, nor should it ever enter v8086 mode.
 *
 * Preferably, the code for the ATA driver and ext2 driver should be shared with
 * the kernel, although of course, since this will be a flat binary file, it
 * will be statically linked.
 *
 */

int main(void)
{
  clear();

  puts("Hello world!");

  __asm__ volatile(
    "cli;"
    "hlt;"
  );
}
