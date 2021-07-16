## Jenkins

### 介绍

Jenkins hash 算法。

**注意**：不常用函数并未在本文档中列出。如有需要，请直接参阅 [jenkins.h](../../../../base/jenkins.h)。

### 命名空间

无命名空间约束。

### 全局函数

#### jenkins_hash

	uint32_t jenkins_hash(const void *key, size_t length, uint32_t initval);

**参数说明**

* **`const void *key`**

	需要 hash 的对象的内存地址。

* **`size_t length`**

	需要 hash 的内存的字节长度。

* **`uint32_t initval`**

	hash 的初始值/上轮 hash 的值。

#### jenkins_hash64

	uint64_t jenkins_hash64(const void *key, size_t length, uint64_t initval);

**参数说明**

* **`const void *key`**

	需要 hash 的对象的内存地址。

* **`size_t length`**

	需要 hash 的内存的字节长度。

* **`uint64_t initval`**

	hash 的初始值/上轮 hash 的值。
