## TCPBroadcastProxy

### 介绍

TCPBroadcastProxy 是以集群广播为访问方式的 TCP 集群代理对象，是 [TCPProxyCore](TCPProxyCore.md) 的子类。

### 命名空间

	namespace fpnn;

### BroadcastAnswerCallback

	struct BroadcastAnswerCallback
	{
		virtual ~BroadcastAnswerCallback();

		virtual void onAnswer(const std::string& endpoint, FPAnswerPtr) {}
		virtual void onException(const std::string& endpoint, FPAnswerPtr, int errorCode) {}
		virtual void onCompleted(std::map<std::string, FPAnswerPtr>& answerMap) {}
	};

向集群异步广播请求时，请求的回调对象。

#### 接口函数

##### onAnswer

	virtual void onAnswer(const std::string& endpoint, FPAnswerPtr);

每收到集群中一个服务实例的正常响应时，会触发该接口函数。

**参数说明**

* **`const std::string& endpoint`**

	响应的服务实例的 endpoint。

* **`FPAnswerPtr`**

	响应的数据。请参见 [FPAnswer](../proto/FPAnswer.md)。

##### onException

	virtual void onException(const std::string& endpoint, FPAnswerPtr, int errorCode);

每收到集群中一个服务实例的异常响应时，会触发该接口函数。

**参数说明**

* **`const std::string& endpoint`**

	异常响应的服务实例的 endpoint。

* **`FPAnswerPtr`**

	异常响应的数据。

	**注意**

	+ 如果遇到连接中断/结束，连接已关闭，超时等情况，`FPAnswerPtr` 将为 `nullptr`。
	+ FPNN 异常应答请参考 [errorAnswer](../proto/FPWriter.md#errorAnswer)。

* **`int errorCode`**

	异常响应的错误代码。

##### onCompleted

	virtual void onCompleted(std::map<std::string, FPAnswerPtr>& answerMap);

当集群广播完成时，将触发该接口。

**参数说明**

* **`std::map<std::string, FPAnswerPtr>& answerMap`**

	集群中各个服务实例响应的应答数据。

	键为各个服务的 endpoint，值为对应服务实例的回应。

	**注意**

	回应包含异常回应。对于回应为 nullptr 的异常回应，此处会将该异常重新包装成对应的异常应答对象。

### TCPBroadcastProxy

	class TCPBroadcastProxy: virtual public TCPProxyCore
	{
	public:
		TCPBroadcastProxy(int64_t questTimeoutSeconds = -1);
		virtual ~TCPBroadcastProxy() {}

		void exceptSelf(const std::string& endpoint);
		virtual bool empty();

		TCPClientPtr getClient(const std::string& endpoint, bool connect);

		std::map<std::string, FPAnswerPtr> sendQuest(FPQuestPtr quest, int timeout = 0);
		bool sendQuest(FPQuestPtr quest, BroadcastAnswerCallback* callback, int timeout = 0);
		bool sendQuest(FPQuestPtr quest, std::function<void (const std::string& endpoint, FPAnswerPtr answer, int errorCode)> task, int timeout = 0);
		bool sendQuest(FPQuestPtr quest, std::function<void (std::map<std::string, FPAnswerPtr>& answerMap)> task, int timeout = 0);
	};
	typedef std::shared_ptr<TCPBroadcastProxy> TCPBroadcastProxyPtr;

#### 构造函数

	TCPBroadcastProxy(int64_t questTimeoutSeconds = -1);

**参数说明**

* **`int64_t questTimeoutSeconds`**

	Proxy 默认的请求超时时间。单位：秒。负值与0表示使用全局([ClientEngine](../core/ClientEngine.md))设置。

#### 成员函数

本文档仅列出基于基类所扩展的成员函数，其余成员函数请参考基类文档 [TCPProxyCore](TCPProxyCore.md)。

##### exceptSelf

	void exceptSelf(const std::string& endpoint);

在集群中排除自身。

如果集群包含自身服务实例，且希望广播时，自身服务实例不收到自身发出的广播，则调用该函数，移除自身服务实例的 endpoint。

##### empty

	virtual bool empty();

判断集群是否为空。

##### getClient

	TCPClientPtr getClient(const std::string& endpoint, bool connect);

获取集群中指定 endpint 对应的 [TCPClient](../core/TCPClient.md)。

**参数说明**

* **`const std::string& endpoint`**

	指定的 endpoint。

* **`bool connect`**

	返回的 [TCPClient](../core/TCPClient.md) 是否必须是处于已连接的状态。


##### sendQuest

	/**
		All SendQuest():
			If return false, caller must free quest & callback.
			If return true, don't free quest & callback.

		timeout in seconds.

	  * For sync function sendQuest(FPQuestPtr, int):
		If one way quest, returned map always empty.
		If two way quest, returned map included all endpoints' answer. If no available client, returned map is empty.
	*/
	std::map<std::string, FPAnswerPtr> sendQuest(FPQuestPtr quest, int timeout = 0);
	bool sendQuest(FPQuestPtr quest, BroadcastAnswerCallback* callback, int timeout = 0);
	bool sendQuest(FPQuestPtr quest, std::function<void (const std::string& endpoint, FPAnswerPtr answer, int errorCode)> task, int timeout = 0);
	bool sendQuest(FPQuestPtr quest, std::function<void (std::map<std::string, FPAnswerPtr>& answerMap)> task, int timeout = 0);

发送请求。

第一个声明为同步发送，其余声明为异步发送。

**注意**

TCPBroadcastProxy sendQuest 的异步接口，会拒绝发送 oneway 请求。oneway 请求，请使用同步接口发送。

**参数说明**

* **`FPQuestPtr quest`**

	请求数据对象。具体请参见 [FPQuest](../proto/FPQuest.md)。

* **`int timeout`**

	本次请求的超时设置。单位：秒。

	**注意**

	如果 `timeout` 为 0，表示使用 Proxy/Client 实例当前的请求超时设置。  
	如果 Proxy/Client 实例当前的请求超时设置为 0，则使用 ClientEngine 的请求超时设置。

* **`BroadcastAnswerCallback* callback`**

	异步广播请求的回调对象。具体请参见 [BroadcastAnswerCallback](#BroadcastAnswerCallback)。

* **`std::function<void (const std::string& endpoint, FPAnswerPtr answer, int errorCode)> task`**

	异步广播请求的的回调函数。广播中，每个目标服务实例回应时将触发该回调函数。

	**参数说明**

	* **`const std::string& endpoint`**

		响应的服务实例的 endpoint。

	* **`FPAnswerPtr answer`**

		服务实力回应的数据。

		**注意**

		+ 如果遇到连接中断/结束，连接已关闭，超时等情况，`answer` 将为 `nullptr`。
		+ 当且仅当 `errorCode == FPNN_EC_OK` 时，`answer` 为业务正常应答；否则其它情况，如果 `answer` 不为 `nullptr`，则为 FPNN 异常应答。
		+ FPNN 异常应答请参考 [errorAnswer](../proto/FPWriter.md#errorAnswer)。

	* **`int errorCode`**

		异常响应的错误代码。


* **`std::function<void (std::map<std::string, FPAnswerPtr>& answerMap)> task`**

	异步广播请求的的回调函数。广播全部完成时，将触发该回调函数。

	**参数说明**

	* **`std::map<std::string, FPAnswerPtr>& answerMap`**

		集群中各个服务实例响应的应答数据。

		键为各个服务的 endpoint，值为对应服务实例的回应。

		**注意**

		回应包含异常回应。对于回应为 nullptr 的异常回应，此处会将该异常重新包装成对应的异常应答对象。


**返回值说明**

* **`std::map<std::string, FPAnswerPtr>`**

	集群中各个服务实例响应的应答数据。

	键为各个服务的 endpoint，值为对应服务实例的回应。

	**注意**

	+ 如果是 oneway 请求，则为空集合。
	+ 回应包含异常回应。对于回应为 nullptr 的异常回应，此处会将该异常重新包装成对应的异常应答对象。

* **bool**

	发送成功，返回 true；失败 返回 false。

	**注意**

	如果发送成功，`BroadcastAnswerCallback* callback` 将不能再被复用，用户将无须处理 `callback` 对象的释放。SDK 会在合适的时候，调用 `delete` 操作进行释放；  
	如果返回失败，用户需要处理 `BroadcastAnswerCallback* callback` 对象的释放。
