## IObjectPool

### 介绍

对象池接口定义。

### 命名空间

	namespace fpnn;

### IObjectPool

	template < typename K >
	class IObjectPool
	{
		public:
			virtual				~IObjectPool() {}

			virtual bool		init(int32_t initBlocks, int32_t perAppendBlocks, int32_t perfectBlocks, int32_t maxBlocks) = 0;
			virtual void		release() = 0;

			//-- Just for memory. NO constructor be called, and NO destructor be called.
			virtual K*			gainMemory() = 0;
			virtual void		recycleMemory(K**) = 0;
			virtual void		recycleMemory(K*&) = 0;

			virtual bool		inited() = 0;
			virtual void		status(struct MemoryPoolStatus&) = 0;

			//-- Just for Object. Both constructor and destructor will be called.
			template<typename... Params>
				K* gain(Params&&... values)
				{
					K* memoryBlock = gainMemory();
					if (memoryBlock)
						new(memoryBlock) K(std::forward<Params>(values)...);

					return memoryBlock;
				}
			void recycle(K** obj) { (*obj)->~K(); recycleMemory(obj); }
			void recycle(K*& obj) { obj->~K(); recycleMemory(obj); }
	};

FPNN 通用对象池接口定义。

#### init

	virtual bool init(int32_t initBlocks, int32_t perAppendBlocks, int32_t perfectBlocks, int32_t maxBlocks) = 0;

初始化对象池。

**参数说明**

* **`int32_t initBlocks`**

	初始化内存池时，预先初始化的对象内存块数量。

* **`int32_t perAppendBlocks`**

	追加准对象时，批量追加的对象内存块数量。

* **`int32_t perfectBlocks`**

	软限制：常驻对象池中的对象内存块数量。

* **`int32_t maxBlocks`**

	硬限制：对象池最大管理对象内存块数量。0 表示不限制。

#### release

	virtual void release() = 0;

释放对象池，及所有对象。

#### gainMemory

	virtual K* gainMemory() = 0;

获取对象内存块。

**注意**

+ 该接口仅只是获取对象所用内存，而非对象。且内存未初始化。

+ 一般情况下，请勿使用该接口，而使用 [gain](#gain) 替代。

#### recycleMemory

	virtual void recycleMemory(K**) = 0;
	virtual void recycleMemory(K*&) = 0;

回收对象内存块。

**注意**

+ 该接口仅只是回收对象内存块，如果存在对象，不会析构对象。

+ 一般情况下，请勿使用该接口，而使用 [recycle](#recycle) 替代。

#### inited

	virtual bool inited() = 0;

判断对象池是否初始化完毕。

#### status

	virtual void status(struct MemoryPoolStatus&) = 0;

获取对象池当前状态。`struct MemoryPoolStatus` 请参见 [MemoryPoolStatus](IMemoryPool.md#MemoryPoolStatus)。

#### gain

	template<typename... Params>
	K* gain(Params&&... values);

获取并初始化对象。可变参数为实例化该模版的具体对象的构造函数参数。

#### recycle

	void recycle(K** obj);
	void recycle(K*& obj);

回收并析构对象。