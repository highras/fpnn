## PrintMemory

### 介绍

内存打印模块。

### 命名空间

	namespace fpnn;

### 全局函数

#### printMemory

	void printMemory(const void* memory, size_t size);

将 `const void* memory` 指向的长度为 `size_t size` 的内存数据，以16进制和文本对照的形式打印到标准输出。