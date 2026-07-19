#include <libk/internal/mmap.h>
#include <stddef.h>
#include <stdint.h>

/*
 * Unfortunately this process assumes that the computer building this is at
 * least compatible with the build target. I will fix this problem in time, but
 * for now I need compile-time generation of sizes.
 */

volatile const uint32_t ABI_MMAP_TABLE_SIZE = MMAP_TABLE_SIZE;
volatile const uint32_t ABI_MMAP_ENTRY_SIZE = MMAP_ENTRY_SIZE;
volatile const uint32_t ABI_MMAP_INFO_BASE  = INFO_BASE_OFFSET;
volatile const uint32_t ABI_MMAP_INFO_NR    = INFO_NR_ENT_OFFSET;
volatile const uint32_t ABI_MMAP_INFO_MAX   = INFO_MAX_NR_OFFSET;

int main(void)
{
	return 0;
}
