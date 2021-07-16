## Client

### 介绍

[TCPClient](TCPClient.md) 和 [UDPClient](UDPClient.md) 的基类。提供客户端的通用方法。

**注意：请勿使用非文档化的 API。**  
非文档化的 API，或为内部使用，或因历史原因遗留，或为将来设计，后续版本均可能存在变动。

### 命名空间

	namespace fpnn;

### 关键定义

	class Client
	{
	public:
		Client(const std::string& host, int port, bool autoReconnect = true);
		virtual ~Client();

		inline bool connected();
		inline const std::string& endpoint();
		inline int socket();
		inline ConnectionInfoPtr connectionInfo();

		inline void setQuestProcessor(IQuestProcessorPtr processor);

		inline void setQuestProcessThreadPool(TaskThreadPoolPtr questProcessPool);
		inline void enableQuestProcessThreadPool(int32_t initCount, int32_t perAppendCount, int32_t perfectCount, int32_t maxCount, size_t maxQueueSize);
		inline void enableAnswerCallbackThreadPool(int32_t initCount, int32_t perAppendCount, int32_t perfectCount, int32_t maxCount);

		inline void setQuestTimeout(int64_t seconds);
		inline int64_t getQuestTimeout();

		virtual bool connect() = 0;
		virtual void close();
		virtual bool reconnect();

		/**
			All SendQuest():
				If return false, caller must free quest & callback.
				If return true, don't free quest & callback.

			timeout in seconds.
		*/
		virtual FPAnswerPtr sendQuest(FPQuestPtr quest, int timeout = 0);
		virtual bool sendQuest(FPQuestPtr quest, AnswerCallback* callback, int timeout = 0);
		virtual bool sendQuest(FPQuestPtr quest, std::function<void (FPAnswerPtr answer, int errorCode)> task, int timeout = 0);

		static TCPClientPtr createTCPClient(const std::string& host, int port, bool autoReconnect = true);
		static TCPClientPtr createTCPClient(const std::string& endpoint, bool autoReconnect = true);

		static UDPClientPtr createUDPClient(const std::string& host, int port, bool autoReconnect = true);
		static UDPClientPtr createUDPClient(const std::string& endpoint, bool autoReconnect = true);
	};

	typedef std::shared_ptr<Client> ClientPtr;

### 构造函数

	Client(const std::string& host, int port, bool autoReconnect = true);

**参数说明**

* **`const std::string& host`**

	服务器 IP 地址或者域名。

* **`int port`**

	服务器端口。

* **`bool autoReconnect`**

	启用或者禁用自动重连功能。

### 成员函数

#### connected

	inline bool connected();

判断链接是否已经建立。

#### endpoint

	inline const std::string& endpoint();

返回要链接的对端服务器的 endpoint。

#### socket

	inline int socket();

返回当前 socket。

#### connectionInfo

	inline ConnectionInfoPtr connectionInfo();

返回当前的连接信息对象 [ConnectionInfo](ConnectionInfo.md)。

**注意**

每次链接，连接信息对象均不相同。


#### setQuestProcessor

	inline void setQuestProcessor(IQuestProcessorPtr processor);

配置链接事件和 Server Push 请求处理模块。具体可参见 [IQuestProcessor](IQuestProcessor.md)。


#### setQuestProcessThreadPool

	inline void setQuestProcessThreadPool(TaskThreadPoolPtr questProcessPool);

设置当前实例私有的独立的处理 Server Push 请求的任务池。默认将使用 ClientEngine 提供的处理 Server Push 请求所用的线程池。

TaskThreadPoolPtr 请参考 [TaskThreadPool](../base/TaskThreadPool.md)。

#### enableQuestProcessThreadPool

	inline void enableQuestProcessThreadPool(int32_t initCount, int32_t perAppendCount, int32_t perfectCount, int32_t maxCount, size_t maxQueueSize);

配置当前实例私有的独立的处理 Server Push 请求的任务池。默认将使用 ClientEngine 提供的处理 Server Push 请求所用的线程池。

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

+ **`size_t maxQueueSize`**

	线程池任务队列最大数量限制。如果为 `0`，则表示不限制。


#### enableAnswerCallbackThreadPool

	inline void enableAnswerCallbackThreadPool(int32_t initCount, int32_t perAppendCount, int32_t perfectCount, int32_t maxCount);

配置当前实例私有的独立的应答处理和 Callback 执行的任务线程池。默认将使用 ClientEngine 提供的应答处理和 Callback 执行的任务线程池。

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

#### setQuestTimeout

	inline void setQuestTimeout(int64_t seconds);

设置当前 Client 实例的请求超时。单位：秒。`0` 表示使用 ClientEngine 的请求超时设置。

#### getQuestTimeout

	inline int64_t getQuestTimeout();

获取当前 Client 实例的请求超时设置。

#### connect

	virtual bool connect() = 0;

同步连接服务器。

#### close

	virtual void close();

关闭当前连接。

#### reconnect

	virtual bool reconnect();

同步重连。

#### sendQuest

	virtual FPAnswerPtr sendQuest(FPQuestPtr quest, int timeout = 0);
	virtual bool sendQuest(FPQuestPtr quest, AnswerCallback* callback, int timeout = 0);
	virtual bool sendQuest(FPQuestPtr quest, std::function<void (FPAnswerPtr answer, int errorCode)> task, int timeout = 0);

发送请求。

第一个声明为同步发送，后两个声明为异步发送。

**参数说明**

* **`FPQuestPtr quest`**

	请求数据对象。具体请参见 [FPQuest](../proto/FPQuest.md)。

	**注意**

	对于 UDP 连接，oneway 消息被视为可丢弃消息；twoway 消息被视为不可丢弃消息。如果需要显式指定消息的可丢弃属性，请使用 [UDPClient](UDPClient.md) 的 [sendQuestEx](UDPClient.md#sendQuestEx) 接口。

* **`int timeout`**

	本次请求的超时设置。单位：秒。

	**注意**

	如果 `timeout` 为 0，表示使用 Client 实例当前的请求超时设置。  
	如果 Client 实例当前的请求超时设置为 0，则使用 ClientEngine 的请求超时设置。

* **`AnswerCallback* callback`**

	异步请求的回调对象。具体请参见 [AnswerCallback](AnswerCallback.md)。

* **`std::function<void (FPAnswerPtr answer, int errorCode)> task`**

	异步请求的回调函数。

	**注意**

	+ 如果遇到连接中断/结束，连接已关闭，超时等情况，`answer` 将为 `nullptr`。
	+ 当且仅当 `errorCode == FPNN_EC_OK` 时，`answer` 为业务正常应答；否则其它情况，如果 `answer` 不为 `nullptr`，则为 FPNN 异常应答。

		FPNN 异常应答请参考 [errorAnswer](../proto/FPWriter.md#errorAnswer)。

**返回值说明**

* **FPAnswerPtr**

	+ 对于 oneway 请求，FPAnswerPtr 的返回值**恒定为** `nullptr`。
	+ 对于 twoway 请求，FPAnswerPtr 可能为正常应答，也可能为异常应答。FPNN 异常应答请参考 [errorAnswer](../proto/FPWriter.md#errorAnswer)。

* **bool**

	发送成功，返回 true；失败 返回 false。

	**注意**

	如果发送成功，`AnswerCallback* callback` 将不能再被复用，用户将无须处理 `callback` 对象的释放。SDK 会在合适的时候，调用 `delete` 操作进行释放；  
	如果返回失败，用户需要处理 `AnswerCallback* callback` 对象的释放。

#### createTCPClient

	static TCPClientPtr createTCPClient(const std::string& host, int port, bool autoReconnect = true);
	static TCPClientPtr createTCPClient(const std::string& endpoint, bool autoReconnect = true);

创建 TCP 客户端。

**参数说明**

* **`const std::string& host`**

	服务器 IP 或者域名。

* **`int port`**

	服务器端口。

* **`const std::string& endpoint`**

	服务器 endpoint。

* **`bool autoReconnect`**

	是否自动重连。

#### createUDPClient

	static UDPClientPtr createUDPClient(const std::string& host, int port, bool autoReconnect = true);
	static UDPClientPtr createUDPClient(const std::string& endpoint, bool autoReconnect = true);

创建 UDP 客户端。

**参数说明**

* **`const std::string& host`**

	服务器 IP 或者域名。

* **`int port`**

	服务器端口。

* **`const std::string& endpoint`**

	服务器 endpoint。

* **`bool autoReconnect`**

	是否自动重连。
