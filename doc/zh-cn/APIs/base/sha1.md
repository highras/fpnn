## sha1

### 介绍

SHA-1 摘要算法。

### 命名空间

无命名空间约束。

### 全局函数

#### sha1_context

	typedef struct {
		uint32_t total[2];
		uint32_t state[5];
		unsigned char buffer[64];
	} sha1_context;

sha-1 上下文结构。

#### sha1_start

	void sha1_start(sha1_context *ctx);

初始化 sha-1 上下文结构。

#### sha1_update

	void sha1_update(sha1_context *ctx, const void *input, size_t length);

使用 `sha1_context *ctx` 指定的上下文，继续计算 `const void *input` 指向的 `size_t length` 字节长的数据的 sha-1 值。

#### sha1_finish

	void sha1_finish(sha1_context *ctx, unsigned char digest[20]);

sha-1 计算完成，获取 sha-1 数值。

#### sha1_checksum

	void sha1_checksum(unsigned char digest[20], const void *input, size_t length);

计算 `const void *input` 所指向 `size_t length` 字节长的数据的 sha-1 值，并在 `unsigned char digest[20]` 中返回。