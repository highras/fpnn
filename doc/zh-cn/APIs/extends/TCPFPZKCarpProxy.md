## TCPFPZKCarpProxy

### 介绍

TCPCarpProxy 是以一致性哈希为访问方式，与 FPZK 服务联动的 TCP 集群代理对象，是 [TCPFPZKProxyCore](TCPFPZKProxyCore.md) 的子类。

### 命名空间

	namespace fpnn;

### TCPFPZKCarpProxy

	class TCPFPZKCarpProxy: public TCPFPZKProxyCore
	{
	public:
		TCPFPZKCarpProxy(FPZKClientPtr fpzkClient, const std::string& serviceName, const std::string& cluster = "", int64_t questTimeoutSeconds = -1, uint32_t keymask = 0);
		virtual ~TCPFPZKCarpProxy();

		TCPClientPtr getClient(int64_t key, bool connect);
		TCPClientPtr getClient(const std::string& key, bool connect);

		bool getClients(const std::set<int64_t>& keys, bool connect, std::map<TCPClientPtr, std::set<int64_t>>& result);
		bool getClients(const std::set<std::string>& keys, bool connect, std::map<TCPClientPtr, std::set<std::string>>& result);
		bool getClients(const std::vector<int64_t>& keys, bool connect, std::map<TCPClientPtr, std::vector<int64_t>>& result);
		bool getClients(const std::vector<std::string>& keys, bool connect, std::map<TCPClientPtr, std::vector<std::string>>& result);
		bool getClients(const std::vector<int64_t>& keys, bool connect, std::map<TCPClientPtr, std::set<int64_t>>& result);
		bool getClients(const std::vector<std::string>& keys, bool connect, std::map<TCPClientPtr, std::set<std::string>>& result);

		bool connectAll();
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
		std::map<std::string, TCPClientPtr> getAllClients();
		size_t count();
	};
	typedef std::shared_ptr<TCPFPZKCarpProxy> TCPFPZKCarpProxyPtr;

#### 构造函数

	TCPFPZKCarpProxy(FPZKClientPtr fpzkClient, const std::string& serviceName, const std::string& cluster = "", int64_t questTimeoutSeconds = -1, uint32_t keymask = 0);

**参数说明**

* **`FPZKClientPtr fpzkClient`**

	关联的 [FPZKClient](FPZKClient.md) 实例。

* **`const std::string& serviceName`**

	集群名称。

* **`const std::string& cluster`**

	集群二级分组名称。

* **`int64_t questTimeoutSeconds`**

	Proxy 默认的请求超时时间。单位：秒。负值与0表示使用全局([ClientEngine](../core/ClientEngine.md))设置。

* **`uint32_t keymask`**

	一致性哈希使用的掩码设置。

#### 成员函数

本文档仅列出基于基类所扩展的成员函数，其余成员函数请参考基类文档 [TCPFPZKProxyCore](TCPFPZKProxyCore.md) 及 [TCPProxyCore](TCPProxyCore.md)。


##### getClient

	TCPClientPtr getClient(int64_t key, bool connect);
	TCPClientPtr getClient(const std::string& key, bool connect);

获取特定的 [TCPClient](../CORE/TCPClient.md)。

**参数说明**

* **`int64_t key`**

	一致性哈希的特征 key。

* **`const std::string& key`**

	一致性哈希的特征 key。

* **`bool connect`**

	返回的 [TCPClient](../CORE/TCPClient.md) 是否必须是处于已连接的状态。

##### getClients

	bool getClients(const std::set<int64_t>& keys, bool connect, std::map<TCPClientPtr, std::set<int64_t>>& result);
	bool getClients(const std::set<std::string>& keys, bool connect, std::map<TCPClientPtr, std::set<std::string>>& result);
	bool getClients(const std::vector<int64_t>& keys, bool connect, std::map<TCPClientPtr, std::vector<int64_t>>& result);
	bool getClients(const std::vector<std::string>& keys, bool connect, std::map<TCPClientPtr, std::vector<std::string>>& result);
	bool getClients(const std::vector<int64_t>& keys, bool connect, std::map<TCPClientPtr, std::set<int64_t>>& result);
	bool getClients(const std::vector<std::string>& keys, bool connect, std::map<TCPClientPtr, std::set<std::string>>& result);

获取指定特征的服务对应的 [TCPClient](../CORE/TCPClient.md)。

**参数说明**

* **`const std::set<int64_t>& keys`** & **`const std::set<std::string>& keys`**

	一致性哈希的特征 key 集合。

* **`bool connect`**

	返回的 [TCPClient](../CORE/TCPClient.md) 是否必须是处于已连接的状态。

* **`std::map<TCPClientPtr, std::set<int64_t>>& result`**&**`std::map<TCPClientPtr, std::vector<int64_t>>& result`**

	获取到的 [TCPClient](../CORE/TCPClient.md) 和其对应的哈希特征集合。

**返值**

true 表示成功，false 表示空集，或者无数据。

##### connectAll

	bool connectAll();

使集群所有服务对应的 [TCPClient](../CORE/TCPClient.md) 进入已连接状态。

##### SendQuest

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

##### getAllClients

	std::map<std::string, TCPClientPtr> getAllClients();

获取当前**已创建**的有效的 [TCPClient](../CORE/TCPClient.md) 集合。

**注意**

+ 该接口为特定目的使用。
+ 此时获取到的 [TCPClient](../CORE/TCPClient.md) 仅为被访问过的服务对应的 [TCPClient](../CORE/TCPClient.md) 的集合，而**非全部**服务实例对应的 [TCPClient](../CORE/TCPClient.md) 的集合。若须获取全部服务实例对应的 [TCPClient](../CORE/TCPClient.md) 的集合，须前置调用 [connectAll](#connectAll) 方法，强制初始化所有服务实例对应的 [TCPClient](../CORE/TCPClient.md)。

##### count

	size_t count();

获取当前**已创建**的有效的 [TCPClient](../CORE/TCPClient.md) 的数量。

**注意**

+ 该接口为特定目的使用。
+ 该接口等价于获取已访问过的服务实例的数量，而非全部服务实例的数量。