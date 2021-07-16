## CachedChainBuffer

### 介绍

内建内存池和对象池的链式缓存实现。

### 命名空间

	namespace fpnn;

### 关键定义

	class CachedChainBuffer
	{
	public:
		class ChainBuffer: public IChainBuffer
		{
		public:
			ChainBuffer(int capacity, MemoryPool* memoryPool);
			virtual ~ChainBuffer();
			
			virtual int length() const { return _length; }
			virtual void append(const void *data, int length);	//--  append into chainBuffer
			virtual int writeTo(void *buffer, int length, int offset);	//-- write to out buffer from chainBuffer
			virtual struct iovec* getIOVec(int &count);  //-- use free() to release.

			virtual int readfd(int fd, int length); 	//-- read fd and write into chainbuffer.
			virtual int writefd(int fd, int length, int offset);	//-- write to fd from chinaBuffer.

			virtual int fread(FILE* stream, int length); 	//-- read stream and write into chainbuffer.
			virtual int fwrite(FILE* stream, int length, int offset);	//-- write to stream from chinaBuffer.

			//-- New interface
			virtual int chunkCount() { return _blockCount; }
			virtual int chunkSize() { return _capacity; }
			virtual void* chunkBuf(int index, int& data_length);
			virtual void* header(int require_length);

			virtual int find(const char c, int start_pos = 0, int* chunckIndex = NULL, int* chunkBufferOffset = NULL);
			virtual bool memcmp(const void* target, int len, int start_pos = 0);

			virtual int getLines(std::vector<std::string>& lines, int offset = 0);
		};
		
	public:
		CachedChainBuffer(int capacity = 512);
		~CachedChainBuffer();
		
		inline bool initMemoryPool(int32_t initBlocks, int32_t perAppendBlocks, int32_t perfectBlocks, int32_t maxBlocks);
		inline bool initObjectPool(int32_t initBlocks, int32_t perAppendBlocks, int32_t perfectBlocks, int32_t maxBlocks);
		
		inline ChainBuffer* createChainBuffer();
		inline void recycle(ChainBuffer*& chainBuffer);
		
		inline void release();
	};

### 内部类

#### ChainBuffer

[IChainBuffer](IChainBuffer.md) 的具体实现。  
所有成员函数，请参考 [IChainBuffer](IChainBuffer.md)。

### 构造函数

	CachedChainBuffer(int capacity = 512);

**参数说明**

* **`int capacity`**

	单块缓存的缓存大小。

### 成员函数

#### initMemoryPool

	inline bool initMemoryPool(int32_t initBlocks, int32_t perAppendBlocks, int32_t perfectBlocks, int32_t maxBlocks);

初始化内部的内存池（用于 ChainBuffer 缓存块）。

**参数说明**

* **`int32_t initBlocks`**

	初始化时分配的内存块数量。

* **`int perAppendBlocks`**

	每次追加时，批量追加的内存块数量。

* **`int perfectBlocks`**

	软上限：当内存块有空闲时，内存池最多管理的内存块数量。

* **`int maxBlocks`**

	硬上限：内存池最多管理的内存块数量。0 表示不限制。

#### initObjectPool

	inline bool initObjectPool(int32_t initBlocks, int32_t perAppendBlocks, int32_t perfectBlocks, int32_t maxBlocks);

初始化内部的对象池（用于 ChainBuffer 对象本身）。

**参数说明**

* **`int32_t initBlocks`**

	初始化时预初始化的对象数量。

* **`int perAppendBlocks`**

	每次追加时，批量追加的对象数量。

* **`int perfectBlocks`**

	软上限：当对象池对象有空闲时，内对象池最多管理的对象数量。

* **`int maxBlocks`**

	硬上限：对象池最多管理的对象数量。0 表示不限制。

#### createChainBuffer

	inline ChainBuffer* createChainBuffer();

获取 ChainBuffer 对象实例。

#### recycle

	inline void recycle(ChainBuffer*& chainBuffer);

回收分配的特定的 ChainBuffer 实例。

#### release

	inline void release();

释放整个 CachedChainBuffer 实例，及其分配的所有 ChainBuffer 实例，和内部的对象池和内存池。
