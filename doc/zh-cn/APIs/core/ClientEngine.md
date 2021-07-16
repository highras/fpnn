## ClientEngine

### 介绍

ClientEngine 为全局的 Client(TCP Client & UDP Client) 网络连接的管理模块。为单例实现。

文件配置可参见 [conf.template](../../../conf.template) 相关条目。

**注意：请勿使用非文档化的 API。**  
非文档化的 API，或为内部使用，或因历史原因遗留，或为将来设计，后续版本均可能存在变动。

### 命名空间

	namespace fpnn;

### 关键定义

	class ClientEngine
	{
	public:
		virtual ~ClientEngine();

		static ClientEngine* nakedInstance();
		static ClientEnginePtr instance();
		static bool created();

		/*===============================================================================
		  Configuration.
		=============================================================================== */
		inline static void setEstimateMaxConnections(size_t estimateMaxConnections, int partitionCount = 64);

		//-- answer callback & error/close event.
		inline static void configAnswerCallbackThreadPool(int32_t initCount, int32_t perAppendCount, int32_t perfectCount, int32_t maxCount);

		inline static void configQuestProcessThreadPool(int32_t initCount, int32_t perAppendCount, int32_t perfectCount, int32_t maxCount, size_t maxQueueSize);

		inline static std::string answerCallbackPoolStatus();

		inline static std::string questProcessPoolStatus();

        inline static bool questProcessPoolExiting();

        inline static void setQuestTimeout(int64_t seconds);
		inline static int64_t getQuestTimeout();
		
		inline static bool wakeUpQuestProcessThreadPool(std::shared_ptr<ITaskThreadPool::ITask> task);
		inline static bool wakeUpAnswerCallbackThreadPool(std::shared_ptr<ITaskThreadPool::ITask> task);
		size_t count();
	};

	typedef std::shared_ptr<ClientEngine> ClientEnginePtr;

**注意**

代码中，`class ClientEngine` 的基类，和非文档化的接口可能随时会更改。因此，请仅使用本文档中，**明确列出**的接口。

### 成员函数

#### nakedInstance

	static ClientEngine* nakedInstance();

获取 ClientEngine 单例的裸指针。  
如果 ClientEngine 单例未被创建，将创建 ClientEngine 单例。

#### instance

	static ClientEnginePtr instance();

获取 ClientEngine 单例的 std::shared_ptr 指针。  
如果 ClientEngine 单例未被创建，将创建 ClientEngine 单例。

#### created

	static bool created();

判断 ClientEngine 单例是否已被创建。

#### setEstimateMaxConnections

	inline static void setEstimateMaxConnections(size_t estimateMaxConnections, int partitionCount = 64);

设置 ClientEngine 支持的连接数峰值。

**注意**

+ 该接口为调优使用，一般情况采用默认配置即可，无需调用。
+ 该接口必须在 ClientEngine 启动前，即任何 Client（TCPClient & UDPClient）数据发送前，调用。否则将会被忽略。

**参数说明**

* **`size_t estimateMaxConnections`**

	预估的 ClientEngine 峰值的连接数。

* **`int partitionCount`**

	连接管理模块的数量。

#### configAnswerCallbackThreadPool

	inline static void configAnswerCallbackThreadPool(int32_t initCount, int32_t perAppendCount, int32_t perfectCount, int32_t maxCount);

配置 Client 的应答处理和 Callback 执行的线程池。

**注意**

+ 一般情况采用默认配置即可。
+ 该接口必须在 ClientEngine 启动前，即任何 Client（TCPClient & UDPClient）数据发送前调用。否则将会被忽略。
+ 如果在显示调用该接口，则配置文件中的相关配置将被忽略。

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

#### configQuestProcessThreadPool

	inline static void configQuestProcessThreadPool(int32_t initCount, int32_t perAppendCount, int32_t perfectCount, int32_t maxCount, size_t maxQueueSize);

配置 Client 处理 Server Push 请求所用的线程池。

**注意**

+ 一般情况采用默认配置即可。
+ 该接口必须在 ClientEngine 启动前，即任何 Client（TCPClient & UDPClient）数据发送前调用。否则将会被忽略。
+ 如果在显示调用该接口，则配置文件中的相关配置将被忽略。

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

#### answerCallbackPoolStatus

	inline static std::string answerCallbackPoolStatus();

返回处理 Client 发送请求的应答和 Client 执行 发送请求的 Callback 的线程池的状态。Json 格式。

#### questProcessPoolStatus

	inline static std::string questProcessPoolStatus();

返回 Client 处理 Server Push 请求所用的线程池的状态。Json 格式。

#### questProcessPoolExiting

    inline static bool questProcessPoolExiting();

判断 Client 处理 Server Push 请求所用的线程池是否处于退出状态。

#### setQuestTimeout

    inline static void setQuestTimeout(int64_t seconds);

配置 Client（TCPClient & UDPClient）请求超时的全局默认值。

#### getQuestTimeout

	inline static int64_t getQuestTimeout();

获取 Client（TCPClient & UDPClient）请求超时的全局默认值。

#### wakeUpQuestProcessThreadPool

	inline static bool wakeUpQuestProcessThreadPool(std::shared_ptr<ITaskThreadPool::ITask> task);

在 Client 处理应答和发送的请求的 Callback 执行的线程池中，执行其他任务。

**注意**：一般不推荐使用，除非是临时的短小任务。否则存在阻塞 Client 响应的可能。

#### wakeUpAnswerCallbackThreadPool

	inline static bool wakeUpAnswerCallbackThreadPool(std::shared_ptr<ITaskThreadPool::ITask> task);

在 Client 处理 Server Push 请求所用的线程池中，执行其他任务。

**注意**：一般不推荐使用，除非是临时的短小任务。否则存在阻塞 Client 响应 Server Push 的可能。

#### count

	size_t count();

获取当前 Client 链接数量。包含 TCPClient 和 UDPClient。