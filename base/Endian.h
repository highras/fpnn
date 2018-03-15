#ifndef FPNN_Endian_H
#define FPNN_Endian_H

#include <stdint.h>
#include <string.h>

namespace fpnn {

class Endian
{
	public:
		enum BOMTypeKind
		{
			UNKNOWN = 0,
			UCS2B = 1,		//-- UCS2 big endian
			UCS2L,			//-- UCS2 little endian
			UTF8			//-- UTF8
		};

		enum EndianType
		{
			LittleEndian = 0,
			BigEndian,
			MixedEndian
		};

	private:
		struct BOMItem
		{
			BOMTypeKind		type;
			int				len;
			unsigned char	BOM[4];
		};

		static const BOMItem	BOMTable[4];

	public:

		//------ Endian judge ---------
		static inline bool isBigEndian()
		{
			int32_t i = 0x12345678;
			char *c = (char *)&i;

			return (c[0] == 0x12);
		}

		static inline bool isLittleEndian()
		{
			int32_t i = 0x12345678;
			char *c = (char *)&i;

			return (c[0] == 0x78);
		}

		static inline enum EndianType endian()
		{
			double d = 1.982031;
			char *c = (char *)&d;

			if (c[0] == 0xe4)
				return LittleEndian;
			if (c[0] == 0x3f)
				return BigEndian;

			return MixedEndian;
		}

		/** Both ARM mixed-endian to little-endian and little-endian to ARM mixed-endian. */
		static inline double exchangeARMMixedEndianWithLittleEndian(double d)
		{
			double res;
			uint8_t *in = (uint8_t *)&d;
			uint8_t *out = (uint8_t *)&res;

			if (sizeof(d) == 8)
			{
				out[0] = in[4];
				out[1] = in[5];
				out[2] = in[6];
				out[3] = in[7];

				out[4] = in[0];
				out[5] = in[1];
				out[6] = in[2];
				out[7] = in[3];
			}
			else
			{
				out[0] = in[2];
				out[1] = in[3];
				out[2] = in[0];
				out[3] = in[1];
			}

			return res;
		}

		//------ BOM ---------
		static inline const unsigned char * BOMHeader(BOMTypeKind type)
		{
			return BOMTable[type].BOM;
		}

		static inline int BOMHeaderLen(BOMTypeKind type)
		{
			return BOMTable[type].len;
		}

		static inline const BOMTypeKind BOMType(const char * BOM);

		//------ Exchange between Big Endian and Little Endian ---------
		static inline void exchange2(void * data)
		{
			char *c = (char *)data;

			c[0] ^= c[1];
			c[1] ^= c[0];
			c[0] ^= c[1];
		}

		static void exchange4(void * data);
		static void exchange8(void * data);

		static inline void exchange2(void * dest, void * src)
		{
			char *d = (char *)dest;
			char *s = (char *)src;

			d[0] = s[1];
			d[1] = s[0];
		}

		static void exchange4(void * dest, void * src);
		static void exchange8(void * dest, void * src);
};
}

#endif
