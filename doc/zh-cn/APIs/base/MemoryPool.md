## MemoryPool

### 介绍

内存池，线程安全版本。

### 命名空间

	namespace fpnn;

### 关键定义

	class MemoryPool: public IMemoryPool
	{
	public:
		MemoryPool();
		~MemoryPool();

		virtual bool init(size_t blockSize, int32_t initBlocks, int32_t perAppendBlocks, int32_t perfectBlocks, int32_t maxBlocks);
		virtual void release();

		virtual void* gain();
		virtual void recycle(void** memory);
		virtual void recycle(void* & memory);

		virtual bool inited();
		virtual void status(struct MemoryPoolStatus& mps);
	};

### 成员函数
 
所有成员函数，请参考 [IMemoryPool](IMemoryPool.md)。