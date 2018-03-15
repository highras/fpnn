#include "base64.h"
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

const base64_t std_base64 = {
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=",

	"AAAAAAAAAAAAAAAA"
	"AAAAAAAAAAAAAAAA"
	"AAAAAAAAAAA>AAA?"
	"456789:;<=AAA@AA"
	"A\x00\x01\x02\x03\x04\x05\x06\x07\x08\t\n\x0b\x0c\r\x0e"
	"\x0f\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19" "AAAAA"
	"A\x1a\x1b\x1c\x1d\x1e\x1f !\"#$%&'("
	")*+,-./0123AAAAA"
	"AAAAAAAAAAAAAAAA"
	"AAAAAAAAAAAAAAAA"
	"AAAAAAAAAAAAAAAA"
	"AAAAAAAAAAAAAAAA"
	"AAAAAAAAAAAAAAAA"
	"AAAAAAAAAAAAAAAA"
	"AAAAAAAAAAAAAAAA"
	"AAAAAAAAAAAAAAAA"
};

const base64_t url_base64 = {
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_=",

	"AAAAAAAAAAAAAAAA"
	"AAAAAAAAAAAAAAAA"
	"AAAAAAAAAAAAA>AA"
	"456789:;<=AAA@AA"
	"A\x00\x01\x02\x03\x04\x05\x06\x07\x08\t\n\x0b\x0c\r\x0e"
	"\x0f\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19" "AAAA?"
	"A\x1a\x1b\x1c\x1d\x1e\x1f !\"#$%&'("
	")*+,-./0123AAAAA"
	"AAAAAAAAAAAAAAAA"
	"AAAAAAAAAAAAAAAA"
	"AAAAAAAAAAAAAAAA"
	"AAAAAAAAAAAAAAAA"
	"AAAAAAAAAAAAAAAA"
	"AAAAAAAAAAAAAAAA"
	"AAAAAAAAAAAAAAAA"
	"AAAAAAAAAAAAAAAA"
};


int base64_init(base64_t *b64, const char *alphabet)
{
	int i;
	unsigned char *a = b64->alphabet;
	int len = strlen(alphabet);

	if (len < 64)
		return -1;

	memcpy(a, alphabet, 64);
	a[64] = alphabet[64] ? alphabet[64] : '=';
	a[65] = '\0';

	/* check duplicate charcaters. */
	for (i = 0; i < 65; ++i)
	{
		if (strchr((char *)a+i+1, a[i]))
			return -1;
	}

	memset(b64->detab, 65, 256);
	for (i = 0; i < 65; ++i)
		b64->detab[a[i]] = i;

	return 0;
}

ssize_t base64_encode(const base64_t *b64, char *out, const void *in, size_t len, int flag)
{
	unsigned char *etab = (unsigned char *)b64->alphabet;
	unsigned char *d = (unsigned char *)out;
	unsigned char *s = (unsigned char *)in;
	unsigned char *end;
	int c0, c1, c2;
	int remain;
	bool auto_newline = (flag & BASE64_AUTO_NEWLINE);

	if ((ssize_t)len < 0)
		len = 0;

	remain = len % 3;
	end = s + len - remain;

	for (; s < end; )
	{
		c0 = *s++;
		c1 = *s++;
		c2 = *s++;
		*d++ = etab[c0 >> 2];
		*d++ = etab[(c0 << 4 | c1 >> 4) & 0x3F];
		*d++ = etab[(c1 << 2 | c2 >> 6) & 0x3F];
		*d++ = etab[c2 & 0x3F];
		if (auto_newline && (d + 1 - (unsigned char *)out) % 77 == 0)
			*d++ = '\n';
	}

	switch (remain)
	{
	case 1:
		c0 = *s++;
		*d++ = etab[c0 >> 2];
		*d++ = etab[(c0 << 4) & 0x3F];
		if (!(flag & BASE64_NO_PADDING))
		{
			*d++ = etab[64];
			*d++ = etab[64];
		}
		break;
	case 2:
		c0 = *s++;
		c1 = *s++;
		*d++ = etab[c0 >> 2];
		*d++ = etab[(c0 << 4 | c1 >> 4) & 0x3F];
		*d++ = etab[(c1 << 2) & 0x3F];
		if (!(flag & BASE64_NO_PADDING))
		{
			*d++ = etab[64];
		}
		break;
	}

	*d = '\0';
	return d - (unsigned char *)out;
}

ssize_t base64_decode(const base64_t *b64, void *out, const char *in, size_t len, int flag)
{
	unsigned char *detab = (unsigned char *)b64->detab;
	unsigned char *d = (unsigned char *)out;
	unsigned char *s = (unsigned char *)in;
	unsigned char *end = ((ssize_t)len < 0) ? (unsigned char *)-1 : s + len;
	int c, x, r, n;

	for (r = 0, n = 0; s < end && (c = *s++) != 0; )
	{
		x = detab[c];
		if (x >= 64)
		{
			if (x == 64)
				break;
			else if (flag & BASE64_IGNORE_NON_ALPHABET)
			{
				while ((c = *s++) != 0 && (x = detab[c]) > 64)
					continue;
				if (x >= 64)
					break;
			}
			else if (flag & BASE64_IGNORE_SPACE)
			{
				while ((c = *s++) != 0 && ((x = detab[c]) > 64) && isspace(c))
					continue;
				if (x >= 64)
					break;
			}
			else
				break;
		}

		switch (n)
		{
		case 0:
			++n;
			r = x << 2;
			break;
		case 1:
			++n;
			*d++ = r + (x >> 4);
			r = x << 4;
			break;
		case 2:
			++n;
			*d++ = r + (x >> 2);
			r = x << 6;
			break;
		case 3:
			n = 0;
			*d++ = r + x;
			break;
		}
	}

	if ((s < end && c != 0 && c != b64->alphabet[64]) || (n == 1) || (n && (r & 0xff)))
	{
		return -(s - (unsigned char *)in);
	}

	return d - (unsigned char *)out;
}


#ifdef TEST_BASE64

#include <assert.h>
#include <stdio.h>

int main()
{
	base64_t b64;
	int len;
	char buf[256];
	char abc[] = 
		"abcdefghijklmnopqrstuvwxyz"
		"abcdefghijklmnopqrstuvwxyz"
		"abcdefghijklmnopqrstuvwxyz"
		;
	int abc_len = strlen(abc);

	assert(base64_init(&b64, std_base64.alphabet) >= 0);
	len = base64_encode(&b64, buf, abc, abc_len, BASE64_AUTO_NEWLINE);
	printf("%d:\n%s\n", len, buf);

	memset(abc, 0, abc_len);
	len = base64_decode(&b64, abc, buf, -1, BASE64_IGNORE_SPACE);
	printf("%d:\n%s\n", len, abc);

	return 0;
}

#endif

