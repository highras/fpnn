## base64

### 介绍

Base64 编码 & 解码。

### 命名空间

无命名空间约束。

### 全局函数

#### base64_t

	typedef struct base64_t base64_t;

Base64 编解码上下文结构。

#### flag 枚举

	enum {
		BASE64_NO_PADDING = 0x01,
		BASE64_AUTO_NEWLINE = 0x02,
		BASE64_IGNORE_SPACE = 0x10,
		BASE64_IGNORE_NON_ALPHABET = 0x20,
	};

#### base64_init

	int base64_init(base64_t *b64, const char *alphabet)

初始化 `base64_t *b64` 对象。

**参数说明**

* `base64_t *b64`

	Base64 编解码上下文结构。

* `const char *alphabet`

	使用**内置**常量 `std_base64.alphabet` 或者 `url_base64.alphabet` 。

**返回值**

0 表示成功，-1 表示失败。

#### base64_encode

	ssize_t base64_encode(const base64_t *b64, char *out, const void *in, size_t len, int flag)

使用 base64 对 `in` 指向的内存块编码，并输出到 `out` 指向的内存块。

**参数说明**

* `const base64_t *b64`

	使用 `base64_init()` 初始化的 base64_t 对象。

* `char *out`

	输出内存。

* `const void *in`

	输入内存。

* `size_t len`

	输入字节长度。

* `int flag`

	使用前面列出的 flag 枚举参数。

**返回值**

实际输出的字节数。

#### base64_decode

	ssize_t base64_decode(const base64_t *b64, void *out, const char *in, size_t len, int flag)

使用 base64 对 `in` 指向的内存块解码，并输出到 `out` 指向的内存块。

**参数说明**

* `const base64_t *b64`

	使用 `base64_init()` 初始化的 base64_t 对象。

* `void *out`

	输出内存。

* `const char *in`

	输入内存。

* `size_t len`

	输入字节长度。

* `int flag`

	使用前面列出的 flag 枚举参数。

**返回值**

实际输出的字节数。  
如果发生错误，返回负值。其绝对值是实际需要的输出内存大小。


