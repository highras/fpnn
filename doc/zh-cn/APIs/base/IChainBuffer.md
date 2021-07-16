## IChainBuffer

### 介绍

链式缓存接口定义。

### 命名空间

	namespace fpnn;

### 关键定义

	class IChainBuffer
	{
	public:
		virtual ~IChainBuffer() {}
		
		virtual int length() const = 0;
		virtual void append(const void *data, int length) = 0;	//--  append into chainBuffer
		virtual int writeTo(void *buffer, int length, int offset) = 0;		//-- write to out buffer from chainBuffer
		virtual struct iovec* getIOVec(int &count) = 0;  //-- use free() to release.

		virtual int readfd(int fd, int length) = 0; 	//-- read fd and write into chainbuffer.
		virtual int writefd(int fd, int length, int offset) = 0;	//-- write to fd from chinaBuffer.

		virtual int fread(FILE* stream, int length) = 0; 	//-- read stream and write into chainbuffer.
		virtual int fwrite(FILE* stream, int length, int offset) = 0;	//-- write to stream from chinaBuffer.

		//-- New interface
		virtual int chunkCount() = 0;
		virtual int chunkSize() = 0;
		virtual void* chunkBuf(int index, int& data_length) = 0;
		virtual void* header(int require_length) = 0;

		virtual int find(const char c, int start_pos = 0, int* chunckIndex = NULL, int* chunkBufferOffset = NULL) = 0;
		virtual bool memcmp(const void* target, int len, int start_pos = 0) = 0;

		virtual int getLines(std::vector<std::string>& lines, int offset = 0) = 0;
	};
	typedef std::shared_ptr<IChainBuffer> IChainBufferPtr;

### 成员函数

#### length
	
	virtual int length() const = 0;

获取当前数据长度。

#### append

	virtual void append(const void *data, int length) = 0;

将数据以拷贝的方式，追加到链式缓存中。

**参数说明**

* **`const void *data`**

	要追加的数据内存地址。

* **`int length`**

	要追加的数据长度。

#### writeTo

	virtual int writeTo(void *buffer, int length, int offset) = 0;

向指定的外部内存地址，写入数据。

**参数说明**

* **`void *buffer`**

	要写入的内存地址。

* **`int length`**

	写入数据的长度。

* **`int offset`**

	IChainBuffer 实现实例中，内部缓存的偏移字节数。

#### getIOVec

	virtual struct iovec* getIOVec(int &count) = 0;

将缓存链以 iovec 的形式返回。  
返回的 iovec 需要调用 `free()` 进行释放。

**参数说明**

* **`int &count`**

	返回 iovec 数组的长度（iovec 的数量）。

#### readfd

	virtual int readfd(int fd, int length) = 0;

从 fd 读取指定长度的数据到链式缓存中。

#### writefd

	virtual int writefd(int fd, int length, int offset) = 0;

向 fd 写入指定长度的数据。

**参数说明**

* **`int fd`**

	要写入的文件描述符。

* **`int length`**

	写入数据的长度。

* **`int offset`**

	IChainBuffer 实现实例中，内部缓存的偏移字节数。

#### fread

	virtual int fread(FILE* stream, int length) = 0;

从 stream 文件流读取指定长度的数据到链式缓存中。

#### fwrite

	virtual int fwrite(FILE* stream, int length, int offset) = 0;

向 stream 文件流写入指定长度的数据。

**参数说明**

* **`FILE* stream`**

	要写入的文件流。

* **`int length`**

	写入数据的长度。

* **`int offset`**

	IChainBuffer 实现实例中，内部缓存的偏移字节数。

#### chunkCount
	
	virtual int chunkCount() = 0;

返回链式缓存内部的节点数量。

#### chunkSize

	virtual int chunkSize() = 0;

返回链式缓存单个节点的缓存大小。

#### chunkBuf

	virtual void* chunkBuf(int index, int& data_length) = 0;

返回链式缓存指定节点的缓存起始地址。

**参数说明**

* **`int index`**

	节点的索引。从 0 开始。

* **`int& data_length`**

	返回指定节点缓存的数据长度。

#### header

**特殊用途：FPNN Framework 内部使用**

	virtual void* header(int require_length) = 0;

当已缓存的数据大于等于 require_length 指定的长度时，返回缓存头节点的数据起始地址。否则返回 NULL。

#### find

	virtual int find(const char c, int start_pos = 0, int* chunckIndex = NULL, int* chunkBufferOffset = NULL) = 0;

在缓存中查找指定字符第一次出现的位置。

**参数说明**

* **`const char c`**

	要查找的字符。

* **`int start_pos`**

	查找位置的起始偏移。

* **`int* chunckIndex`**

	如果发现指定字符，则返回指定字符所在的缓存块的索引。索引从 0 开始。

* **`int* chunkBufferOffset`**

	如果发现指定字符，则返回指定字符在所在的缓存块内的偏移。

**返回值**

如果发现指定字符，返回从缓存头到指定字符所在位置的总偏移。否则返回 `-1`。

#### memcmp

	virtual bool memcmp(const void* target, int len, int start_pos = 0) = 0;

内存比较。

**参数说明**

* **`const void* target`**

	需要比较的内存的起始地址。

* **`int len`**

	需要比较的数据的长度。

* **`int start_pos`**

	比较的起始偏移。

**返回值**

相等返回 true，否则返回 false 。

#### getLines

	virtual int getLines(std::vector<std::string>& lines, int offset = 0) = 0;

对于文本数据，按行切割，并返回。

**注意**：是否忽略空行，取决于具体实现。

**参数说明**

* **`std::vector<std::string>& lines`**

	返回的行数据。

* **`int offset`**

	缓存的起始偏移。

**返回值**

返回的行数。
