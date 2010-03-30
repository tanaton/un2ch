#ifndef UNMAP_CRC32_H_INCLUDE
#define UNMAP_CRC32_H_INCLUDE

#include <string.h>

#define UNMAP_CRC32_TABLE_SIZE		(256)

unmap_hash_t unmap_hash_create(const char *str, size_t size);

#endif /* UNMAP_CRC32_H_INCLUDE */

