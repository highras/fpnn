#include "Endian.h"

using namespace fpnn;

const Endian::BOMItem Endian::BOMTable[4] = {
	{Endian::UNKNOWN, 0, { 0, 0, 0, 0 }}, 
	{Endian::UCS2B, 2, { 0xFE, 0xFF, 0, 0 }}, 
	{Endian::UCS2L, 2, { 0xFF, 0xFE, 0, 0 }}, 
	{Endian::UTF8,  3, { 0xEF, 0xBB, 0xBF, 0 }}
}; 

const Endian::BOMTypeKind Endian::BOMType(const char * BOM)
{
	if((BOMTable[1].BOM[0] == BOM[0]) && (BOMTable[1].BOM[1] == BOM[1]))
		return BOMTable[1].type;

	if((BOMTable[2].BOM[0] == BOM[0]) && (BOMTable[2].BOM[1] == BOM[1]))
		return BOMTable[2].type;

	if((BOMTable[3].BOM[0] == BOM[0]) && (BOMTable[3].BOM[1] == BOM[1]) && (BOMTable[3].BOM[2] == BOM[2]))
		return BOMTable[3].type;

	return BOMTable[0].type;
}

void Endian::exchange4(void * data)
{
	char *c = (char *)data;
	char a = c[0], b = c[1];

	c[0] = c[3];
	c[1] = c[2];
	c[2] = b;
	c[3] = a;

}

void Endian::exchange8(void * data)
{
	char *c = (char *)data;
	char a = c[0], b = c[1], x = c[2], y = c[3];

	c[0] = c[7];
	c[1] = c[6];
	c[2] = c[5];
	c[3] = c[4];
	c[4] = y;
	c[5] = x;
	c[6] = b;
	c[7] = a;
}

void Endian::exchange4(void * dest, void * src)
{
	char *d = (char *)dest;
	char *s = (char *)src;

	d[0] = s[3];
	d[1] = s[2];
	d[2] = s[1];
	d[3] = s[0];

}

void Endian::exchange8(void * dest, void * src)
{
	char *d = (char *)dest;
	char *s = (char *)src;

	d[0] = s[7];
	d[1] = s[6];
	d[2] = s[5];
	d[3] = s[4];
	d[4] = s[3];
	d[5] = s[2];
	d[6] = s[1];
	d[7] = s[0];
}

