#include "hex.h"

static char _HEX[] = "0123456789ABCDEF";
static char _hex[] = "0123456789abcdef";

static signed char _tab[256] = {
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	 0,  1,  2,  3,  4,  5,  6,  7,  8,  9, -1, -1, -1, -1, -1, -1,
	-1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
};


int Hexlify(char *dst, const void *src, int size)
{
        char *d = dst;
	unsigned char *s = (unsigned char *)src;
        int i;
        for (i = 0; i < size; ++i)
        {
                int b1 = (s[i] >> 4);
                int b2 = (s[i] & 0x0f);
                *d++ = _HEX[b1];
                *d++ = _HEX[b2];
        }
        *d = 0;
	return d - dst;
}

int hexlify(char *dst, const void *src, int size)
{
        char *d = dst;
	unsigned char *s = (unsigned char *)src;
        int i;
        for (i = 0; i < size; ++i)
        {
                int b1 = (s[i] >> 4);
                int b2 = (s[i] & 0x0f);
                *d++ = _hex[b1];
                *d++ = _hex[b2];
        }
        *d = 0;
	return d - dst;
}

int unhexlify(void *dst, const char *src, int size)
{
	unsigned char *s = (unsigned char *)src;
	unsigned char *end = size < 0 ? (unsigned char *)-1 : s + size;
	char *d = (char *)dst;
	int b1, b2;

	while (s + 2 <= end && (b1 = _tab[*s++]) >= 0 && (b2 = _tab[*s++]) >= 0)
	{
		*d++ = (b1 << 4) + b2;
	}

	if (s != end && s[-1] != 0)
	{
		return -(s - 1 - (unsigned char *)src);
	}

	return d - (char *)dst;
}


#ifdef TEST_HEX

#include <stdio.h>
#include <assert.h>
#include <string.h>

int main()
{
	char buf[1024];
	char *src = "hello, world!";
	int len = strlen(src);
	int i;

	for (i = 0; i < 1024*1024; ++i)
	{
		hexlify(buf, src, len);
		unhexlify(buf, buf, -1);
	}

	len = Hexlify(buf, src, len);
	printf("%d\t%s\t%s\n", len, src, buf);

	len = unhexlify(buf, buf, -1);
	buf[len] = 0;
	printf("%d\t%s\t%s\n", len, src, buf);

	return 0;
}

#endif
