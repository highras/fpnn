#ifndef CRC64_H_
#define CRC64_H_ 1

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* old_crc should be set to 0, before the first call of crc64_update(). */
uint64_t crc64_update(uint64_t oldcrc, const void *buf, size_t size);
uint64_t crc64_checksum(const void *buf, size_t size);

uint64_t crc64_update_cstr(uint64_t oldcrc, const char *str);
uint64_t crc64_checksum_cstr(const char *str);


#ifdef __cplusplus
}
#endif

#endif
