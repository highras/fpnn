## TCPFPZKRotatoryProxy

### 介绍

TCPFPZKRotatoryProxy 是以轮询为访问方式，与 FPZK 服务联动的 TCP 集群代理对象，是 [TCPFPZKProxyCore](TCPFPZKProxyCore.md) 的子类。

### 命名空间

	namespace fpnn;

### TCPFPZKRotatoryProxy

	class TCPFPZKRotatoryProxy: public TCPFPZKProxyCore
	{
	public:
		TCPFPZKRotatoryProxy(FPZKClientPtr fpzkClient, const std::string& serviceName, const std::string& cluster = "", int64_t questTimeoutSeconds = -1);
		virtual ~TCPFPZKRotatoryProxy();

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
	typedef std::shared_ptr<TCPFPZKRotatoryProxy> TCPFPZKRotatoryProxyPtr;

#### 构造函数

	TCPFPZKRotatoryProxy(FPZKClientPtr fpzkClient, const std::string& serviceName, const std::string& cluster = "", int64_t questTimeoutSeconds = -1);

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

##### getClient

	TCPClientPtr getClient(bool connect);

获取特定的 [TCPClient](../CORE/TCPClient.md)。

**参数说明**

* **`bool connect`**

	返回的 [TCPClient](../CORE/TCPClient.md) 是否必须是处于已连接的状态。

##### sendQuest

	FPAnswerPtr sendQuest(FPQuestPtr quest, int timeout = 0);
	bool sendQuest(FPQuestPtr quest, AnswerCallback* callback, int timeout = 0);
	bool sendQuest(FPQuestPtr quest, std::function<void (FPAnswerPtr answer, int errorCode)> task, int timeout = 0);

发送请求。

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

