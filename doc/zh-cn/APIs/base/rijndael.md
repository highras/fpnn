## rijndael

### 介绍

AES 加密算法。  
rijndael 为该算法被确定为 AES 加密算法前的名字。

### 命名空间

无命名空间约束。

### 全局函数

#### rijndael_context

	typedef struct {
		int nrounds;
		uint32_t rk[60];
	} rijndael_context;

rijndael 上下文结构。

#### rijndael_setup_encrypt

	bool rijndael_setup_encrypt(rijndael_context *ctx, const uint8_t *key, size_t keylen);

初始化加密上下文。

**注意**

`keylen` 必须为 16、24、32 之一。

#### rijndael_setup_decrypt

	bool rijndael_setup_decrypt(rijndael_context *ctx, const uint8_t *key, size_t keylen);

初始化解密上下文。

**注意**

	`keylen` 必须为 16、24、32 之一。

#### rijndael_encrypt

	void rijndael_encrypt(const rijndael_context *ctx, const uint8_t plain[16], uint8_t cipher[16]);

AES/rijndael 加密函数。`const uint8_t plain[16]` 和 `uint8_t cipher[16]` 可以指向同一内存区域。

#### rijndael_decrypt

	void rijndael_decrypt(const rijndael_context *ctx, const uint8_t cipher[16], uint8_t plain[16]);

AES/rijndael 解密函数。`const uint8_t cipher[16]` 和 `uint8_t plain[16]` 可以指向同一内存区域。

#### rijndael_cbc_encrypt

	void rijndael_cbc_encrypt(const rijndael_context *ctx, const uint8_t *plain, uint8_t *cipher, size_t len, uint8_t ivec[16]);

AES/rijndael 加密函数，采用 CBC 密码分组模式。

**注意**

`len` 为 `uint8_t *plain` 指向数据的字节数。如果 `len` 不为 16 的倍数，那 `int8_t *cipher` 指向的输出数据，将会以 `\0` 进行填充，使输出总字节数为 16 的倍数。

#### rijndael_cbc_decrypt

	void rijndael_cbc_decrypt(const rijndael_context *ctx, const uint8_t *cipher, uint8_t *plain, size_t len, uint8_t ivec[16]);

AES/rijndael 解密函数，采用 CBC 密码分组模式。

**注意**

`len` 为 `uint8_t *plain` 指向数据的字节数。如果 `len` 不为 16 的倍数，那多余的数据(加密时填充的数据)将被舍弃。

#### rijndael_cfb_encrypt

	void rijndael_cfb_encrypt(const rijndael_context *ctx, bool encrypt, const uint8_t *in, uint8_t *out, size_t len, uint8_t ivec[16], size_t *p_num);

AES/rijndael 加解密函数，采用 CFB 密文反馈模式。

`size_t *p_num` 指向上次加解密的位置偏移。

**注意**

+ 不论加密或者解密，`const rijndael_context *ctx` 都的使用 `rijndael_setup_encrypt()` 初始化。

+ 参数 `bool encrypt` 决定执行加密，还是解密。

#### rijndael_ofb_encrypt

	void rijndael_ofb_encrypt(const rijndael_context *ctx, const uint8_t *in, uint8_t *out, size_t len, uint8_t ivec[16], size_t *p_num);

AES/rijndael 加解密函数，采用 OFB 输出反馈模式。

`size_t *p_num` 指向上次加解密的位置偏移。

**注意**

+ 不论加密或者解密，`const rijndael_context *ctx` 都的使用 `rijndael_setup_encrypt()` 初始化。

+ 加解密均使用同一函数。