## strhash

### 介绍

字符串 & 内存 哈希工具。

### 命名空间

无命名空间约束。

### 全局函数

#### strhash

	unsigned int strhash(const char *str, unsigned int initval);

字符串哈希函数。

**参数说明**

* **`const char *str`**

	需要哈希的字符串。

* **`unsigned int initval`**

	连续计算哈希时，上次计算的哈希值。第一次用 0 代替。

#### memhash

	unsigned int memhash(const void *mem, size_t n, unsigned int initval);

内存哈希函数。

**参数说明**

* **`const void *mem`**

	需要哈希的内存的地址。

* **`size_t n`**

	需要计算哈希的内存的长度。

* **`unsigned int initval`**

	连续计算哈希时，上次计算的哈希值。第一次用 0 代替。