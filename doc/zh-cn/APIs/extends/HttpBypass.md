## HttpBypass

### 介绍

FPNN 数据以 HTTP 的方式，旁路到指定的 URL。

**注意**

+ 本模块必须由配置文件驱动。

	配置条目为：

		# split by ";, "
		HTTPBypass.methods.list =
		# ? 为 methods 里表中对应的 method
		HTTPBypass.method.?.url =
		# timeout 默认 120 秒
		HTTPBypass.Task.timeoutSeconds =

	例 1:

		HTTPBypass.methods.list = deomHethod
		HTTPBypass.method.deomHethod.url = http://demo.com/s/deomHethodInterface

	例 2:

		HTTPBypass.methods.list = liveStart, liveFinish, watchLive
		HTTPBypass.method.liveStart.url = http://demo.com/s/liveStart
		HTTPBypass.method.liveFinish.url = http://demo.com/s/liveFinish
		HTTPBypass.method.watchLive.url = http://demo.com/s/watchLive

	如果服务没有自己的 [MultipleURLEngine](MultipleURLEngine.md)， 或者希望 HTTPBypass 使用独立的 [MultipleURLEngine](MultipleURLEngine.md)，则须要增加以下条目：

		HTTPBypass.URLEngine.connsInPerThread = 
		HTTPBypass.URLEngine.ThreadPool.initCount = 
		HTTPBypass.URLEngine.ThreadPool.perfectCount = 
		HTTPBypass.URLEngine.ThreadPool.maxCount = 
		HTTPBypass.URLEngine.maxConcurrentTaskCount =
		HTTPBypass.URLEngine.ThreadPool.latencySeconds =

	HttpBypass 具体配置项目亦可参见 [HttpBypass.h](../../../../extends/HttpBypass.h) 的头部注释。

### 命名空间

	namespace fpnn;

### 关键定义

	class HttpBypass
	{
	public:
		HttpBypass(std::shared_ptr<MultipleURLEngine> engine = nullptr);
		~HttpBypass();
		void bypass(const FPQuestPtr quest);
	};
	typedef std::shared_ptr<HttpBypass> HttpBypassPtr;

### 构造函数

	HttpBypass(std::shared_ptr<MultipleURLEngine> engine = nullptr);

**参数说明**

* **`std::shared_ptr<MultipleURLEngine> engine`**

	多路 URL 并发访问引擎 [MultipleURLEngine](MultipleURLEngine.md)。如果值为 `nullptr`，HttpBypass 将通过配置文件，初始化私有的 [MultipleURLEngine](MultipleURLEngine.md)。

### 成员函数

#### bypass

	void bypass(const FPQuestPtr quest);

旁路指定的请求数据。

当旁路失败时，不会重试。相关信息将会以 `ERROR` 级别，输出到日志中。日志请参考：[FPLog](../base/FPLog.md)。
