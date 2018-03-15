#include "bit.h"

int bitmap_find1(const unsigned char *bitmap, size_t start, size_t end)
{
	size_t i, s, e, b, bstart, bend;
	if (start >= end)
		return -1;

	s = start / CHAR_BIT;
	i = start % CHAR_BIT;
	if ((bitmap[s] >> i) != 0)
	{
		b = s;
		bstart = i;
		bend = CHAR_BIT;
	}
	else
	{
		e = (end - 1) / CHAR_BIT;
		for (b = s + 1; b <= e && bitmap[b] == 0; ++b)
			continue;
		if (b > e)
			return -1;

		if (b == e)
		{
			bend = end % CHAR_BIT;
			if (bend == 0)
				bend = CHAR_BIT;
		}
		else
		{
			bend = CHAR_BIT;
		}
		bstart = 0;
	}

	for (i = bstart; i < bend; ++i)
	{
		if (bitmap[b] & (1 << i))
			return b * CHAR_BIT + i;
	}

	return -1;
}

int bitmap_find0(const unsigned char *bitmap, size_t start, size_t end)
{
	size_t i, s, e, b, bstart, bend;
	if (start >= end)
		return -1;

	s = start / CHAR_BIT;
	i = start % CHAR_BIT;
	if ((bitmap[s] >> i) != (0xFF >> i))
	{
		b = s;
		bstart = i;
		bend = CHAR_BIT;
	}
	else
	{
		e = (end - 1) / CHAR_BIT;
		for (b = s + 1; b <= e && bitmap[b] == 0xFF; ++b)
			continue;
		if (b > e)
			return -1;

		if (b == e)
		{
			bend = end % CHAR_BIT;
			if (bend == 0)
				bend = CHAR_BIT;
		}
		else
		{
			bend = CHAR_BIT;
		}
		bstart = 0;
	}

	for (i = bstart; i < bend; ++i)
	{
		if (!(bitmap[b] & (1 << i)))
			return b * CHAR_BIT + i;
	}

	return -1;
}

int bit_parity(unsigned int w)
{
	w ^= w>>1;
	w ^= w>>2;
	w ^= w>>4;
	w ^= w>>8;
	w ^= w>>16;
	return w & 1;
}

int bit_count(uintmax_t x)
{
	int n = 0;

        if (x)
	{
		do {
			n++;            
		} while ((x &= x-1));
	}

        return n;
}

uintmax_t round_up_power_two(uintmax_t x)
{
        int r = 1;

        if (x == 0 || (intmax_t)x < 0)
                return 0;
        --x;
        while (x >>= 1)
                r++;
	return (UINTMAX_C(1) << r);
}

uintmax_t round_down_power_two(uintmax_t x)
{
        int r = 0;

        if (x == 0)
                return 0;
        while (x >>= 1)
                r++;
	return (UINTMAX_C(1) << r);
}

