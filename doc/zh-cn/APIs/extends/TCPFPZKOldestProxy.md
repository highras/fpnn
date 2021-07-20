## TCPFPZKOldestProxy

### 介绍

TCPFPZKOldestProxy 是与 FPZK 服务联动的，以仅能访问在 FPZK 系统中注册时间最久的服务实例的 TCP 集群代理对象，是 [TCPFPZKProxyCore](TCPFPZKProxyCore.md) 的子类。

### 命名空间

	namespace fpnn;

### TCPFPZKOldestProxy

	class TCPFPZKOldestProxy: public TCPFPZKProxyCore
	{
	public:
		TCPFPZKOldestProxy(FPZKClientPtr fpzkClient, const std::string& serviceName, const std::string& cluster = "", int64_t questTimeoutSeconds = -1);
		virtual ~TCPFPZKOldestProxy();

		std::string getOldestEndpoint();

		bool isMyself();

		TCPClientPtr getClient(bool connect);

		/**
			All SendQuest():
				If return false, caller must free quest & callback.
				If return true, don't free quest & callback.

			timeout in seconds.
		*/
		FPAnswerPtr sendQuest(FPQuestPtr quest, int timeout = 0);
		bool sendQuest(FPQuestPtr quest, AnswerCallback* callback, int timeout = 0);
		bool sendQuest(FPQuestPtr quest, std::function<void (FPAnswerPtr answer, int errorCode)> task, int timeout = 0);
	};
	typedef std::shared_ptr<TCPFPZKOldestProxy> TCPFPZKOldestProxyPtr;

#### 构造函数

	TCPFPZKOldestProxy(FPZKClientPtr fpzkClient, const std::string& serviceName, const std::string& cluster = "", int64_t questTimeoutSeconds = -1);

**参数说明**

* **`FPZKClientPtr fpzkClient`**

	关联的 [FPZKClient](FPZKClient.md) 实例。

* **`const std::string& serviceName`**

	集群名称。

* **`const std::string& cluster`**

	集群二级分组名称。

* **`int64_t questTimeoutSeconds`**

	Proxy 默认的请求超时时间。单位：秒。负值与0表示使用全局([ClientEngine](../core/ClientEngine.md))设置。

#### 成员函数

本文档仅列出基于基类所扩展的成员函数，其余成员函数请参考基类文档 [TCPFPZKProxyCore](TCPFPZKProxyCore.md) 及 [TCPProxyCore](TCPProxyCore.md)。

##### getOldestEndpoint

	std::string getOldestEndpoint();

获取在 FPZK 系统中，注册时间最久的服务实例的 endpoint。

##### isMyself

	bool isMyself();

判断在 FPZK 系统中，注册时间最久的服务实例是否是自身。

##### getClient

	TCPClientPtr getClient(bool connect);

获取在 FPZK 系统中，注册时间最久的服务实例的 [TCPClient](../core/TCPClient.md)。

**参数说明**

* **`bool connect`**

	返回的 [TCPClient](../core/TCPClient.md) 是否必须是处于已连接的状态。

##### sendQuest

	FPAnswerPtr sendQuest(FPQuestPtr quest, int timeout = 0);
	bool sendQuest(FPQuestPtr quest, AnswerCallback* callback, int timeout = 0);
	bool sendQuest(FPQuestPtr quest, std::function<void (FPAnswerPtr answer, int errorCode)> task, int timeout = 0);

向在 FPZK 系统中，注册时间最久的服务实例发送请求。

第一个声明为同步发送，后两个声明为异步发送。

**参数说明**

* **`FPQuestPtr quest`**

	请求数据对象。具体请参见 [FPQuest](../proto/FPQuest.md)。

* **`int timeout`**

	本次请求的超时设置。单位：秒。

	**注意**

	如果 `timeout` 为 0，表示使用 Proxy/Client 实例当前的请求超时设置。  
	如果 Proxy/Client 实例当前的请求超时设置为 0，则使用 ClientEngine 的请求超时设置。

* **`AnswerCallback* callback`**

	异步请求的回调对象。具体请参见 [AnswerCallback](../core/AnswerCallback.md)。

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
