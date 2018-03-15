#include "strhash.h"

unsigned int strhash(const char *str, unsigned int initval)
{
	unsigned int c;
	unsigned int h = initval;
	unsigned char *p = (unsigned char *)str;
	while ((c = *p++) != 0)
		h += (h << 7) + c + 987654321;
	return h;
}

unsigned int memhash(const void *mem, size_t n, unsigned int initval)
{
	unsigned int h = initval;
	unsigned char *p = (unsigned char *)mem;
	unsigned char *end = (unsigned char *)mem + n;
	while (p < end)
		h += (h << 7) + (*p++) + 987654321;
	return h;
}

