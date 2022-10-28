#include <string.h>
#include <limits>
#include "FPIEEE754.h"

const uint32_t IEEE745_PositiveInfinity32 = 0x7F800000;
const uint32_t IEEE745_NegativeInfinity32 = 0xFF800000;
const uint64_t IEEE745_PositiveInfinity64 = 0x7FF0000000000000;
const uint64_t IEEE745_NegativeInfinity64 = 0xFFF0000000000000;

uint32_t fpnn::toIeee754(float value)
{
	uint32_t rev = 0;

	//-- IEEE 754 check
	if (std::numeric_limits<float>::is_iec559)
	{
		memcpy(&rev, &value, 4);
		return rev;
	}

	//-- Zero check
	if (value == 0)
		return 0;

	//-- Infinity check
	if (isinf(value))
		return (value > 0) ? IEEE745_PositiveInfinity32 : IEEE745_NegativeInfinity32;

	//--- NaN check
	if (isnan(value))
		return IEEE745_PositiveInfinity32  | 0x1;

	//-- Normal float process
	bool positive = value > 0;
	if (!positive)
	{
		value = -value;
		rev |= 0x80000000;
	}

	uint8_t exp = 127;
	int fractionBits = 23;

	if (value > (uint32_t)0x7FFFFFFF)
	{
		int expBase = 30;
		exp += expBase;
		float all = value / 0x40000000;		//-- 0x40000000: 2^30

		while (all >= 1073741824.f)
		{
			exp += 30;
			all /= 1073741824.f;
		}

		while (all >= 1024.f)
		{
			exp += 10;
			all /= 1024.f;
		}

		while (all >= 2.f)
		{
			exp += 1;
			all /= 2.f;
		}

		//-- exponent bits
		rev |= (((uint32_t)exp) << fractionBits);

		float fraction = all - 1.f;

		for (; fractionBits > 0; fractionBits--)
		{
			fraction *= 2.f;
			if (fraction >= 1.f)
			{
				fraction -= 1.f;
				rev |= ((uint32_t)0x1 << (fractionBits - 1));
			}
			if (fraction == 0)
				break;
		}

		return rev;
	}

	int integer = (int)value;

	if (integer == -2147483648)
	{
		if (positive)
			return 0x4f000000;
		else
			return 0xcf000000;
	}

	if (integer > 0)
	{
		int step = 0;
		float fraction = value - integer;

		int tmp = integer >> 1;
		while (tmp > 1024)
		{
			step += 10;
			exp += 10;
			tmp >>= 10;
		}
		while (tmp > 0)
		{
			step += 1;
			exp += 1;
			tmp >>= 1;
		}

		//-- exponent bits
		rev |= (((uint32_t)exp) << fractionBits);

		if (step > fractionBits)
		{
			integer >>= (step - fractionBits);
			step = fractionBits;
		}

		//-- fraction bits: 1
		uint32_t mask = 0xFFFFFFFF;
		mask <<= step;
		mask = ~mask;
		
		fractionBits -= step;
		rev |= (((uint32_t)integer & mask) << fractionBits);

		//-- fraction bits: 2
		for (; fractionBits > 0; fractionBits--)
		{
			fraction *= 2.f;
			if (fraction >= 1.f)
			{
				fraction -= 1.f;
				rev |= ((uint32_t)0x1 << (fractionBits - 1));
			}
			if (fraction == 0)
				break;
		}
	}
	else
	{
		while (true)
		{
			value *= 2.f;
			exp -= 1;

			if (value >= 1.f)
			{
				if (exp > 0)
					value -= 1.f;
				else
					value /= 2.f;

				break;
			}

			if (exp == 0)
			{
				fractionBits -= 1;
				break;
			}
		}

		//-- exponent bits
		rev |= ((uint32_t)exp << fractionBits);

		//-- fraction bits
		for (; fractionBits > 0; fractionBits--)
		{
			value *= 2.f;
			if (value >= 1.f)
			{
				value -= 1.f;
				rev |= ((uint32_t)0x1 << (fractionBits - 1));
			}
			if (value == 0)
				break;
		}
	}

	return rev;
}

uint64_t fpnn::toIeee754(double value)
{
	uint64_t rev = 0;

	//-- IEEE 754 check
	if (std::numeric_limits<double>::is_iec559)
	{
		memcpy(&rev, &value, 8);
		return rev;
	}

	//-- Zero check
	if (value == 0)
		return 0;

	//-- Infinity check
	if (isinf(value))
		return (value > 0) ? IEEE745_PositiveInfinity64 : IEEE745_NegativeInfinity64;

	//-- NaN check
	if (isnan(value))
		return IEEE745_PositiveInfinity64 | 0x1;

	//-- Normal double process
	bool positive = value > 0;
	if (!positive)
	{
		value = -value;
		rev |= 0x8000000000000000;
	}

	uint16_t exp = 1023;
	int fractionBits = 52;

	if (value > (uint64_t)0x7FFFFFFFFFFFFFFF)
	{
		int expBase = 62;
		exp += expBase;
		double all = value / 0x4000000000000000;		//-- 0x4000000000000000: 2^62

		while (all >= 1152921504606846976.)
		{
			exp += 60;
			all /= 1152921504606846976.;
		}

		while (all >= 1073741824.)
		{
			exp += 30;
			all /= 1073741824.;
		}

		while (all >= 1024.)
		{
			exp += 10;
			all /= 1024.;
		}
		while (all >= 2.)
		{
			exp += 1;
			all /= 2.;
		}

		//-- exponent bits
		rev |= (((uint64_t)exp) << fractionBits);

		double fraction = all - 1.;

		for (; fractionBits > 0; fractionBits--)
		{
			fraction *= 2.;
			if (fraction >= 1.)
			{
				fraction -= 1.;
				rev |= ((uint64_t)0x1 << (fractionBits - 1));
			}
			if (fraction == 0)
				break;
		}

		return rev;
	}

	int64_t integer = (int64_t)value;
	if (integer > 0)
	{
		int step = 0;
		double fraction = value - integer;

		int64_t tmp = integer >> 1;
		while (tmp > 1024)
		{
			step += 10;
			exp += 10;
			tmp >>= 10;
		}
		while (tmp > 0)
		{
			step += 1;
			exp += 1;
			tmp >>= 1;
		}

		//-- exponent bits
		rev |= (((uint64_t)exp) << fractionBits);

		if (step > fractionBits)
		{
			integer >>= (step - fractionBits);
			step = fractionBits;
		}

		//-- fraction bits: 1
		uint64_t mask = 0xFFFFFFFFFFFFFFFF;
		mask <<= step;
		mask = ~mask;
		
		fractionBits -= step;
		rev |= (((uint64_t)integer & mask) << fractionBits);

		//-- fraction bits: 2
		for (; fractionBits > 0; fractionBits--)
		{
			fraction *= 2.;
			if (fraction >= 1.)
			{
				fraction -= 1.;
				rev |= ((uint64_t)0x1 << (fractionBits - 1));
			}
			if (fraction == 0)
				break;
		}
	}
	else
	{
		while (true)
		{
			value *= 2.;
			exp -= 1;

			if (value >= 1.)
			{
				if (exp > 0)
					value -= 1.;
				else
					value /= 2.;
				
				break;
			}

			if (exp == 0)
			{
				fractionBits -= 1;
				break;
			}
		}

		//-- exponent bits
		rev |= ((uint64_t)exp << fractionBits);

		//-- fraction bits
		for (; fractionBits > 0; fractionBits--)
		{
			value *= 2.;
			if (value >= 1.)
			{
				value -= 1.;
				rev |= ((uint64_t)0x1 << (fractionBits - 1));
			}
			if (value == 0)
				break;
		}
	}

	return rev;
}

float fpnn::fromIeee754(uint32_t value)
{
	float rev = 0;

	//-- IEEE 754 check
	if (std::numeric_limits<float>::is_iec559)
	{
		memcpy(&rev, &value, 4);
		return rev;
	}

	//-- Zero check
	if ((value & 0x7FFFFFFF) == 0)
		return (float)value;

	//-- Infinity & NaN check
	bool negative = value & 0x80000000;
	int fraction = (int)(value & 0x7FFFFF);

	if ((value & 0x7F800000) == 0x7F800000)
	{
		if (fraction)
			return NAN;
		else
			return negative ? -INFINITY : INFINITY;
	}

	//-- Normal float process
	int exp = (int)((value & 0x7FFFFFFF) >> 23) - 127;
	if (exp > -127)
		rev = 1.f;

	int fractionBits = 23;
	for (int i = 0; i < fractionBits; i++)
	{
		int32_t bit = ((int32_t)0x1 << i) & fraction;
		if (bit > 0)
			rev += 1.f / ((int32_t)1 << (fractionBits - i));
	}

	if (exp > 0)
	{
		while (exp > 30)
		{
			rev *= 0x40000000;
			exp -= 30;
		}

		if (exp > 0)
			rev *= (int32_t)1 << exp;

	}
	else if (exp > -127)
	{
		for (; exp < 0; exp++)
			rev /= 2;
	}
	else
	{
		for (int i = 0; i < 126; i++)
			rev /= 2;
	}

	return negative ? -rev : rev;
}

double fpnn::fromIeee754(uint64_t value)
{
	double rev = 0;

	//-- IEEE 754 check
	if (std::numeric_limits<double>::is_iec559)
	{
		memcpy(&rev, &value, 8);
		return rev;
	}

	//-- Zero check
	if ((value & 0x7FFFFFFFFFFFFFFF) == 0)
		return (double)value;

	//-- Infinity  & NaN check
	bool negative = value & 0x8000000000000000;
	int64_t fraction = (int64_t)(value & 0xFFFFFFFFFFFFF);

	if ((value & 0x7FF0000000000000) == 0x7FF0000000000000)
	{
		if (fraction)
			return NAN;
		else
			return negative ? -INFINITY : INFINITY;
	}

	//-- Normal double process
	int exp = (int)((value & 0x7FFFFFFFFFFFFFFF) >> 52) - 1023;
	if (exp > -1023)
		rev = 1.;

	int fractionBits = 52;
	for (int i = 0; i < fractionBits; i++)
	{
		int64_t bit = ((int64_t)0x1 << i) & fraction;
		if (bit > 0)
			rev += 1. / ((int64_t)1 << (fractionBits - i));
	}

	if (exp > 0)
	{
		while (exp > 62)
		{
			rev *= 0x4000000000000000;
			exp -= 62;
		}

		if (exp > 0)
			rev *= (int64_t)1 << exp;

	}
	else if (exp > -1023)
	{
		for (; exp < 0; exp++)
			rev /= 2;
	}
	else
	{
		for (int i = 0; i < 1022; i++)
			rev /= 2;
	}

	return negative ? -rev : rev;
}
