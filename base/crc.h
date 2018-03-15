#ifndef CRC_H_
#define CRC_H_ 1

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* old_crc should be set to 0, before the first call of crc32_update(). */
uint32_t crc32_update(uint32_t old_crc, const void *buf, size_t size);
uint32_t crc32_checksum(const void *buf, size_t size);

uint32_t crc32_update_cstr(uint32_t old_crc, const char *str);
uint32_t crc32_checksum_cstr(const char *str);


/* old_crc should be set to 0, before the first call of crc16_update(). */
uint16_t crc16_update(uint16_t old_crc, const void *buf, size_t size);
uint16_t crc16_checksum(const void *buf, size_t size);

uint16_t crc16_update_cstr(uint16_t old_crc, const char *str);
uint16_t crc16_checksum_cstr(const char *str);


#ifdef __cplusplus
}
#endif

#endif

