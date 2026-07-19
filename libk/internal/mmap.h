#ifndef MMAP_INTERNAL_H
#define MMAP_INTERNAL_H

#include <stddef.h>
#include <stdint.h>

#include <libk/config/mmap.h>

enum MMAP_TYPES {
	MMAP_USABLE = 1,
	MMAP_RESERVED,
	MMAP_ACPI_RECLAIMABLE,
	MMAP_ACPI_NVS,
	MMAP_BAD_MEMORY,
	MMAP_BOOTLOADER_RECLAIMABLE,
	MMAP_FRAMEBUFFER
};

struct e820_map {
	uint64_t base;
	uint64_t size;
	uint32_t type;
	uint32_t attrib;
};

struct e820_info {
	struct e820_map *base;
	size_t           nr_entries;
	size_t           max_nr_entries;
};

#define MMAP_ENTRY_SIZE  (sizeof(struct e820_map))
#define MMAP_TABLE_SIZE  (MMAP_MAX_ENTRIES * MMAP_ENTRY_SIZE)
#define MMAP_BASE_OFFSET (offsetof(struct e820_map, base))
#define MMAP_SIZE_OFFSET (offsetof(struct e820_map, size))
#define MMAP_TYPE_OFFSET (offsetof(struct e820_map, type))
#define MMAP_ATTR_OFFSET (offsetof(struct e820_map, attrib))

#define INFO_BASE_OFFSET   (offsetof(struct e820_info, base))
#define INFO_NR_ENT_OFFSET (offsetof(struct e820_info, nr_entries))
#define INFO_MAX_NR_OFFSET (offsetof(struct e820_info, max_nr_entries))

#endif
