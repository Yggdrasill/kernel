#ifdef GEN_LD_STAGE2

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
	    "#define MMAP_TABLE_SIZE 0x%X\n"
	    "#define E820_ENTRY_SIZE 0x%X\n"
	    "#define E820_INFO_BASE 0x%X\n"
	    "#define E820_INFO_NR 0x%X\n"
	    "#define E820_INFO_MAX 0x%X\n",
	    MMAP_MAX_ENTRIES * MMAP_ENTRY_SIZE,
	    MMAP_ENTRY_SIZE,
	    INFO_BASE_OFFSET,
	    INFO_NR_ENT_OFFSET,
	    INFO_MAX_NR_OFFSET);
	return 0;
}

#endif
