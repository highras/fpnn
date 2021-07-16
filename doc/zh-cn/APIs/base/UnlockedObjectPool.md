## UnlockedObjectPool

### 介绍

对象池，**非**线程**安全**版本。需要调用者加锁。

## 命名空间

	namespace fpnn;

### 关键定义

	template < typename K >
	class UnlockedObjectPool: public IObjectPool<K>
	{
	public:
		UnlockedObjectPool();
		~UnlockedObjectPool();

		virtual bool init(int32_t initBlocks, int32_t perAppendBlocks, int32_t perfectBlocks, int32_t maxBlocks);
		virtual void release();

		virtual K* gainMemory();											//- Gain the memory block.
		virtual void recycleMemory(K** ppNode);							//- Recycle the memory block.
		virtual void recycleMemory(K* & pNode);							//- Recycle the memory block.

		virtual bool inited() { return _inited; }
		virtual void status(struct MemoryPoolStatus& mps);
	};

### 成员函数
 
所有成员函数，请参考 [IObjectPool](IObjectPool.md)。