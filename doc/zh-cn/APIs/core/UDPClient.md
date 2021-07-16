## UDPClient

### 介绍

UDP 客户端。[Client](Client.md) 的子类。

UDP 客户端内部实现了可靠 UDP 连接，可混合发送可靠和不可靠数据。

**注意：请勿使用非文档化的 API。**  
非文档化的 API，或为内部使用，或因历史原因遗留，或为将来设计，后续版本均可能存在变动。

### 命名空间

	namespace fpnn;

### 关键定义

	class UDPClient: public Client, public std::enable_shared_from_this<UDPClient>
	{
	public:
		virtual ~UDPClient() { close(); }

		virtual bool connect();
		virtual void close();

		inline static UDPClientPtr createClient(const std::string& host, int port, bool autoReconnect = true);
		inline static UDPClientPtr createClient(const std::string& endpoint, bool autoReconnect = true);

		//-- Send Quest
		//-- Timeout in Seconds
		//-- Oneway will deal as discardable, towway will deal as reliable.
		virtual FPAnswerPtr sendQuest(FPQuestPtr quest, int timeout = 0);
		virtual bool sendQuest(FPQuestPtr quest, AnswerCallback* callback, int timeout = 0);
		virtual bool sendQuest(FPQuestPtr quest, std::function<void (FPAnswerPtr answer, int errorCode)> task, int timeout = 0);

		//-- Timeout in milliseconds
		virtual FPAnswerPtr sendQuestEx(FPQuestPtr quest, bool discardable, int timeoutMsec = 0);
		virtual bool sendQuestEx(FPQuestPtr quest, AnswerCallback* callback, bool discardable, int timeoutMsec = 0);
		virtual bool sendQuestEx(FPQuestPtr quest, std::function<void (FPAnswerPtr answer, int errorCode)> task, bool discardable, int timeoutMsec = 0);

		void keepAlive();
		void setUntransmittedSeconds(int untransmittedSeconds);		//-- 0: using default; -1: disable.
		void setMTU(int MTU);			//-- Please called before connect() or sendQuest().
	};


	typedef std::shared_ptr<UDPClient> UDPClientPtr;

### 创建与构造

UDPClient 的构造函数为私有成员，无法直接调用。请使用静态成员函数

	inline static UDPClientPtr UDPClient::createClient(const std::string& host, int port, bool autoReconnect = true);
	inline static UDPClientPtr UDPClient::createClient(const std::string& endpoint, bool autoReconnect = true);

或基类静态成员函数

	static UDPClientPtr Client::createUDPClient(const std::string& host, int port, bool autoReconnect = true);
	static UDPClientPtr Client::createUDPClient(const std::string& endpoint, bool autoReconnect = true);

创建。

**参数说明**

* **`const std::string& host`**

	服务器 IP 或者域名。

* **`int port`**

	服务器端口。

* **`const std::string& endpoint`**

	服务器 endpoint。

* **`bool autoReconnect`**

	是否自动重连。

### 成员函数

本文档仅列出基于基类所扩展的成员函数，其余成员函数请参考基类文档 [Client](Client.md)。

#### sendQuest

	virtual FPAnswerPtr sendQuest(FPQuestPtr quest, int timeout = 0);
	virtual bool sendQuest(FPQuestPtr quest, AnswerCallback* callback, int timeout = 0);
	virtual bool sendQuest(FPQuestPtr quest, std::function<void (FPAnswerPtr answer, int errorCode)> task, int timeout = 0);

发送请求。

**注意**

对于 UDP 连接，oneway 消息被视为可丢弃消息；twoway 消息被视为不可丢弃消息。如果需要显式指定消息的可丢弃属性，请使用 [sendQuestEx](#sendQuestEx) 接口。

**参数说明**

* **`FPQuestPtr quest`**

	请求数据对象。具体请参见 [FPQuest](../proto/FPQuest.md)。

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

#### sendQuestEx

	virtual FPAnswerPtr sendQuestEx(FPQuestPtr quest, bool discardable, int timeoutMsec = 0);
	virtual bool sendQuestEx(FPQuestPtr quest, AnswerCallback* callback, bool discardable, int timeoutMsec = 0);
	virtual bool sendQuestEx(FPQuestPtr quest, std::function<void (FPAnswerPtr answer, int errorCode)> task, bool discardable, int timeoutMsec = 0);

发送请求。

**参数说明**

* **`FPQuestPtr quest`**

	请求数据对象。具体请参见 [FPQuest](../proto/FPQuest.md)。

* **`bool discardable`**

	数据包是否可丢弃/是否是必达消息。

* **`int timeoutMsec`**

	本次请求的超时设置。单位：**毫秒**。

	**注意**

	如果 `timeoutMsec` 为 0，表示使用 Client 实例当前的请求超时设置。  
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

#### keepAlive

	void keepAlive();

开启连接自动保活/保持连接。

#### setUntransmittedSeconds

	void setUntransmittedSeconds(int untransmittedSeconds);		//-- 0: using default; -1: disable.

设置连接 idle 超时时间。单位：秒。默认：60 秒。`0` 表示使用默认值，`-1` 表示取消 idle 超时设置，即永远不会因为 idle 导致链接关闭。

#### setMTU

	void setMTU(int MTU); { _MTU = MTU; }		//-- Please called before connect() or sendQuest().

设置当前连接 MTU 大小。默认：对于内网地址为 1500 字节，外网地址为 576 字节。

**注意**

此设置，只有在**任何连接和发送操作**调用前调用，才会生效。否则会使用默认 MTU 大小，且不可更改。

