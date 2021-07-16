## crc

### 介绍

CRC 校验码计算。

### 命名空间

无命名空间约束。

### 全局函数

#### 对于多片段数据，连续计算 CRC 校验码

	uint16_t crc16_update(uint16_t old_crc, const void *buf, size_t size);
	uint16_t crc16_update_cstr(uint16_t old_crc, const char *str);

	uint32_t crc32_update(uint32_t old_crc, const void *buf, size_t size);
	uint32_t crc32_update_cstr(uint32_t old_crc, const char *str);

	uint64_t crc64_update(uint64_t oldcrc, const void *buf, size_t size);
	uint64_t crc64_update_cstr(uint64_t oldcrc, const char *str);

**参数说明**

* `old_crc`

	上一个片段的 CRC 校验码。  
	对于第一个片段，设置为 0。

* `const void *buf`

	要计算校验码的内存起始地址。

* `size_t size`

	要计算校验码的数据长度。

* `const char *str`

	要计算校验码的 c 风格字符串指针。

**返回值**

CRC 校验值。


#### 对于整块数据，一次性计算 CRC 校验码

	uint16_t crc16_checksum(const void *buf, size_t size);
	uint16_t crc16_checksum_cstr(const char *str);

	uint32_t crc32_checksum(const void *buf, size_t size);
	uint32_t crc32_checksum_cstr(const char *str);

	uint64_t crc64_checksum(const void *buf, size_t size);
	uint64_t crc64_checksum_cstr(const char *str);

**参数说明**

* `const void *buf`

	要计算校验码的内存起始地址。

* `size_t size`

	要计算校验码的数据长度。

* `const char *str`

	要计算校验码的 c 风格字符串指针。

**返回值**

CRC 校验值。

