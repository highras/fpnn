## TaskThreadPool

### 介绍

任务线程池。

### 命名空间

	namespace fpnn;

### 关键定义

	class TaskThreadPool: public ITaskThreadPool
	{
	public:
		virtual bool			init(int32_t initCount, int32_t perAppendCount, int32_t perfectCount, int32_t maxCount, size_t maxQueueLength = 0, size_t tempThreadLatencySeconds = 60);
		virtual bool			wakeUp(ITaskPtr task);
		virtual bool			wakeUp(std::function<void ()> task);
		#if(0)
		template <class Fn, class... Args>
		virtual bool			wakeUp(Fn&& func, Args&&... args);
		#endif
		virtual void			release();
		virtual void			status(int32_t &normalThreadCount, int32_t &temporaryThreadCount, int32_t &busyThreadCount, int32_t &taskQueueSize, int32_t& min, int32_t& max, int32_t& maxQueue);
		virtual std::string		infos();

		virtual bool inited();
		virtual bool exiting();

		TaskThreadPool();
		virtual ~TaskThreadPool();
	};
	typedef std::shared_ptr<TaskThreadPool> TaskThreadPoolPtr;

### 成员函数
 
所有成员函数，除下面额外说明的，其余请参考 [ITaskThreadPool](ITaskThreadPool.md)。

#### init

	virtual bool init(int32_t initCount, int32_t perAppendCount, int32_t perfectCount, int32_t maxCount, size_t maxQueueLength = 0, size_t tempThreadLatencySeconds = 60);

任务线程池初始化函数。

**参数说明**

+ **`int32_t initCount`**

	初始的线程数量。

+ **`int32_t perAppendCount`**

	追加线程时，单次的追加线程数量。

	**注意**，当线程数超过 `perfectCount` 的限制后，单次追加数量仅为 `1`。

+ **`int32_t perfectCount`**

	线程池常驻线程的最大数量。

+ **`int32_t maxCount`**

	线程池线程最大数量。如果为 `0`，则表示不限制。

+ **`size_t maxQueueLength`**

	线程池任务队列最大数量限制。如果为 `0`，则表示不限制。

+ **`size_t tempThreadLatencySeconds`**

	超过 `perfectCount` 数量限制的临时线程退出前，等待新任务的等待时间。

#### wakeUp

	virtual bool			wakeUp(ITaskPtr task);
	virtual bool			wakeUp(std::function<void ()> task);
	#if(0)
	template <class Fn, class... Args>
	virtual bool			wakeUp(Fn&& func, Args&&... args);
	#endif

向线程池中添加任务，并唤醒线程池，执行任务。

**注意**

+ 如果线程池排队任务数量超过 `maxQueueLength` 限制，将添加失败。
+ 形态 3 需要修改代码，改变宏状态才可调用。
+ 形态 2 & 3，**务必**注意，捕获或者绑定的的参数的**生命期**。**避免**当函数执行还未完成时，相关的引用计数，或者指针已经失效。