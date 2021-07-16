## unixfs

### 介绍

C 版本 UNIX 文件读取和写入模块。

### 命名空间

无命名空间约束。

### 全局函数

#### unixfs_mkdir

	int unixfs_mkdir(const char *pathname, mode_t mode, uid_t owner, gid_t group);

递归创建目录。

**返回值**

+ 0: 创建成功
+ 1: 创建成功，但 `chown()` 调用失败。
+ -1: 创建失败。

#### unixfs_open

	int unixfs_open(const char *pathname, int flags, mode_t mode);

打开文件。

类似于 `open(2)`。区别：当 `flags` 包含 `O_CREAT` 标记时，如果文件路径上有目录不存在 `unixfs_open()` 将创建相应的目录。 


#### unixfs_get_content

	ssize_t unixfs_get_content(const char *pathname, char **p_content, size_t *n);

将文件路径指向的文件内容读取到 p_content 中，并返回内容长度。

**参数说明**

* **`const char *pathname`**

	要访问的文件路径。

* **`char **p_content`**

	要写入文件内容的内存。

	可以是通过 `malloc()` 分配的空间，或者是 `NULL`。

	如果分配的空间长度不够，函数会调用 `realloc()` 重新分配。


* **`size_t *n`**

	`*p_content` 参数指向内存的大小。

	如果文件尺寸大于 `*p_content` 参数已分配的大小，则函数会调用 `realloc()` 重新分配，并将 `*n` 设置为新分配空间的大小。

**返回值**

+ -1: 失败。
+ 非负数：文件内容字节数。


#### unixfs_put_content

	ssize_t unixfs_put_content(const char *pathname, const void *content, size_t size);

向指定文件路径写入内存数据。