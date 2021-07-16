## hex

### 介绍

16 进制工具模块。

### 命名空间

无命名空间约束。

### 全局函数

#### hexlify

	int hexlify(char *dst, const void *src, int size);

将 `const void *src` 指向的内存中，`int size` 字节的二进制数据，转换成**小写**16进制文本，输出到 `char *dst` 中。  
返回值为输出的长度，不计入结尾的`\0`。

#### Hexlify

	int Hexlify(char *dst, const void *src, int size);

将 `const void *src` 指向的内存中，`int size` 字节的二进制数据，转换成**大写**16进制文本，输出到 `char *dst` 中。  
返回值为输出的长度，不计入结尾的`\0`。

#### unhexlify


	int unhexlify(void *dst, const char *src, int size);

将 `const void *src` 指向的 `int size` 字节长16进制文本，转化为二进制数据，并输出到 `void *dst` 所指向内存中。  
如果 `int size` 参数为 `-1`，则 `unhexlify()` 将调用 `stelen(src)` 计算实际长度。

返回值为实际写入到 `void *dst` 的字节数。  
如果发生错误，将返回负值。其绝对值为实际使用的 `const void *src` 的字节数。