## ParamTemplateThreadPoolArray

### 介绍

参数化线程池模版阵列。

本线程池阵列为 [ParamTemplateThreadPool](ParamTemplateThreadPool.md) 的阵列封装，因此核心接口与 [ParamTemplateThreadPool](ParamTemplateThreadPool.md) 一致。

### 命名空间

	namespace fpnn;

### 关键定义

	template<typename K>
	class ParamTemplateThreadPoolArray
	{
	public:
		ParamTemplateThreadPoolArray(int count, std::shared_ptr<typename ParamTemplateThreadPool<K>::IProcessor> processor);
		ParamTemplateThreadPoolArray(int count);
		ParamTemplateThreadPoolArray();
		~ParamTemplateThreadPoolArray();

		void config(int count, std::shared_ptr<typename ParamTemplateThreadPool<K>::IProcessor> processor = nullptr);
		bool init(int32_t initCount, int32_t perAppendCount, int32_t perfectCount, int32_t maxCount, size_t maxQueueLength = 0);

		bool wakeUp(int hint, K task);
		bool wakeUp(K task);

		bool forceWakeUp(int hint, K task);
		bool forceWakeUp(K task);

		bool priorWakeUp(int hint, K task);
		bool priorWakeUp(K task);

		void release();

		void status(int32_t &normalThreadCount, int32_t &temporaryThreadCount, int32_t &busyThreadCount, int32_t &taskQueueSize, int32_t& min, int32_t& max, int32_t& maxQueue);
		std::string infos();

		inline void setProcessor(std::shared_ptr<typename ParamTemplateThreadPool<K>::IProcessor> processor);
		inline std::shared_ptr<typename ParamTemplateThreadPool<K>::IProcessor> processor();

		inline bool inited();
		inline bool exiting();
	};

### 构造函数

	ParamTemplateThreadPoolArray(int count, std::shared_ptr<typename ParamTemplateThreadPool<K>::IProcessor> processor);
	ParamTemplateThreadPoolArray(int count);
	ParamTemplateThreadPoolArray();

**参数说明**

* **`int count`**

	阵列数量。即，所含线程池数量。

* **`std::shared_ptr<typename ParamTemplateThreadPool<K>::IProcessor> processor`**

	任务参数处理模块。

### 成员函数

#### config

	void config(int count, std::shared_ptr<typename ParamTemplateThreadPool<K>::IProcessor> processor = nullptr);

配置阵列所含线程池数量，以及任务参数处理模块。

#### init

	bool init(int32_t initCount, int32_t perAppendCount, int32_t perfectCount, int32_t maxCount, size_t maxQueueLength = 0);

初始化线程池阵列。

**注意**

+ 初始化线程池阵列前，必须完成线程池阵列配置。
+ 接口参数为线程池阵列总参数，初始化线程池阵列内每一个线程池时，会做等分。

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

#### wakeUp

	bool wakeUp(int hint, K task);
	bool wakeUp(K task);

向线程池中添加任务，并唤醒线程池，执行任务。  
如果线程池排队任务数量超过 `maxQueueLength` 限制，将添加失败。

**注意**

`int hint` 参数为线程池选择参数。按阵列数量取模后，便是所选择线程池索引编号。  
对于不含该参数的接口，实际执行任务的线程池将按线程池索引轮换。

#### forceWakeUp

	bool forceWakeUp(int hint, K task);
	bool forceWakeUp(K task);

向线程池中添加任务，并唤醒线程池，执行任务。  
忽略 `maxQueueLength` 限制，除非线程池处于未初始化或退出状态，必定添加成功。

**注意**

`int hint` 参数为线程池选择参数。按阵列数量取模后，便是所选择线程池索引编号。  
对于不含该参数的接口，实际执行任务的线程池将按线程池索引轮换。

#### priorWakeUp

	bool priorWakeUp(int hint, K task);
	bool priorWakeUp(K task);

向线程池中添加任务，并唤醒线程池，执行任务。  
忽略 `maxQueueLength` 限制，除非线程池处于未初始化或退出状态，必定添加成功。

**注意**

`int hint` 参数为线程池选择参数。按阵列数量取模后，便是所选择线程池索引编号。  
对于不含该参数的接口，实际执行任务的线程池将按线程池索引轮换。

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

	inline void setProcessor(std::shared_ptr<typename ParamTemplateThreadPool<K>::IProcessor> processor);

设置任务参数处理模块。

#### processor

	inline std::shared_ptr<typename ParamTemplateThreadPool<K>::IProcessor> processor();

获取任务参数处理模块。

#### inited

	inline bool inited();

判断线程池是否初始化。

#### exiting

	inline bool exiting();

判断线程池是否正在释放/退出。