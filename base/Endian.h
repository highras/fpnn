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

			if ((uint8_t)c[0] == 0xe4)
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

#ifdef __APPLE__
	public:
		static uint16_t htobe16(uint16_t host_16bits);
		static uint16_t htole16(uint16_t host_16bits);
		static uint16_t be16toh(uint16_t big_endian_16bits);
		static uint16_t le16toh(uint16_t little_endian_16bits);

		static uint32_t htobe32(uint32_t host_32bits);
		static uint32_t htole32(uint32_t host_32bits);
		static uint32_t be32toh(uint32_t big_endian_32bits);
		static uint32_t le32toh(uint32_t little_endian_32bits);

		static uint64_t htobe64(uint64_t host_64bits);
		static uint64_t htole64(uint64_t host_64bits);
		static uint64_t be64toh(uint64_t big_endian_64bits);
		static uint64_t le64toh(uint64_t little_endian_64bits);
#endif
};
}

#ifdef __APPLE__
inline uint16_t htobe16(uint16_t host_16bits) { return fpnn::Endian::htobe16(host_16bits); }
inline uint16_t htole16(uint16_t host_16bits) { return fpnn::Endian::htole16(host_16bits); }
inline uint16_t be16toh(uint16_t big_endian_16bits) { return fpnn::Endian::be16toh(big_endian_16bits); }
inline uint16_t le16toh(uint16_t little_endian_16bits) { return fpnn::Endian::le16toh(little_endian_16bits); }

inline uint32_t htobe32(uint32_t host_32bits) { return fpnn::Endian::htobe32(host_32bits); }
inline uint32_t htole32(uint32_t host_32bits) { return fpnn::Endian::htole32(host_32bits); }
inline uint32_t be32toh(uint32_t big_endian_32bits) { return fpnn::Endian::be32toh(big_endian_32bits); }
inline uint32_t le32toh(uint32_t little_endian_32bits) { return fpnn::Endian::le32toh(little_endian_32bits); }

inline uint64_t htobe64(uint64_t host_64bits) { return fpnn::Endian::htobe64(host_64bits); }
inline uint64_t htole64(uint64_t host_64bits) { return fpnn::Endian::htole64(host_64bits); }
inline uint64_t be64toh(uint64_t big_endian_64bits) { return fpnn::Endian::be64toh(big_endian_64bits); }
inline uint64_t le64toh(uint64_t little_endian_64bits) { return fpnn::Endian::le64toh(little_endian_64bits); }
#endif

#endif
