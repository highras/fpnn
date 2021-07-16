## ParamTemplateThreadPool

### 介绍

参数化线程池模版。

因为本线程池是 FPNN Framework 特定线程池实现，所以没有继承自 [ITaskThreadPool](ITaskThreadPool.md)，但具体接口和 [ITaskThreadPool](ITaskThreadPool.md) 基本无异。

### 命名空间

	namespace fpnn;

### 关键定义

#### IProcessor

	template<typename K>
	class ParamTemplateThreadPool
	{
	public:
		class IProcessor
		{
			public:
				/** Please release param in run() function. */
				virtual void run(K param) = 0;
				virtual ~IProcessor() {}
		};
		typedef std::shared_ptr<IProcessor> IProcessorPtr;
	};

任务参数处理模块。

##### run

	virtual void run(K param) = 0;

任务参数执行入口。

#### ParamTemplateThreadPool

	template<typename K>
	class ParamTemplateThreadPool
	{
	public:
		bool					init(int32_t initCount, int32_t perAppendCount, int32_t perfectCount, int32_t maxCount, size_t maxQueueLength = 0, size_t tempThreadLatencySeconds = 60);
		bool					wakeUp(K task);
		bool					forceWakeUp(K task);
		bool					priorWakeUp(K task);
		void					release();
		void					status(int32_t &normalThreadCount, int32_t &temporaryThreadCount, int32_t &busyThreadCount, int32_t &taskQueueSize, int32_t& min, int32_t& max, int32_t& maxQueue);
		std::string				infos();

		inline void				setProcessor(IProcessorPtr processor);
		inline IProcessorPtr		processor();

		inline bool inited();
		inline bool exiting();

		ParamTemplateThreadPool(IProcessorPtr processor);
		ParamTemplateThreadPool();
		~ParamTemplateThreadPool();
	};

### 构造函数

	ParamTemplateThreadPool(IProcessorPtr processor);
	ParamTemplateThreadPool();

**参数说明**

* **`IProcessorPtr processor`**

	任务参数处理模块。

### 成员函数

#### init

	bool init(int32_t initCount, int32_t perAppendCount, int32_t perfectCount, int32_t maxCount, size_t maxQueueLength = 0, size_t tempThreadLatencySeconds = 60);

初始化线程池。

**参数说明**

* **`int32_t initCount`**

	初始的线程数量。

* **`int32_t perAppendCount`**

	追加线程时，单次的追加线程数量。

* **`int32_t perfectCount`**

	线程池**常驻**线程的最大数量。

* **`int32_t maxCount`**

	线程池线程最大数量。如果为 `0`，则表示不限制。

* **`size_t maxQueueLength`**

	线程池任务队列最大数量限制。如果为 `0`，则表示不限制。

* **`size_t tempThreadLatencySeconds`**

	超过 `perfectCount` 数量限制的临时线程退出前，等待新任务的等待时间。

#### wakeUp

	bool wakeUp(K task);

向线程池中添加任务，并唤醒线程池，执行任务。  
如果线程池排队任务数量超过 `maxQueueLength` 限制，将添加失败。

#### forceWakeUp

	bool forceWakeUp(K task);

向线程池中添加任务，并唤醒线程池，执行任务。  
忽略 `maxQueueLength` 限制，除非线程池处于未初始化或退出状态，必定添加成功。

#### priorWakeUp

	bool priorWakeUp(K task);

向线程池中添加任务，并唤醒线程池，执行任务。  
忽略 `maxQueueLength` 限制，除非线程池处于未初始化或退出状态，必定添加成功。

**注意**

当前版本的添加的优先任务，将按照先入后出的顺序排队执行。

#### release

	void release();

释放线程池。  
释放后的线程池，可重新初始化。

#### status

	void status(int32_t &normalThreadCount, int32_t &temporaryThreadCount, int32_t &busyThreadCount, int32_t &taskQueueSize, int32_t& min, int32_t& max, int32_t& maxQueue);

获取线程池状态。

**参数说明**

* **`normalThreadCount`**

	当前常驻线程数量。

* **`temporaryThreadCount`**

	当前临时线程数量。

* **`busyThreadCount`**

	当前正在执行任务的线程数量。

* **`taskQueueSize`**

	当前待处理的任务数量（不含执行中的）。

* **`min`**

	初始化线程池时的初始化线程数。

* **`max`**

	最大线程数限制。

* **`maxQueue`**

	最大任务队列限制。

#### infos

	std::string infos();

以 JSON 格式，返回线程池状态。

#### setProcessor

	inline void setProcessor(IProcessorPtr processor);

设置任务参数处理模块。

#### processor

	inline IProcessorPtr processor();

获取任务参数处理模块。

#### inited

	inline bool inited();

判断线程池是否初始化。

#### exiting

	inline bool exiting();

判断线程池是否正在释放/退出。