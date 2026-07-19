#ifdef GEN_ASM_STAGE2

	#include <stddef.h>
	#include <stdint.h>
	#include <stdio.h>

	#include <libk/internal/mmap.h>

/*
 * Unfortunately this process assumes that the computer building this is at
 * least compatible with the build target. I will fix this problem in time, but
 * for now I need compile-time generation of sizes.
 */

int main(void)
{
	printf(
	    "E820_ENTRY_SIZE equ 0x%X\n"
	    "E820_INFO_BASE  equ 0x%X\n"
	    "E820_INFO_NR equ 0x%X\n"
	    "E820_INFO_MAX equ 0x%X\n",
	    MMAP_ENTRY_SIZE,
	    INFO_BASE_OFFSET,
	    INFO_NR_ENT_OFFSET,
	    INFO_MAX_NR_OFFSET);
	return 0;
}

#endif
