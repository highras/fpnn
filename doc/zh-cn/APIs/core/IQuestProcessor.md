## IQuestProcessor

### 介绍

FPNN 事件处理对象基类。

IQuestProcessor 处理链接建立和断开事件，以及对端请求。  
对于服务器，对端请求即客户端的访问请求；对于客户端，对端请求即 Server Push 请求。

**注意：请勿使用非文档化的 API。**  
非文档化的 API，或为内部使用，或因历史原因遗留，或为将来设计，后续版本均可能存在变动。

### 命名空间

	namespace fpnn;

### 关键定义

	class IQuestProcessor
	{
	protected:
		enum MethodAttribute { EncryptOnly = 0x1, PrivateIPOnly = 0x2 };	//-- Only for methods attributes.

	public:
		IQuestProcessor();
		virtual ~IQuestProcessor();

	protected:
		/*
			The following four methods ONLY can be called in server interfaces function which is called by FPNN framework.
		*/
		bool sendAnswer(FPAnswerPtr answer);
		IAsyncAnswerPtr genAsyncAnswer();

	protected:
		inline FPAnswerPtr illegalAccessRequest(const std::string& method_name, const FPQuestPtr quest, const ConnectionInfo& connInfo);

	public:
		QuestSenderPtr genQuestSender(const ConnectionInfo& connectionInfo);

		/**
			All SendQuest() & sendQuestEx():
				If return false, caller must free quest & callback.
				If return true, don't free quest & callback.

				timeout for sendQuest() is in seconds, timeoutMsec for sendQuestEx() is in milliseconds.
		*/
		virtual FPAnswerPtr sendQuest(FPQuestPtr quest, int timeout = 0);
		virtual bool sendQuest(FPQuestPtr quest, AnswerCallback* callback, int timeout = 0);
		virtual bool sendQuest(FPQuestPtr quest, std::function<void (FPAnswerPtr answer, int errorCode)> task, int timeout = 0);

		virtual FPAnswerPtr sendQuestEx(FPQuestPtr quest, bool discardable, int timeoutMsec = 0);
		virtual bool sendQuestEx(FPQuestPtr quest, AnswerCallback* callback, bool discardable, int timeoutMsec = 0);
		virtual bool sendQuestEx(FPQuestPtr quest, std::function<void (FPAnswerPtr answer, int errorCode)> task, bool discardable, int timeoutMsec = 0);

		/*===============================================================================
		  Event Hook. (Common)
		=============================================================================== */
	public:
		virtual void connected(const ConnectionInfo&) {}
		virtual void connectionWillClose(const ConnectionInfo& connInfo, bool closeByError);
		virtual FPAnswerPtr unknownMethod(const std::string& method_name, const FPReaderPtr args, const FPQuestPtr quest, const ConnectionInfo& connInfo);

		/*===============================================================================
		  Event Hook. (Server Only)
		=============================================================================== */
		virtual void start(){}					//-- ONLY used by server
		virtual void serverWillStop() {}		//-- ONLY used by server
		virtual void serverStopped() {}			//-- ONLY used by server

		virtual bool status() { return true; }	//-- ONLY used by server
		virtual std::string infos() { return "{}"; }		//-- ONLY used by server
		virtual void tune(const std::string& key, std::string& value) {}	//-- ONLY used by server
	};

	typedef std::shared_ptr<IQuestProcessor> IQuestProcessorPtr;

	#define QuestProcessorClassPrivateFields(processor_name)

	#define QuestProcessorClassBasicPublicFuncs

### 构造函数

	IQuestProcessor();

### 成员函数

#### sendAnswer

	bool sendAnswer(FPAnswerPtr answer);

在请求处理函数内为 twoway 类型的请求，提前发送应答。  
对于服务端，为在客户端请求处理函数内，对客户 twoway 类型的请求，提前发送应答。  
对于客户端，为在 Server Push 的请求处理函数内，对服务器 twoway 类型的请求，提前发送应答。

**注意**

+ 调用该函数后，调用的请求处理函数必须返回 `nullptr`，否则会触发多次应答的异常日志。
+ 该函数必须在请求处理函数内(或请求处理函数返回前，同一线程上，被请求处理函数调用的其他函数中)被调用，否则无效。
+ 在请求处理函数内，该函数只能调用一次，后续调用无效。

**返回值说明**

如果发送成功，返回 true；如果发送失败，或者是 oneway 类型消息，或者前面已经调用过该成员函数，则返回 false。

#### genAsyncAnswer

	IAsyncAnswerPtr genAsyncAnswer();

在请求处理函数内，为当前twoway 类型的请求生成异步返回器。  
对于服务端，为在客户端请求处理函数内，对客户 twoway 类型的请求生成异步返回器。  
对于客户端，为在 Server Push 的请求处理函数内，对服务器 twoway 类型的请求生成异步返回器。

异步返回器请参见 [IAsyncAnswer](IAsyncAnswer.md)。

**注意**

+ 调用该函数后，请求处理函数必须返回 `nullptr`。
+ 该函数必须在请求处理函数内(或请求处理函数返回前，同一线程上，被请求处理函数调用的其他函数中)被调用，否则无效。
+ 在请求处理函数内，该函数只能调用一次，后续调用无效。

#### genQuestSender

	QuestSenderPtr genQuestSender(const ConnectionInfo& connectionInfo);

使用连接信息对象的**原本**实例，生成请求发送对象。

请求发送对象请参见 [QuestSender](QuestSender.md)。  
ConnectionInfo 请参见 [ConnectionInfo](ConnectionInfo.md)。

**注意**

使用连接信息对象的**副本**实例无效。

#### illegalAccessRequest
	
	inline FPAnswerPtr illegalAccessRequest(const std::string& method_name, const FPQuestPtr quest, const ConnectionInfo& connInfo);

当发生非法访问时，将触发该函数。

**关于非法访问**

+ 如果注册请求处理函数时，指定了 [加密访问](#MethodAttribute)，那非加密链接访问该接口将会触发该函数。
+ 如果注册请求处理函数时，指定了 [内网访问](#MethodAttribute)，那非内网链接访问该接口将会触发该函数。
+ 对于服务器，如果服务器启动了白名单（请参见 [TCPEpollServer](TCPEpollServer.md) 及 [UDPEpollServer](UDPEpollServer.md) 白名单相关接口），则来自于白名单之外的访问，**不会**触发该函数。

**参数说明**

* **`const std::string& method_name`**

	非法请求请求的接口名/方法名。

* **`const FPQuestPtr quest`**

	非法请求所对应的 [FPQuest](../proto/FPQuest.md) 对象。

* **`const ConnectionInfo& connInfo`**

	连接信息对象 [ConnectionInfo](ConnectionInfo.md)（**实例原本**）。


#### sendQuest

	virtual FPAnswerPtr sendQuest(FPQuestPtr quest, int timeout = 0);
	virtual bool sendQuest(FPQuestPtr quest, AnswerCallback* callback, int timeout = 0);
	virtual bool sendQuest(FPQuestPtr quest, std::function<void (FPAnswerPtr answer, int errorCode)> task, int timeout = 0);

在请求处理函数内，向对端发送请求的便捷函数。  
对于服务端，为在客户端请求处理函数内，向客户端发送请求。  
对于客户端，为在 Server Push 的请求处理函数内，向服务器发送请求。

**注意**

该函数必须在请求处理函数内(或请求处理函数返回前，同一线程上，被请求处理函数调用的其他函数中)被调用，否则无效。

**参数说明**

* **`FPQuestPtr quest`**

	请求数据对象。具体请参见 [FPQuest](../proto/FPQuest.md)。

	**注意**

	对于 UDP 连接，oneway 消息被视为可丢弃消息；twoway 消息被视为不可丢弃消息。如果需要显式指定消息的可丢弃属性，请使用 [sendQuestEx](#sendQuestEx) 接口。

* **`int timeout`**

	本次请求的超时设置。单位：秒。

	**注意**

	如果 `timeout` 为 0，表示使用 ClientEngine 的请求超时设置。

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

在请求处理函数内，向对端发送请求的便捷函数扩展版。  
对于服务端，为在客户端请求处理函数内，向客户端发送请求。  
对于客户端，为在 Server Push 的请求处理函数内，向服务器发送请求。

**注意**

该函数必须在请求处理函数内(或请求处理函数返回前，同一线程上，被请求处理函数调用的其他函数中)被调用，否则无效。

**参数说明**

* **`FPQuestPtr quest`**

	请求数据对象。具体请参见 [FPQuest](../proto/FPQuest.md)。

* **`bool discardable`**

	对于 UDP 连接，数据包是否可丢弃/是否是必达消息。TCP 连接忽略该参数。

* **`int timeoutMsec`**

	本次请求的超时设置。单位：**毫秒**。

	**注意**

	如果 `timeoutMsec` 为 0，表示使用 ClientEngine 的请求超时设置。

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

#### connected

	virtual void connected(const ConnectionInfo&);

链接建立事件。

**注意**

链接建立事件函数返回后，该连接上数据才可以正常发送。

**参数说明**

* **`const ConnectionInfo&`**

	连接信息对象 [ConnectionInfo](ConnectionInfo.md)（**实例原本**）。


#### connectionWillClose

	virtual void connectionWillClose(const ConnectionInfo& connInfo, bool closeByError);

链接即将关闭事件。

**注意**

链接即将关闭事件函数返回后，socket 进入失效，或者关闭状态。但因为**操作系统线程调度**的原因，链接即将关闭事件被触发时，可能会有同一 socket 上的请求正在处理。

**参数说明**

* **`const ConnectionInfo& connInfo`**

	连接信息对象 [ConnectionInfo](ConnectionInfo.md)（**实例原本**）。

* **`bool closeByError`**

	连接正常关闭，还是(因错误)异常关闭。

#### start

	virtual void start();

服务器启动事件。  
当该函数返回后，服务器才正式对外提供服务。

#### serverWillStop

	virtual void serverWillStop();

服务器即将停止事件。  
当该函数返回后，服务器将终止收发信息。

#### serverStopped

	virtual void serverStopped();

服务器完全停止事件。

#### status

	virtual bool status();

服务器状态接口。业务服务重载该接口，以表示当前服务是否可用，或者是否可以响应外部请求。

FPNN 内置接口 `*status` 被调用时，将调用该接口，并返回该接口返回值。  
`*status` 接口请参见 [FPNN 内置接口](../../fpnn-build-in-methods.md#status)。

#### infos

	virtual std::string infos();

服务器信息接口。业务服务重载该接口，以返回当前业务状态。返回数据为 JSON 对象字符串。

FPNN 内置接口 `*infos` 被调用时，将调用该接口，并将该接口返回的 JSON 数据作为 `*infos` 接口返回的 JSON 数据中， `APP.status` 字段的内容。  
`*infos` 接口请参见 [FPNN 内置接口](../../fpnn-build-in-methods.md#infos)。

#### tune

	virtual void tune(const std::string& key, std::string& value);

服务器参数动态调节接口。业务服务可以通过重载该接口，获取 `*tune` 接口发送的业务参数调整数据。

`*tune` 接口请参见 [FPNN 内置接口](../../fpnn-build-in-methods.md#tune)。

#### unknownMethod

	virtual FPAnswerPtr unknownMethod(const std::string& method_name, const FPReaderPtr args, const FPQuestPtr quest, const ConnectionInfo& connInfo);

非法请求处理函数。

**参数说明**

* **`const std::string& method_name`**

	非法请求请求的接口名/方法名。

* **`const FPReaderPtr args`**

	非法请求所含数据的读取器。参见 [FPQReader](../proto/FPReader.md#FPQReader)。

* **`const FPQuestPtr quest`**

	非法请求所对应的 [FPQuest](../proto/FPQuest.md) 对象。

* **`const ConnectionInfo& connInfo`**

	连接信息对象 [ConnectionInfo](ConnectionInfo.md)（**实例原本**）。

#### registerMethod

	inline void registerMethod(const std::string& method_name, MethodFunc func, uint32_t attributes = 0);

注册请求处理函数。

**参数说明**

* **`const std::string& method_name`**

	注册的接口名称/方法名称。

* **`MethodFunc func`**

	注册的处理函数。

	**注意**

	+ 处理函数必须是**非静态**的成员函数。
	+ 处理函数签名参见 [请求处理函数原型](#请求处理函数原型)。

* **`uint32_t attributes`**

	请求处理函数访问属性，请参见 [MethodAttribute](#MethodAttribute)。

#### MethodAttribute

	enum MethodAttribute { EncryptOnly = 0x1, PrivateIPOnly = 0x2 };

请求处理函数访问属性。

+ EncryptOnly

	该接口只有加密链接可以访问，不然会触发非法访问，并触发 [illegalAccessRequest](#illegalAccessRequest) 函数。

+ PrivateIPOnly

	该接口只有内网链接可以访问，不然会触发非法访问，并触发 [illegalAccessRequest](#illegalAccessRequest) 函数。


#### 请求处理函数原型

	FPAnswerPtr MethodFunc(const FPReaderPtr, const FPQuestPtr, const ConnectionInfo&);

请求处理函数签名原型。

**参数说明**

* **`const FPReaderPtr`**

	Server Push 请求所含数据的读取器。参见 [FPQReader](../proto/FPReader.md#FPQReader)。

* **`const FPQuestPtr`**

	Server Push 请求所对应的 [FPQuest](../proto/FPQuest.md) 对象。

* **`const ConnectionInfo&`**

	连接信息对象 [ConnectionInfo](ConnectionInfo.md)（**实例原本**）。

### 宏定义

#### QuestProcessorClassPrivateFields

	#define QuestProcessorClassPrivateFields(processor_name)

在每一个 IQuestProcessor 的子类中，必须在**私有域**里声明的宏。

负责给 IQuestProcessor 的子类添加基类接口需要的成员对象。

**参数说明**

* **`processor_name`**

	IQuestProcessor 子类类名。

#### QuestProcessorClassBasicPublicFuncs

	#define QuestProcessorClassBasicPublicFuncs

在每一个 IQuestProcessor 的子类中，必须在**公有域**里声明的宏。

负责给 IQuestProcessor 的子类添加工具接口，和非文档化的基类虚方法的实现。
