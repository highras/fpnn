#ifndef HASHINT_H
#define HASHINT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif


uint32_t hash32_uint(unsigned int a);
uint32_t hash32_ulong(unsigned long a);
uint32_t hash32_ulonglong(unsigned long long a);
uint32_t hash32_uintptr(uintptr_t a);


uint32_t hash32_uint32(uint32_t a);
uint32_t hash32_uint64(uint64_t a);


uint64_t hash64_uint64(uint64_t a);


uint32_t hash32_mix(uint32_t a, uint32_t b, uint32_t c);


#ifdef __cplusplus
}
#endif

#endif
