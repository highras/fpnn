#include "httpcode.h"

struct code_desc_struct {
	int code;
	const char *desc;
};

struct code_desc_struct _cds[] =
{
#define HCS(code, name, desc)	{ code, desc },
	HTTPCODES
#undef HCS
};

#define ARRCOUNT(arr)	(sizeof(arr) / sizeof((arr)[0]))

const char *httpcode_description(int code)
{
	int i;
	for (i = 0; i < ARRCOUNT(_cds); ++i)
	{
		if (code == _cds[i].code)
			return _cds[i].desc;
	}
	return "";
}
