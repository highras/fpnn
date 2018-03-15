#ifndef BIT_H_
#define BIT_H_ 1

#include <limits.h>
#include <stdlib.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif


#define BITMAP_SET(map, p)	((void)(((char*)(map))[(p)/CHAR_BIT] |= 1<<(p)%CHAR_BIT))
#define BITMAP_CLEAR(map, p)	((void)(((char*)(map))[(p)/CHAR_BIT] &= ~(1<<(p)%CHAR_BIT)))
#define BITMAP_FLIP(map, p)	((void)(((char*)(map))[(p)/CHAR_BIT] ^= 1<<(p)%CHAR_BIT))
#define BITMAP_TEST(map, p)	(((char*)(map))[(p)/CHAR_BIT] & (1<<(p)%CHAR_BIT))


#define IS_POWER_TWO(w)		(((w) & -(w)) == (w))
#define BIT_IS_SUBSET(sub, u)	(((sub) & (u)) == (sub))
#define BIT_LEFT_ROTATE(w, s)	(((w) << (s)) | ((w) >> (sizeof(w)*CHAR_BIT-(s))))
#define BIT_RIGHT_ROTATE(w, s)	(((w) << (sizeof(w)*CHAR_BIT-(s))) | ((w) >> (s)))
#define BIT_ALIGN(n, align)	(((n) + (align) - 1) & ~((align) - 1))


int bitmap_find1(const unsigned char *bitmap, size_t start, size_t end);
int bitmap_find0(const unsigned char *bitmap, size_t start, size_t end);

int bit_parity(unsigned int w);
int bit_count(uintmax_t x);
uintmax_t round_up_power_two(uintmax_t x);
uintmax_t round_down_power_two(uintmax_t x);


#ifdef __cplusplus
}
#endif

#endif

