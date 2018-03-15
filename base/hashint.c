#include "hashint.h"

/*
  see:  http://www.cris.com/~Ttwang/tech/inthash.htm
 */

inline uint32_t hash32_uint32(uint32_t key)
{
	key = ~key + (key << 15); 	/* key = (key << 15) - key - 1; */
	key = key ^ (key >> 12);
	key = key + (key << 2);
	key = key ^ (key >> 4);
	key = key * 2057; 		/* key = (key + (key << 3)) + (key << 11); */
	key = key ^ (key >> 16);
	return key;
}

inline uint32_t hash32_uint64(uint64_t key)
{
	key = (~key) + (key << 18); 	/* key = (key << 18) - key - 1; */
	key = key ^ (key >> 31);
	key = key * 21;			/* key = (key + (key << 2)) + (key << 4); */
	key = key ^ (key >> 11);
	key = key + (key << 6);
	key = key ^ (key >> 22);
	return (uint32_t)key;
}

inline uint64_t hash64_uint64(uint64_t key)
{
	key = (~key) + (key << 21); 	/* key = (key << 21) - key - 1; */
	key = key ^ (key >> 24);
	key = (key + (key << 3)) + (key << 8); 	/* key * 265 */
	key = key ^ (key >> 14);
	key = (key + (key << 2)) + (key << 4); 	/* key * 21 */
	key = key ^ (key >> 28);
	key = key + (key << 31);
	return key;
}

inline uint32_t hash32_uintptr(uintptr_t key)
{
	return (sizeof(uintptr_t) > sizeof(uint32_t)) ? hash32_uint64(key) : hash32_uint32(key);
}

inline uint32_t hash32_uint(unsigned int key)
{
	return hash32_uint32(key);
}

inline uint32_t hash32_ulong(unsigned long key)
{
	return (sizeof(unsigned long) > sizeof(uint32_t)) ? hash32_uint64(key) : hash32_uint32(key);
}

inline uint32_t hash32_ulonglong(unsigned long long key)
{
	return hash32_uint64(key);
}


#define rot(x,k) (((x)<<(k)) | ((x)>>(32-(k))))

inline uint32_t hash32_mix(uint32_t a, uint32_t b, uint32_t c)
{
	a -= c;  a ^= rot(c, 4);  c += b;
	b -= a;  b ^= rot(a, 6);  a += c;
	c -= b;  c ^= rot(b, 8);  b += a;
	a -= c;  a ^= rot(c,16);  c += b;
	b -= a;  b ^= rot(a,19);  a += c;
	c -= b;  c ^= rot(b, 4);  b += a;
	return c;
}

