## Endian

### 介绍

字节序 & BOM 处理模块。

### 命名空间

	namespace fpnn;

### BOMTypeKind

BOM 类型枚举。

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
	};

### EndianType

字节序类型枚举。

	class Endian
	{
	public:
		enum EndianType
		{
			LittleEndian = 0,
			BigEndian,
			MixedEndian
		};
	};

### Endian

	class Endian
	{
	public:

		//------ Endian judge ---------
		static inline bool isBigEndian();
		static inline bool isLittleEndian();

		static inline enum EndianType endian();

		/** Both ARM mixed-endian to little-endian and little-endian to ARM mixed-endian. */
		static inline double exchangeARMMixedEndianWithLittleEndian(double d);

		//------ BOM ---------
		static inline const unsigned char * BOMHeader(BOMTypeKind type);
		static inline int BOMHeaderLen(BOMTypeKind type);
		static inline const BOMTypeKind BOMType(const char * BOM);

		//------ Exchange between Big Endian and Little Endian ---------
		static inline void exchange2(void * data);
		static void exchange4(void * data);
		static void exchange8(void * data);

		static inline void exchange2(void * dest, void * src);
		static void exchange4(void * dest, void * src);
		static void exchange8(void * dest, void * src);
	};

#### 成员函数

#### isBigEndian

	static inline bool isBigEndian();

判断当前设备是否是大端设备。

#### isLittleEndian

	static inline bool isLittleEndian();

判断当前设备是否是小端设备。

#### endian

	static inline enum EndianType endian();

返回当前设备的字节序类型。

#### exchangeARMMixedEndianWithLittleEndian

	static inline double exchangeARMMixedEndianWithLittleEndian(double d);

在 ARM mixedEndian 字节序和小端字节序之间转换。

#### BOMHeader

	static inline const unsigned char * BOMHeader(BOMTypeKind type);

返回指定类型的 BOM 数据。

#### BOMHeaderLen

	static inline int BOMHeaderLen(BOMTypeKind type);

返回指定类型的 BOM 标识长度。

#### BOMType

	static inline const BOMTypeKind BOMType(const char * BOM);

检测 BOM 类型。

#### exchange2

	static inline void exchange2(void * data);
	static inline void exchange2(void * dest, void * src);

在大小端字节序之间转换（2 字节数据）。

#### exchange4

	static void exchange4(void * data);
	static void exchange4(void * dest, void * src);

在大小端字节序之间转换（4 字节数据）。

#### exchange8

	static void exchange8(void * data);
	static void exchange8(void * dest, void * src);

在大小端字节序之间转换（8 字节数据）。
