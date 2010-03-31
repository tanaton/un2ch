#ifndef UNMAP_CRC32_H_INCLUDE
#define UNMAP_CRC32_H_INCLUDE

#include <string.h>

#define UNMAP_CRC32_TABLE_SIZE		(256)

void unmap_hash_create(const char *str, size_t size, size_t max_level, unmap_box_t *box);

#endif /* UNMAP_CRC32_H_INCLUDE */

