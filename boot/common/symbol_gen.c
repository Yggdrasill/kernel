#include "disk.h"

#include <stddef.h>
#include <stdint.h>

volatile const uint32_t ABI_DISK_CYLINDERS = DISK_CYLINDERS_OFFSET;
volatile const uint32_t ABI_DISK_HEADS     = DISK_HEADS_OFFSET;
volatile const uint32_t ABI_DISK_SECTORS   = DISK_SECTORS_OFFSET;
volatile const uint32_t ABI_DISK_DRIVES    = DISK_DRIVES_OFFSET;
volatile const uint32_t ABI_DISK_SIZEOF    = sizeof(struct disk_info);

volatile const uint32_t ABI_STAGE2_LBA = 4;

int main(void)
{
    return 0;
}
