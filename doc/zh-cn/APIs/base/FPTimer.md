## FPTimer

### 介绍

FPNN Framework 基于 epoll 实现的定时器。

### 命名空间

	namespace fpnn;

### 关键定义

#### ITask

	class Timer: public std::enable_shared_from_this<Timer>
	{
	public:
		class ITask
		{
		public:
			virtual void run() = 0;
			virtual void cancelled() {}		//-- When Timer exiting.
			virtual ~ITask() {}
		};
		typedef std::shared_ptr<ITask> ITaskPtr;
	};

#### Timer

	class Timer: public std::enable_shared_from_this<Timer>
	{
	public:
		inline static TimerPtr create();
		inline static TimerPtr create(ITaskThreadPoolPtr threadPool);
		~Timer();

		bool init(ITaskThreadPoolPtr threadPool);

		/*
		  Intro:
		  	addPeriodTask(): periodInMsec will be re-calculated when task triggered.
		  	addRepeatedTask(): intervalInMsec will be re-calculated after task done.
		  	  Repeated: |- intervalInMsec -|- task executing time -|- intervalInMsec -|- task next executed -|- ...
		  	  Period: |- periodInMsec -|- periodInMsec -|- periodInMsec -|- ...

		  Tolerance for period task:
		  	In normal load, 5 msec.
		  	heavy load will be larger.

		  Parameters:
			cycleTimes: 0 means infinite.
			hour: using UTC time.

		  Return Value:
			return 0 is meaning false, lager Zero, is success, and the value is taskId. Using for removing task.
			!!! taskId will be reused. !!!
		*/
		uint64_t addTask(ITaskPtr task, uint64_t delayMsec);
		uint64_t addPeriodTask(ITaskPtr task, uint64_t periodInMsec, int cycleTimes = 0, bool runRightNow = false);
		uint64_t addRepeatedTask(ITaskPtr task, uint64_t intervalInMsec, int cycleTimes = 0, bool runRightNow = false);
		uint64_t addDailyTask(ITaskPtr task, int hour, int minute, int second);

		uint64_t addTask(std::function<void ()> task, uint64_t delayMsec);
		uint64_t addPeriodTask(std::function<void ()> task, uint64_t periodInMsec, int cycleTimes = 0, bool runRightNow = false);
		uint64_t addRepeatedTask(std::function<void ()> task, uint64_t intervalInMsec, int cycleTimes = 0, bool runRightNow = false);
		uint64_t addDailyTask(std::function<void ()> task, int hour, int minute, int second);

		//-- std::function<void (bool): bool means: true: normal, false: Timer is exiting.
		uint64_t addTask(std::function<void (bool)> task, uint64_t delayMsec);
		uint64_t addPeriodTask(std::function<void (bool)> task, uint64_t periodInMsec, int cycleTimes = 0, bool runRightNow = false);
		uint64_t addRepeatedTask(std::function<void (bool)> task, uint64_t intervalInMsec, int cycleTimes = 0, bool runRightNow = false);
		uint64_t addDailyTask(std::function<void (bool)> task, int hour, int minute, int second);

		uint64_t addTask(std::function<void ()> task, std::function<void ()> cannelled, uint64_t delayMsec);
		uint64_t addPeriodTask(std::function<void ()> task, std::function<void ()> cannelled, uint64_t periodInMsec, int cycleTimes = 0, bool runRightNow = false);
		uint64_t addRepeatedTask(std::function<void ()> task, std::function<void ()> cannelled, uint64_t intervalInMsec, int cycleTimes = 0, bool runRightNow = false);
		uint64_t addDailyTask(std::function<void ()> task, std::function<void ()> cannelled, int hour, int minute, int second);

		void removeTask(uint64_t taskId);
	};

	typedef std::shared_ptr<Timer> TimerPtr;

### 成员函数

#### ITask

##### run

	virtual void run() = 0;

定时器执行的具体任务函数。

##### cancelled

	virtual void cancelled() {}

当任务被**被动**取消时会被调用。

**注意**

当任务被显式删除时，不会调用该函数。


#### Timer

##### create

	inline static TimerPtr create();
	inline static TimerPtr create(ITaskThreadPoolPtr threadPool);

创建定时器实例。

**注意**

第一个重载需要再调用 `init()` 函数进行初始化，第二个重载无须再调用 `init()` 函数进行初始化。

**参数说明**

* **`ITaskThreadPoolPtr threadPool`**

	执行定时任务所需要的任务池/线程池。具体请参见 [TaskThreadPool](TaskThreadPool.md)。

##### init

	bool init(ITaskThreadPoolPtr threadPool);

配置定时器所需的任务池/线程池，并初始化定时器。

**参数说明**

* **`ITaskThreadPoolPtr threadPool`**

	执行定时任务所需要的任务池/线程池。具体请参见 [TaskThreadPool](TaskThreadPool.md)。

##### addTask

	uint64_t addTask(ITaskPtr task, uint64_t delayMsec);
	uint64_t addTask(std::function<void ()> task, uint64_t delayMsec);
	uint64_t addTask(std::function<void (bool)> task, uint64_t delayMsec);
	uint64_t addTask(std::function<void ()> task, std::function<void ()> cannelled, uint64_t delayMsec);

添加**单次执行**的任务。

**参数说明**

* **`ITaskPtr task`**

	需要添加的任务。

* **`std::function<void ()> task`**

	需要添加的任务。

* **`std::function<void (bool)> task`**

	需要添加的任务。bool 表示是否被取消。true 表示被正常执行，false 表示被取消。

* **`std::function<void ()> cannelled`**

	任务被取消时触发的操作。

* **`uint64_t delayMsec`**

	延迟多少毫秒执行本次任务。


**返回值**

返回 0 表示添加失败，否则为 taskId。

**注意**

taskId 保证在对应的 Timer 实例内，**当前**唯一，但**历史**不唯一。即：taskId 会在对应的 Timer 实例中，被有条件复用。

##### addPeriodTask

	uint64_t addPeriodTask(ITaskPtr task, uint64_t periodInMsec, int cycleTimes = 0, bool runRightNow = false);
	uint64_t addPeriodTask(std::function<void ()> task, uint64_t periodInMsec, int cycleTimes = 0, bool runRightNow = false);
	uint64_t addPeriodTask(std::function<void (bool)> task, uint64_t periodInMsec, int cycleTimes = 0, bool runRightNow = false);
	uint64_t addPeriodTask(std::function<void ()> task, std::function<void ()> cannelled, uint64_t periodInMsec, int cycleTimes = 0, bool runRightNow = false);

添加**周期性**任务。

**注意**

+ **周期性**任务：周期性触发。触发间隔从**上次触发时间**开始计算。两次任务执行时间仅为触发周期，不含执行时间。
+ **重复型**任务：周期性触发。触发间隔从**上次任务完成时间**开始计算。两次任务执行时间为触发周期加上上次任务执行时间。

**参数说明**

* **`ITaskPtr task`**

	需要添加的任务。

* **`std::function<void ()> task`**

	需要添加的任务。

* **`std::function<void (bool)> task`**

	需要添加的任务。bool 表示是否被取消。true 表示被正常执行，false 表示被取消。

* **`std::function<void ()> cannelled`**

	任务被取消时触发的操作。

* **`uint64_t periodInMsec`**

	触发周期。

* **`int cycleTimes`**

	触发次数。0 表示不限次数。


* **`bool runRightNow`**

	首次执行是否立刻触发，还是等一个周期后再触发。


**返回值**

返回 0 表示添加失败，否则为 taskId。

**注意**

taskId 保证在对应的 Timer 实例内，**当前**唯一，但**历史**不唯一。即：taskId 会在对应的 Timer 实例中，被有条件复用。

##### addRepeatedTask

	uint64_t addRepeatedTask(ITaskPtr task, uint64_t intervalInMsec, int cycleTimes = 0, bool runRightNow = false);
	uint64_t addRepeatedTask(std::function<void ()> task, uint64_t intervalInMsec, int cycleTimes = 0, bool runRightNow = false);
	uint64_t addRepeatedTask(std::function<void (bool)> task, uint64_t intervalInMsec, int cycleTimes = 0, bool runRightNow = false);
	uint64_t addRepeatedTask(std::function<void ()> task, std::function<void ()> cannelled, uint64_t intervalInMsec, int cycleTimes = 0, bool runRightNow = false);

添加**重复型**任务。

**注意**

+ **周期性**任务：周期性触发。触发间隔从**上次触发时间**开始计算。两次任务执行时间仅为触发周期，不含执行时间。
+ **重复型**任务：周期性触发。触发间隔从**上次任务完成时间**开始计算。两次任务执行时间为触发周期加上上次任务执行时间。

**参数说明**

* **`ITaskPtr task`**

	需要添加的任务。

* **`std::function<void ()> task`**

	需要添加的任务。

* **`std::function<void (bool)> task`**

	需要添加的任务。bool 表示是否被取消。true 表示被正常执行，false 表示被取消。

* **`std::function<void ()> cannelled`**

	任务被取消时触发的操作。

* **`uint64_t intervalInMsec`**

	每次执行的间隔时间。

* **`int cycleTimes`**

	触发次数。0 表示不限次数。


* **`bool runRightNow`**

	首次执行是否立刻触发，还是等一个周期后再触发。


**返回值**

返回 0 表示添加失败，否则为 taskId。

**注意**

taskId 保证在对应的 Timer 实例内，**当前**唯一，但**历史**不唯一。即：taskId 会在对应的 Timer 实例中，被有条件复用。

##### addDailyTask

	uint64_t addDailyTask(ITaskPtr task, int hour, int minute, int second);
	uint64_t addDailyTask(std::function<void ()> task, int hour, int minute, int second);
	uint64_t addDailyTask(std::function<void (bool)> task, int hour, int minute, int second);
	uint64_t addDailyTask(std::function<void ()> task, std::function<void ()> cannelled, int hour, int minute, int second);

添加每日定时任务。

**注意**

时间为 24 小时制，UTC 时间。

**参数说明**

* **`ITaskPtr task`**

	需要添加的任务。

* **`std::function<void ()> task`**

	需要添加的任务。

* **`std::function<void (bool)> task`**

	需要添加的任务。bool 表示是否被取消。true 表示被正常执行，false 表示被取消。

* **`std::function<void ()> cannelled`**

	任务被取消时触发的操作。


**返回值**

返回 0 表示添加失败，否则为 taskId。

**注意**

taskId 保证在对应的 Timer 实例内，**当前**唯一，但**历史**不唯一。即：taskId 会在对应的 Timer 实例中，被有条件复用。

##### removeTask

	void removeTask(uint64_t taskId);

删除任务。

**注意**

删除任务为**主动**操作，因此不会触发任务的取消事件调用。任务的取消事件，只有在**被动**取消/**被动**删除的情况下，才会触发。

