## TCPProxyCore

### 介绍

Proxy 为服务集群的代理对象，内部包含一组 [Client](../core/Client.md)，对应着该集群内部具体的服务进程。

TCPProxyCore 为 TCP Proxy 的核心基类，内部含有的 Client 为 [TCPClient](../CORE/TCPClient.md)。

### 命名空间

	namespace fpnn;

### IProxyQuestProcessorFactory

	class IProxyQuestProcessorFactory
	{
	public:
		virtual IQuestProcessorPtr generate(const std::string& host, int port) = 0;
		virtual ~IProxyQuestProcessorFactory() {}
	};
	typedef std::shared_ptr<IProxyQuestProcessorFactory> IProxyQuestProcessorFactoryPtr;

链接事件与 Server Push 处理对象 [IQuestProcessor](../core/IQuestProcessor.md) 工厂类基类。 

### TCPProxyCore

	class TCPProxyCore
	{
	public:
		TCPProxyCore(int64_t questTimeoutSeconds = -1);
		virtual ~TCPProxyCore();

		void enablePrivateQuestProcessor(IProxyQuestProcessorFactoryPtr factory);

		void setSharedQuestProcessor(IQuestProcessorPtr sharedQuestProcessor);
		void setSharedQuestProcessThreadPool(TaskThreadPoolPtr questProcessPool);

		void keepAlive();
		void setKeepAlivePingTimeout(int seconds);
		void setKeepAliveInterval(int seconds);
		void setKeepAliveMaxPingRetryCount(int count);

		void updateEndpoints(const std::vector<std::string>& newEndpoints);

		virtual bool empty();
		std::vector<std::string> endpoints();
	};

TCP Proxy 核心基类。

#### 构造函数

	TCPProxyCore(int64_t questTimeoutSeconds = -1);

**参数说明**

* **`int64_t questTimeoutSeconds`**

	Proxy 默认的请求超时时间。单位：秒。负值与0表示使用全局([ClientEngine](../core/ClientEngine.md))设置。

#### 成员函数

##### enablePrivateQuestProcessor

	void enablePrivateQuestProcessor(IProxyQuestProcessorFactoryPtr factory);

配置事件与请求处理对象工厂类。配置后，Proxy 内部每个 [TCPClient](../core/TCPClient.md) 将使用独立的事件与请求处理对象([IQuestProcessor](../core/IQuestProcessor.md))。IProxyQuestProcessorFactoryPtr 请参见 [IProxyQuestProcessorFactoryPtr](#IProxyQuestProcessorFactory)。

##### setSharedQuestProcessor

	void setSharedQuestProcessor(IQuestProcessorPtr sharedQuestProcessor);

配置共享的事件与请求处理对象([IQuestProcessor](../core/IQuestProcessor.md))。配置后，Proxy 内部每个 [TCPClient](../core/TCPClient.md) 将使用同一个事件与请求处理对象([IQuestProcessor](../core/IQuestProcessor.md))。

##### setSharedQuestProcessThreadPool

	void setSharedQuestProcessThreadPool(TaskThreadPoolPtr questProcessPool);

设置共享的链接事件和 Server Push 请求处理任务池。配之后，Proxy 内部每个 [TCPClient](../core/TCPClient.md) 将使用同一个链接事件和 Server Push 请求处理任务池，而不再使用默认的 ClientEngine 提供的链接事件和 Server Push 请求处理任务池。

TaskThreadPoolPtr 请参考 [TaskThreadPool](../base/TaskThreadPool.md)。

##### keepAlive

	void keepAlive();

开启连接自动保活/保持连接。

##### setKeepAlivePingTimeout

	void setKeepAlivePingTimeout(int seconds);

设置自动保活状态下的 ping 请求超时时间，单位：秒。默认：5 秒。

##### setKeepAliveInterval

	void setKeepAliveInterval(int seconds);

设置自动保活状态下的 ping 请求间隔时间，单位：秒。默认：20 秒。

##### setKeepAliveMaxPingRetryCount

	void setKeepAliveMaxPingRetryCount(int count);

设置开启自动保活时，ping 超时后，最大重试次数。超过该次数，认为链接丢失。默认：3 次。

##### updateEndpoints

	void updateEndpoints(const std::vector<std::string>& newEndpoints);

更新 Proxy 对应的集群的 endpoint 列表。FPZK 相关的 Proxy 无须调用该接口。

##### empty

	virtual bool empty();

判断 Proxy 对应的集群是否是空集群。

##### endpoints

	std::vector<std::string> endpoints();

获取 Proxy 对应的集群的 endpoint 列表。
