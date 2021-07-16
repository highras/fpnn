## IMemoryPool

### 介绍

定长内存池接口定义。

### 命名空间

	namespace fpnn;

### MemoryPoolStatus

	struct MemoryPoolStatus
	{
		int32_t				usedCount;		//-- Not include the Overdraft nodes.
		int32_t				freeCount;
		int32_t				overdraftCount;
		int32_t				totalUsedCount;
		int32_t				totalCount;
	};

FPNN 通用定长内存池状态结构。

**成员说明**

* **`int32_t usedCount`**

	当前使用中的常驻内存池的内存块数量。**注意**：不包含临时分配的内存块数量。

* **`int32_t freeCount`**

	当前空闲的内存块数量。

* **`int32_t overdraftCount`**

	临时分配的内存块数量。

* **`int32_t totalUsedCount`**

	当前总的正在使用中的内存块数量。

* **`int32_t totalCount`**

	当前总的内存块数量。


### IMemoryPool

	class IMemoryPool
	{
	public:
		virtual				~IMemoryPool() {}

		virtual bool		init(size_t blockSize, int32_t initBlocks, int32_t perAppendBlocks, int32_t perfectBlocks, int32_t maxBlocks) = 0;
		virtual void		release() = 0;

		virtual void*		gain() = 0;
		virtual void		recycle(void**) = 0;
		virtual void		recycle(void*&) = 0;

		virtual bool		inited() = 0;
		virtual void		status(struct MemoryPoolStatus&) = 0;
	};

FPNN 通用定长内存池接口定义。

#### init

	virtual bool init(size_t blockSize, int32_t initBlocks, int32_t perAppendBlocks, int32_t perfectBlocks, int32_t maxBlocks) = 0;

初始化内存池。

**参数说明**

* **`size_t blockSize`**

	单个内存块容量大小。

* **`int32_t initBlocks`**

	初始化内存池时，预先初始化的内存块数量。

* **`int32_t perAppendBlocks`**

	追加内存块时，批量追加的数量。

* **`int32_t perfectBlocks`**

	软限制：常驻内存池中的内存块数量。

* **`int32_t maxBlocks`**

	硬限制：内存池最大管理内存块数量。0 表示不限制。


#### release

	virtual void release() = 0;

释放内存池，及所有内存块。

#### gain

	virtual void* gain() = 0;

获取一块内存。

#### recycle

	virtual void recycle(void**) = 0;
	virtual void recycle(void*&) = 0;

回收一块内存。

#### inited

	virtual bool inited() = 0;

判断内存池是否初始化完毕。

#### status

	virtual void status(struct MemoryPoolStatus&) = 0;

获取内存池当前状态。