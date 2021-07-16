## ITaskThreadPool

### 介绍

线程池/任务池接口定义。

### 命名空间

	namespace fpnn;

### ITask

	class ITaskThreadPool
	{
	public:
		class ITask
		{
			public:
				virtual void run() = 0;
				virtual ~ITask() {}
		};
		typedef std::shared_ptr<ITask> ITaskPtr;
	};

线程池标准线程任务接口定义。

#### run

	virtual void run() = 0;

线程池需要执行的具体任务函数。

### ITaskThreadPool

	class ITaskThreadPool
	{
	public:
		virtual bool init(int32_t initCount, int32_t appendCount, int32_t perfectCount, int32_t maxCount, size_t maxQueueLength = 0, size_t tempThreadLatencySeconds = 60) = 0;
		virtual bool wakeUp(ITaskPtr task) = 0;
		virtual bool wakeUp(std::function<void ()> task) = 0;
		virtual void release() = 0;
		virtual void status(int32_t &normalThreadCount, int32_t &temporaryThreadCount, int32_t &busyThreadCount, int32_t &taskQueueSize, int32_t& min, int32_t& max, int32_t& maxQueue) = 0;
		virtual std::string infos() = 0;

		virtual bool inited() = 0;
		virtual bool exiting() = 0;

		virtual ~ITaskThreadPool() {}
	};
	typedef std::shared_ptr<ITaskThreadPool> ITaskThreadPoolPtr;

#### init

	virtual bool init(int32_t initCount, int32_t appendCount, int32_t perfectCount, int32_t maxCount, size_t maxQueueLength = 0, size_t tempThreadLatencySeconds = 60) = 0;

初始化线程池。

**参数说明**

* **`int32_t initCount`**

	初始的线程数量。

* **`int32_t appendCount`**

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

	virtual bool wakeUp(ITaskPtr task) = 0;
	virtual bool wakeUp(std::function<void ()> task) = 0;

向线程池中添加任务，并唤醒线程池，执行任务。

#### release

	virtual void release() = 0;

释放线程池。  
释放后的线程池，可重新初始化。


#### status

	virtual void status(int32_t &normalThreadCount, int32_t &temporaryThreadCount, int32_t &busyThreadCount, int32_t &taskQueueSize, int32_t& min, int32_t& max, int32_t& maxQueue) = 0;
	
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

	virtual std::string infos() = 0;

以 JSON 格式，返回线程池状态。

#### inited

	virtual bool inited() = 0;

判断线程池是否初始化。

#### exiting

	virtual bool exiting() = 0;

判断线程池是否正在释放/退出。