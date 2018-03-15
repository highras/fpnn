#include "binary_abbr.h"
#include <string.h>

intmax_t binary_abbr(const char *str, char **end)
{
	static const char *Abbr = "KMGTPEZY";
	static const char *abbr = "kmgtpezy";

	intmax_t r = 1;
	char *s = (char *)str;
	char *p;
	int n;

	if ((p = strchr(Abbr, *s)) != NULL)
	{
		n = p - Abbr + 1;
	}
	else if ((p = strchr(abbr, *s)) != NULL)
	{
		n = p - abbr + 1;
	}
	else
	{
		n = 0;
	}

	if (n)
	{
		intmax_t x;
		if (s[1] == 'i' || s[1] == 'I')
		{
			s += 2;
			x = 1024;
		}
		else
		{
			s += 1;
			x = 1000;
		}

		switch (n)
		{
		case 9: r *= x;
		case 8: r *= x;
		case 7: r *= x;
		case 6: r *= x;
		case 5: r *= x;
		case 4: r *= x;
		case 3: r *= x;
		case 2: r *= x;
		case 1: r *= x;
		}
	}

	if (end)
	{
		*end = s;
	}

	return r;
}
