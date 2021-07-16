## uuid

### 介绍

生成 UUID。

### 命名空间

无命名空间约束。

### 全局函数

#### uuid_t

	typedef unsigned char uuid_t[16];

uuid 二进制结构。

#### uuid_generate

	void uuid_generate(uuid_t uuid);

生成 uuid。


#### uuid_generate_random

	void uuid_generate_random(uuid_t uuid);

用随机数生成 uuid。


#### uuid_generate_time

	void uuid_generate_time(uuid_t uuid);

用时间生成 uuid。


#### uuid_string

	size_t uuid_string(const uuid_t uuid, char buf[], size_t len);

将二进制的 uuid 转换成标准的字符串形式。


#### uuid_get_random_bytes

	void uuid_get_random_bytes(void *buf, int nbytes);

生成 nbytes 字节长度的随机串到 buf 中。