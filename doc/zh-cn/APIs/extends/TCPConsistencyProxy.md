## TCPConsistencyProxy

### 介绍

TCPConsistencyProxy 是以一致性集群广播为访问方式的 TCP 集群代理对象，是 [TCPProxyCore](TCPProxyCore.md) 的子类。

TCPConsistencyProxy 默认所有服务实例对同一请求的正常响应完全相同，因此，当一致性达成时，仅返回其收到的所有正常响应之一作为 proxy 请求的回应。

如需获取每一个服务实例的响应数据，请使用 [TCPBroadcastProxy](TCPBroadcastProxy.md)。

### 命名空间

	namespace fpnn;

### ConsistencySuccessCondition

	enum class ConsistencySuccessCondition
	{
		AllQuestsSuccess,
		HalfQuestsSuccess,
		CountedQuestsSuccess,
		OneQuestSuccess,
	};

一致性达成类型枚举。

| 枚举值 | 含义 |
|-------|-----|
| AllQuestsSuccess | 全部成功方才达成一致性 |
| HalfQuestsSuccess | 半数以上成功算作一致性达成 |
| CountedQuestsSuccess | 指定数量成功便视为一致性达成 |
| OneQuestSuccess | 任一成功视为一致性达成 |

### TCPConsistencyProxy

	class TCPConsistencyProxy: virtual public TCPProxyCore
	{
	public:
		TCPConsistencyProxy(ConsistencySuccessCondition condition, int requiredCount = 0, int64_t questTimeoutSeconds = -1);
		virtual ~TCPConsistencyProxy();

		TCPClientPtr getClient(const std::string& endpoint, bool connect);

		/**
			All SendQuest():
				If return false, caller must free quest & callback.
				If return true, don't free quest & callback.

			timeout in seconds.
		*/
		FPAnswerPtr sendQuest(FPQuestPtr quest, int timeout = 0);
		bool sendQuest(FPQuestPtr quest, AnswerCallback* callback, int timeout = 0);
		bool sendQuest(FPQuestPtr quest, std::function<void (FPAnswerPtr answer, int errorCode)> task, int timeout = 0);

		FPAnswerPtr sendQuest(FPQuestPtr quest, ConsistencySuccessCondition condition, int requiredCount = 0, int timeout = 0);
		bool sendQuest(FPQuestPtr quest, AnswerCallback* callback, ConsistencySuccessCondition condition, int requiredCount = 0, int timeout = 0);
		bool sendQuest(FPQuestPtr quest, std::function<void (FPAnswerPtr answer, int errorCode)> task, ConsistencySuccessCondition condition, int requiredCount = 0, int timeout = 0);
	};
	typedef std::shared_ptr<TCPConsistencyProxy> TCPConsistencyProxyPtr;

#### 构造函数

	TCPConsistencyProxy(ConsistencySuccessCondition condition, int requiredCount = 0, int64_t questTimeoutSeconds = -1);

**参数说明**

* **`ConsistencySuccessCondition condition`**

	一致性达成类型，请参见 [ConsistencySuccessCondition](#ConsistencySuccessCondition)。

* **`int requiredCount`**

	ConsistencySuccessCondition 为 CountedQuestsSuccess 时，指定的成功应答的数量标准。

	如果 ConsistencySuccessCondition 为其他字段，该参数忽略。

* **`int64_t questTimeoutSeconds`**

	Proxy 默认的请求超时时间。单位：秒。负值与0表示使用全局([ClientEngine](../core/ClientEngine.md))设置。

#### 成员函数

本文档仅列出基于基类所扩展的成员函数，其余成员函数请参考基类文档 [TCPProxyCore](TCPProxyCore.md)。

##### getClient

	TCPClientPtr getClient(const std::string& endpoint, bool connect);

获取集群中指定 endpint 对应的 [TCPClient](../core/TCPClient.md)。

**参数说明**

* **`const std::string& endpoint`**

	指定的 endpoint。

* **`bool connect`**

	返回的 [TCPClient](../core/TCPClient.md) 是否必须是处于已连接的状态。

##### sendQuest

* **标准接口**

		FPAnswerPtr sendQuest(FPQuestPtr quest, int timeout = 0);
		bool sendQuest(FPQuestPtr quest, AnswerCallback* callback, int timeout = 0);
		bool sendQuest(FPQuestPtr quest, std::function<void (FPAnswerPtr answer, int errorCode)> task, int timeout = 0);

	发送请求。一致性达成规则以创建 proxy 时确认的规则为准。如需指定不同的规则，请用扩展接口。

* **扩展接口**

		FPAnswerPtr sendQuest(FPQuestPtr quest, ConsistencySuccessCondition condition, int requiredCount = 0, int timeout = 0);
		bool sendQuest(FPQuestPtr quest, AnswerCallback* callback, ConsistencySuccessCondition condition, int requiredCount = 0, int timeout = 0);
		bool sendQuest(FPQuestPtr quest, std::function<void (FPAnswerPtr answer, int errorCode)> task, ConsistencySuccessCondition condition, int requiredCount = 0, int timeout = 0);

	发送请求。一致性达成规则以接口参数指定规则为准，忽略 proxy 创建时的一致性达成规则。

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

	**注意**

	当一致性达成时，仅集群中某一成功的应答会被作为本次一致性请求的应答而返回。

* **`std::function<void (FPAnswerPtr answer, int errorCode)> task`**

	异步请求的回调函数。

	**注意**

	+ 当一致性达成时，仅集群中某一成功的应答会被作为本次一致性请求的应答而返回。
	+ 当且仅当 `errorCode == FPNN_EC_OK` 时，`answer` 为业务正常应答；否则其它情况，如果 `answer` 不为 `nullptr`，则为 FPNN 异常应答。

		FPNN 异常应答请参考 [errorAnswer](../proto/FPWriter.md#errorAnswer)。

* **`ConsistencySuccessCondition condition`**

	一致性达成类型，请参见 [ConsistencySuccessCondition](#ConsistencySuccessCondition)。

* **`int requiredCount`**

	ConsistencySuccessCondition 为 CountedQuestsSuccess 时，指定的成功应答的数量标准。

	如果 ConsistencySuccessCondition 为其他字段，该参数忽略。

**返回值说明**

* **FPAnswerPtr**

	+ 对于 oneway 请求，FPAnswerPtr 的返回值**恒定为** `nullptr`。
	+ 对于 twoway 请求，FPAnswerPtr 可能为正常应答，也可能为异常应答。FPNN 异常应答请参考 [errorAnswer](../proto/FPWriter.md#errorAnswer)。
	+ 当一致性达成时，仅集群中某一成功的应答会被作为本次一致性请求的应答而返回。

* **bool**

	发送成功，返回 true；失败 返回 false。

	**注意**

	如果发送成功，`AnswerCallback* callback` 将不能再被复用，用户将无须处理 `callback` 对象的释放。SDK 会在合适的时候，调用 `delete` 操作进行释放；  
	如果返回失败，用户需要处理 `AnswerCallback* callback` 对象的释放。
