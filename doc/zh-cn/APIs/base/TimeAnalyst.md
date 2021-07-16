## TimeAnalyst

### 介绍

时间消耗分析模块。

### 命名空间

	namespace fpnn;

### 全局函数

#### diff_timeval

	struct timeval diff_timeval(struct timeval start, struct timeval finish);

计算两个 struct timeval 之间的差值。

### SegmentTimeAnalyst

	class SegmentTimeAnalyst
	{
	public:
		SegmentTimeAnalyst(const std::string& desc);
		~SegmentTimeAnalyst();
		
		inline void start(const std::string& desc);
		inline void end(const std::string& desc);

		inline void mark(const std::string& desc);

		inline void fragmentStart(const std::string& desc);
		inline void fragmentEnd(const std::string& desc);
	};

侵入式的多线程安全的时间消耗分析记录器。

除了总的时间消耗外，还可以跟踪记录每一个流程**分段**的时间消耗，以及每一个循环的次数，总时间消耗和每个循环**片段**平均时间消耗。  
当 SegmentTimeAnalyst 实例析构时，以上时间消耗数据均会以 `"Time cost"` 为 tag，用 [UXLOG](FPLog.md#UXLOG) 进行输出。

**注意**

+ 流程分段的文字描述不能重复，否则会覆盖。
+ 每一个循环，循环片段的文字描述需要相同，不然会被记录问两个不同的循环。
+ 任何侵入式的分析，都会带来额外的性能和时间开销。除非必要，否则不建议对循环进行片段记录和分析。
+ 实际执行精度为**微妙**，但日志输出只到**毫秒**。如有必要，可调整代码，修改日志输出精度。

#### 构造函数

	SegmentTimeAnalyst(const std::string& desc);

指定本次记录的描述文字。描述文字将在日志中输出。

**注意**

+ 当 SegmentTimeAnalyst 实例创建后，就开始计时；
+ 当 SegmentTimeAnalyst 实例析构时，停止计时。
+ 整个流程的耗时，其子描述文字是 `"__FLOW_COST__"`。

#### start

	inline void start(const std::string& desc);

开始记录一个新的流程分段耗时。参数为该流程分段的子描述文字。

#### end

	inline void end(const std::string& desc);

结束记录一个流程分段的耗时。参数为该流程分段的子描述文字。

#### mark

	inline void mark(const std::string& desc);

标记该子描述文字描述的事件的发生时间。

#### fragmentStart

	inline void fragmentStart(const std::string& desc);

开始记录一个新的循环片段耗时。参数为该循环片段的子描述文字。

#### fragmentEnd

	inline void fragmentEnd(const std::string& desc);

结束记录一个循环片段的耗时。参数为该循环片段的子描述文字。


### SegmentTimeMonitor

	struct SegmentTimeMonitor
	{
	public:
		SegmentTimeMonitor(SegmentTimeAnalyst* analyst, const std::string& desc);
		~SegmentTimeMonitor();
		void end();
	};

当实例创建时，开始记录流程分段的时间消耗。当实例析构时，结束记录对应流程分段的时间消耗。需配合 (SegmentTimeAnalyst)[#SegmentTimeAnalyst] 使用。

### FragmentTimeMonitor

	struct FragmentTimeMonitor
	{
	public:
		FragmentTimeMonitor(SegmentTimeAnalyst* analyst, const std::string& desc): _done(false), _analyst(analyst);
		~FragmentTimeMonitor();
		void end();
	};

当实例创建时，开始记录循环片段的时间消耗。当实例析构时，结束记录对应循环片段的时间消耗。需配合 (SegmentTimeAnalyst)[#SegmentTimeAnalyst] 使用。

### TimeCostAlarm

	class TimeCostAlarm
	{
	public:
		TimeCostAlarm(const std::string& desc, time_t second_threshold, suseconds_t microsecond_threshold);
		TimeCostAlarm(const std::string& desc, int32_t millisecond_threshold);
		~TimeCostAlarm();
	};

超时警告对象。

当实例创建时，开始记录时间消耗。当实例析构时，将检测时间消耗是否超过构造时设定的时长。如果实际执行时长大于设定的时长，将使用 [LOG_WARN](FPLog.md#LOG_WARN) 在日志中输出实际执行时长。

**注意**：实际执行精度为**微妙**，但日志输出只到**毫秒**。如有必要，可调整代码，修改日志输出精度。
