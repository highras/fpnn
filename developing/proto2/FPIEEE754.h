#ifndef FPNN_IEEE_754_H
#define FPNN_IEEE_754_H

#include <math.h>
#include <stdint.h>

namespace fpnn
{
	uint32_t toIeee754(float value);
	uint64_t toIeee754(double value);
	float fromIeee754(uint32_t value);
	double fromIeee754(uint64_t value);

	inline bool isIeee754Nan(uint32_t value)
	{
		return ((value & 0x7FFFFFFF) > 0x7F800000);
	}
	inline bool isIeee754Nan(uint64_t value)
	{
		return ((value & 0x7FFFFFFFFFFFFFFF) > 0x7FF0000000000000);
	}
	inline bool isIeee754Infinity(uint32_t value)
	{
		return ((value & 0x7FFFFFFF) == 0x7F800000);
	}
	inline bool isIeee754Infinity(uint64_t value)
	{
		return ((value & 0x7FFFFFFFFFFFFFFF) == 0x7FF0000000000000);
	}

	inline bool isIeee754PositiveInfinity(uint32_t value)
	{
		return (value == 0x7F800000);
	}
	inline bool isIeee754PositiveInfinity(uint64_t value)
	{
		return (value == 0x7FF0000000000000);
	}
	inline bool isIeee754NegativeInfinity(uint32_t value)
	{
		return (value == 0xFF800000);
	}
	inline bool isIeee754NegativeInfinity(uint64_t value)
	{
		return (value == 0xFFF0000000000000);
	}
}

#endif