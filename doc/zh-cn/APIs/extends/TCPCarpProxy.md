## TCPCarpProxy

### 介绍

TCPCarpProxy 是以一致性哈希为访问方式的 TCP 集群代理对象，是 [TCPProxyCore](TCPProxyCore.md) 的子类。

### 命名空间

	namespace fpnn;

### TCPCarpProxy

	class TCPCarpProxy: public TCPProxyCore
	{
	public:
		TCPCarpProxy(int64_t questTimeoutSeconds = -1, uint32_t keymask = 0);
		virtual ~TCPCarpProxy() {}

		TCPClientPtr getClient(int64_t key, bool connect);
		TCPClientPtr getClient(const std::string& key, bool connect);

		/**
			All SendQuest():
				If return false, caller must free quest & callback.
				If return true, don't free quest & callback.

			timeout in seconds.
		*/
		FPAnswerPtr sendQuest(int64_t key, FPQuestPtr quest, int timeout = 0);
		bool sendQuest(int64_t key, FPQuestPtr quest, AnswerCallback* callback, int timeout = 0);
		bool sendQuest(int64_t key, FPQuestPtr quest, std::function<void (FPAnswerPtr answer, int errorCode)> task, int timeout = 0);

		FPAnswerPtr sendQuest(const std::string& key, FPQuestPtr quest, int timeout = 0);
		bool sendQuest(const std::string& key, FPQuestPtr quest, AnswerCallback* callback, int timeout = 0);
		bool sendQuest(const std::string& key, FPQuestPtr quest, std::function<void (FPAnswerPtr answer, int errorCode)> task, int timeout = 0);
	};
	typedef std::shared_ptr<TCPCarpProxy> TCPCarpProxyPtr;

#### 构造函数

	TCPCarpProxy(int64_t questTimeoutSeconds = -1, uint32_t keymask = 0);

**参数说明**

* **`int64_t questTimeoutSeconds`**

	Proxy 默认的请求超时时间。单位：秒。负值与0表示使用全局([ClientEngine](../core/ClientEngine.md))设置。

* **`uint32_t keymask`**

	一致性哈希使用的掩码设置。

#### 成员函数

本文档仅列出基于基类所扩展的成员函数，其余成员函数请参考基类文档 [TCPProxyCore](TCPProxyCore.md)。

##### getClient

	TCPClientPtr getClient(int64_t key, bool connect);
	TCPClientPtr getClient(const std::string& key, bool connect);

获取特定的 [TCPClient](../core/TCPClient.md)。

**参数说明**

* **`int64_t key`**

	一致性哈希的特征 key。

* **`const std::string& key`**

	一致性哈希的特征 key。

* **`bool connect`**

	返回的 [TCPClient](../core/TCPClient.md) 是否必须是处于已连接的状态。

##### sendQuest

	FPAnswerPtr sendQuest(int64_t key, FPQuestPtr quest, int timeout = 0);
	bool sendQuest(int64_t key, FPQuestPtr quest, AnswerCallback* callback, int timeout = 0);
	bool sendQuest(int64_t key, FPQuestPtr quest, std::function<void (FPAnswerPtr answer, int errorCode)> task, int timeout = 0);

	FPAnswerPtr sendQuest(const std::string& key, FPQuestPtr quest, int timeout = 0);
	bool sendQuest(const std::string& key, FPQuestPtr quest, AnswerCallback* callback, int timeout = 0);
	bool sendQuest(const std::string& key, FPQuestPtr quest, std::function<void (FPAnswerPtr answer, int errorCode)> task, int timeout = 0);

发送请求。

第一个和第四个声明为同步发送，后两个声明为异步发送。

**参数说明**

* **`int64_t key`**

	一致性哈希的特征 key。

* **`const std::string& key`**

	一致性哈希的特征 key。

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
