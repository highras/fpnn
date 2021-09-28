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

#ifdef __APPLE__
static bool isLittleEndian()
{
	const int32_t i = 0x12345678;
	const char *c = (const char *)&i;

	return (c[0] == 0x78);
}

static uint16_t exchange2(uint16_t value)
{
	unsigned char *c = (unsigned char *)&value;

	c[0] ^= c[1];
	c[1] ^= c[0];
	c[0] ^= c[1];

	return value;
}

static uint32_t exchange4(uint32_t value)
{
	unsigned char *c = (unsigned char *)&value;
	unsigned char a = c[0], b = c[1];

	c[0] = c[3];
	c[1] = c[2];
	c[2] = b;
	c[3] = a;

	return value;
}

static uint64_t exchange8(uint64_t value)
{
	unsigned char *c = (unsigned char *)&value;
	unsigned char a = c[0], b = c[1], x = c[2], y = c[3];

	c[0] = c[7];
	c[1] = c[6];
	c[2] = c[5];
	c[3] = c[4];
	c[4] = y;
	c[5] = x;
	c[6] = b;
	c[7] = a;

	return value;
}

static bool gc_hostIsLittleEndian = isLittleEndian();

uint16_t Endian::htobe16(uint16_t host_16bits)
{
	if (gc_hostIsLittleEndian)
		return ::exchange2(host_16bits);
	else
		return host_16bits;
}
uint16_t Endian::htole16(uint16_t host_16bits)
{
	if (gc_hostIsLittleEndian)
		return host_16bits;
	else
		return ::exchange2(host_16bits);
}
uint16_t Endian::be16toh(uint16_t big_endian_16bits)
{
	if (gc_hostIsLittleEndian)
		return ::exchange2(big_endian_16bits);
	else
		return big_endian_16bits;
}
uint16_t Endian::le16toh(uint16_t little_endian_16bits)
{
	if (gc_hostIsLittleEndian)
		return little_endian_16bits;
	else
		return ::exchange2(little_endian_16bits);
}


uint32_t Endian::htobe32(uint32_t host_32bits)
{
	if (gc_hostIsLittleEndian)
		return ::exchange4(host_32bits);
	else
		return host_32bits;
}
uint32_t Endian::htole32(uint32_t host_32bits)
{
	if (gc_hostIsLittleEndian)
		return host_32bits;
	else
		return ::exchange4(host_32bits);
}
uint32_t Endian::be32toh(uint32_t big_endian_32bits)
{
	if (gc_hostIsLittleEndian)
		return ::exchange4(big_endian_32bits);
	else
		return big_endian_32bits;
}
uint32_t Endian::le32toh(uint32_t little_endian_32bits)
{
	if (gc_hostIsLittleEndian)
		return little_endian_32bits;
	else
		return ::exchange4(little_endian_32bits);
}


uint64_t Endian::htobe64(uint64_t host_64bits)
{
	if (gc_hostIsLittleEndian)
		return ::exchange8(host_64bits);
	else
		return host_64bits;
}
uint64_t Endian::htole64(uint64_t host_64bits)
{
	if (gc_hostIsLittleEndian)
		return host_64bits;
	else
		return ::exchange8(host_64bits);
}
uint64_t Endian::be64toh(uint64_t big_endian_64bits)
{
	if (gc_hostIsLittleEndian)
		return ::exchange8(big_endian_64bits);
	else
		return big_endian_64bits;
}
uint64_t Endian::le64toh(uint64_t little_endian_64bits)
{
	if (gc_hostIsLittleEndian)
		return little_endian_64bits;
	else
		return ::exchange8(little_endian_64bits);
}
#endif

