## md5

### 介绍

md5 摘要算法。

### 命名空间

无命名空间约束。

### 全局函数

#### md5_context

	typedef struct {
		uint32_t total[2];
		uint32_t state[4];
		unsigned char buffer[64];
	} md5_context;

md5 上下文结构。

#### md5_start

	void md5_start(md5_context *ctx);

初始化 md5 上下文结构。

#### md5_update

	void md5_update(md5_context *ctx, const void *input, size_t length);

使用 `md5_context *ctx` 指定的上下文，继续计算 `const void *input` 指向的 `size_t length` 字节长的数据的 md5。

#### md5_finish

	void md5_finish(md5_context *ctx, unsigned char digest[16]);

md5 计算完成，获取 md5 数值。

#### md5_checksum

	void md5_checksum(unsigned char digest[16], const void *input, size_t length);

计算 `const void *input` 所指向 `size_t length` 字节长的数据的 md5 值，并在 `unsigned char digest[16]` 中返回。