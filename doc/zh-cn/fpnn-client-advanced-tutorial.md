# FPNN 客户端高级使用向导

[TOC]

## 请求超时

FPNN 中，客户端侧一共存在3种级别请求超时。分别是 [ClientEngine](APIs/core/ClientEngine.md) 中配置的全局请求超时，[Client][] 实例的请求超时，[sendQuest][] 发送接口的请求超时。  
如果 [sendQuest][] 的请求超时为 0，则应用 [Client][] 实例的请求超时；  
如果 [Client][] 实例的请求超时设置为 0，则应用 [ClientEngine](APIs/core/ClientEngine.md) 中配置的全局请求超时。

配置 [Client][] 实例的请求超时：

	void setQuestTimeout(int64_t seconds);

注：配置类的接口非线程安全，需要在收发数据前，配置完成。

配置 [ClientEngine](APIs/core/ClientEngine.md) 中配置的全局请求超时：

	static void ClientEngine::setQuestTimeout(int64_t seconds);

或通过配置文件配置

	FPNN.client.quest.timeout = <number>


## 链接保活

FPNN 中，链接保活可分别通过 [TCPClient][] 或 [UDPClient][] 的 `keepAlive` 接口启用。

	void keepAlive();

链接保活一旦启用后，不可禁用。

对于 UDP 客户端，链接保活无额外配置项目；  
对于 TCP 客户端，链接保活存在以下三个配置项目：

	void setKeepAlivePingTimeout(int seconds);
	void setKeepAliveInterval(int seconds);
	void setKeepAliveMaxPingRetryCount(int count);

+ `setKeepAlivePingTimeout`

	设置链接活性探测的请求超时时间，详情可参见：[setKeepAlivePingTimeout](APIs/core/TCPClient.md#setKeepAlivePingTimeout)。

+ `setKeepAliveInterval`

	设置链接活性探测间隔时间，详情可参见：[setKeepAliveInterval](APIs/core/TCPClient.md#setKeepAliveInterval)。

	注意，该间隔时间为动态间隔，是指据上次数据接收到的时间，而非两次探测之间的间隔时间。

+ `setKeepAliveMaxPingRetryCount`

	设置链接活性探测失败时的重试次数，详情可参见：[setKeepAliveMaxPingRetryCount](APIs/core/TCPClient.md#setKeepAliveMaxPingRetryCount)。

此外，对于所有 TCP 客户端链接，可通过以下配置，**强制**并**隐式**地启动链接保活：

	FPNN.client.keepAlive.defaultEnable = true

具体可参见 [FPNN 标准配置模版](../conf.template) 中，“TCP 保活配置”一节。

## 日志配置

如果是独立的客户端程序，未和服务器一同运行(在同一进程内)，则框架默认会忽略 DEBUG、INFO、WARN 三个级别的日志，并将 ERROR 及 FATAL 级别的日志直接输出至显示终端。

配置日志模块，须根据需要，调用以下接口配置：

	static FPLogBasePtr FPLog::init(std::ostream& out, const std::string& route, const std::string& level = "ERROR", const std::string& serverName = "unknown", int32_t pid = 0, size_t maxQueueSize = FPLOG_DEFAULT_MAX_QUEUE_SIZE);

    static FPLogBasePtr FPLog::init(const std::string& endpoint, const std::string& route, const std::string& level = "ERROR", const std::string& serverName = "unknown", int32_t pid = 0, size_t maxQueueSize = FPLOG_DEFAULT_MAX_QUEUE_SIZE);

    static void FPLog::setLevel(const std::string& logLevel);

详情请参见 [init](APIs/base/FPLog.md#init) 及 [setLevel](APIs/base/FPLog.md#setLevel)。


## 链接建立与关闭事件

处理链接建立事件，须要向 [Client][] 设置 [IQuestProcessor][] 事件处理对象的具体实例。 

链接建立事件须重载 [IQuestProcessor][] 的 `connected` 接口：

	virtual void connected(const ConnectionInfo&);

详情请参见 [connected](APIs/core/IQuestProcessor.md#connected)。

链接关闭事件须重载 [IQuestProcessor][] 的  `connectionWillClose` 接口：

	virtual void connectionWillClose(const ConnectionInfo& connInfo, bool closeByError);

详情请参见 [connectionWillClose](APIs/core/IQuestProcessor.md#connectionWillClose)。


[Client][] 设置 [IQuestProcessor][] 事件处理对象的接口：

	void setQuestProcessor(IQuestProcessorPtr processor);

详情请参见 [setQuestProcessor](APIs/core/Client.md#setQuestProcessor)。

注：[Client][] 的配置类接口非线程安全，需要在数据收发前配置完成。


## 加密链接

目前仅 TCP 支持加密链接，UDP 需要等待后续版本支持。

[TCPClient][] 支持两种加密方式：SSL/TLS 加密和 FPNN 加密。

SSL/TLS 加密的接口为：

	bool enableSSL(bool enable = true);

FPNN 加密分为包加密和流加密两种模式。相关接口为：

	void enableEncryptor(const std::string& curve, const std::string& peerPublicKey, bool packageMode = true, bool reinforce = false);
	bool enableEncryptorByDerData(const std::string &derData, bool packageMode = true, bool reinforce = false);
	bool enableEncryptorByPemData(const std::string &PemData, bool packageMode = true, bool reinforce = false);
	bool enableEncryptorByDerFile(const char *derFilePath, bool packageMode = true, bool reinforce = false);
	bool enableEncryptorByPemFile(const char *pemFilePath, bool packageMode = true, bool reinforce = false);

具体参数为：

+ curve 为服务器采用的椭圆曲线名称，有四个选项：

	secp192r1、secp224r1、secp256r1、secp256k1。

+ peerPublicKey 为服务器的公钥。该参数要求传入二进制数据，非 base64 或者 hex 之后的可视数据。
+ packageMode 为加密模式。true 表示采用包加密模式，false 表示采用流加密模式。
+ reinforce 为加密强度。true 表示采用 256 bits 秘钥，false 表示采用 128 位秘钥。

**注意**：在同一连接上，SSL/TLS 与 FPNN 自身的加密不可同时开启。为了防止无谓的消耗系统资源，FPNN 不支持冗余的加密。

注：[TCPClient][] 的配置类接口非线程安全，需要在数据收发前配置完成。




## Server Push

Server Push 需要处理服务端发送的请求，并根据请求类型进行回应。因此客户端侧要启用 Server Push 的功能，需要三个步骤：

1. 实现 Server Push 的请求处理接口：

	实现 Server Push 的请求处理接口与服务端侧实现服务接口的方法完全一致，因此详细内容可以参考 [“FPNN 服务端基础使用向导” - “2. 添加接口方法”](fpnn-server-basic-tutorial.md#2-添加接口方法)。

	以下为简要介绍：

	Server Push 的请求处理接口格式均统一为：

		FPAnswerPtr method_name(const FPReaderPtr args, const FPQuestPtr quest, const ConnectionInfo& ci);

	开发中用实际接口对应的函数名称替换 method_name 即可。

	oneway 类型的接口/请求须返回 `nullptr`，twoway 类型的接口/请求，在没有使用[提前返回](#Server-Push-中的提前返回)或者[异步返回](#Server-Push-中的异步返回)时，须返回有效的 [FPAnswerPtr](APIs/proto/FPAnswer.md)，在使用[提前返回](#Server-Push-中的提前返回)或者[异步返回](#Server-Push-中的异步返回)后，须返回 `nullptr`。


2. 向 [IQuestProcessor][] 注册 Server Push 的请求处理接口：

	注册 Server Push 的请求处理接口与服务端侧注册服务接口的方法完全一致，因此详细内容可以参考 [“FPNN 服务端基础使用向导” - “2.4. 接口注册”](fpnn-server-basic-tutorial.md#2-添加接口方法)。

	以下为简要介绍：

	接口注册一般是在 [IQuestProcessor][] 子类实例的构造函数里注册。  
	接口注册函数为：

		/*
		    attributes:
		        EncryptOnly: 只有加密链接可以调用该接口
		        PrivateIPOnly: 只有内网地址可以调用该接口（IPv4 内网地址，或者IPv4/IPv6 本地环路地址）
		*/
		inline void registerMethod(const std::string& method_name, MethodFunc func, uint32_t attributes = 0);

	具体请参见 [registerMethod](APIs/core/IQuestProcessor.md#registerMethod)。

	注册的接口名可以和函数名不同，只要是合法字符串即可。可以包含空格。但不得以 * 号开头。

	“*” 号开头的接口，保留为**框架内置接口**。目前可用的内置接口请参考 [FPNN 内置接口](fpnn-build-in-methods.md)。

	一般情况下，为了便于维护，注册的接口名建议与接口函数名保持一致。

3. 向 [Client][] 注册 [IQuestProcessor][]:

	[Client][] 设置 [IQuestProcessor][] 事件处理对象的接口：

		void setQuestProcessor(IQuestProcessorPtr processor);

	详情请参见 [setQuestProcessor](APIs/core/Client.md#setQuestProcessor)。

	注：[Client][] 的配置类接口非线程安全，需要在数据收发前配置完成。

以上三个步骤完成后，[Client][] 便可处理 Server Push 请求。


### Server Push 中的提前返回

部分特殊业务，可能存在以下两种特殊需求：

* 需要尽可能快的响应。具体的请求数据可以在相应后再进行处理。
* 在响应数据准备好后，还有其他后续工作需要处理。且 QPS 压力不大。

此时为了简化业务流程，可以使用提前返回，在请求处理函数执行中途，发送 twoway 请求的响应，并在响应发送后，继续执行请求处理函数的剩余内容，不必等到请求处理函数执行完成时，才返回对 twoway 请求的响应。

提前返回接口为：

	bool sendAnswer(FPAnswerPtr answer);


**注意**

+ 调用该函数后，调用的请求处理函数必须返回 `nullptr`，否则会触发多次应答的异常日志。
+ 该函数必须在请求处理函数内(或请求处理函数返回前，同一线程上，被请求处理函数调用的其他函数中)被调用，否则无效。
+ 在请求处理函数内，该函数只能调用一次，后续调用无效。
+ 提前返回后继续执行的任务会继续占用 FPNN 客户端侧的 Server Push 请求处理线程。如果业务 Server Push 相对频繁，可能需要根据具体情况，调整 [ClientEngine 全局请求处理线程池](APIs/core/ClientEngine.md#configQuestProcessThreadPool)的大小，或者 [Client 私有请求处理线程池](APIs/core/Client.md#enableQuestProcessThreadPool)的大小。或者改为使用[异步返回](#Server-Push-中的异步返回)。

**返回值说明**

如果发送成功，返回 true；如果发送失败，或者是 oneway 类型消息，或者前面已经调用过该成员函数，则返回 false。

具体请参见 [sendAnswer](APIs/core/IQuestProcessor.md#sendAnswer)。



### Server Push 中的异步返回

部分业务，可能需要等待其他资源就绪后，才能准备响应数据，并进行返回。此时，可以使用异步返回，生成异步返回器。当资源就绪后，在其他任意线程中，返回响应。

异步返回器生成接口为：

	IAsyncAnswerPtr genAsyncAnswer();

具体可参见 [genAsyncAnswer](APIs/core/IQuestProcessor.md#genAsyncAnswer)。

**注意**

+ 调用该函数后，请求处理函数必须返回 `nullptr`。
+ 该函数必须在请求处理函数内(或请求处理函数返回前，同一线程上，被请求处理函数调用的其他函数中)被调用，否则无效。
+ 在请求处理函数内，该函数只能调用一次，后续调用无效。

异步返回器的使用，请参见 [IAsyncAnswer](APIs/core/IAsyncAnswer.md)。

**追踪 Server Push 中异步返回的异常**

如果流程过于复杂，在异步返回前，整个流程可能就因异常或者其他条件而中断、结束。  
这个时候，异步返回器对象，将自动返回一个错误号为 20001，内容为 “Answer is lost in normal logic. The error answer is sent for instead.” 的错误。  
这个错误指明了异步返回流程被异常终止，应答数据未正常返回。但对快速有效的定位中断位置，并无帮助。  
如果需要在流程的异常中断发生时，更加有效的定位中断位置，可以在流程的必要环节，调用 [IAsyncAnswer](APIs/core/IAsyncAnswer.md) 对象的 [cacheTraceInfo](APIs/core/IAsyncAnswer.md#cacheTraceInfo) 接口，写入必要的流程环节信息和状态信息。该信息会替代默认的 “Answer is lost in normal logic. The error answer is sent for instead.” 作为错误内容返回。  

[cacheTraceInfo](APIs/core/IAsyncAnswer.md#cacheTraceInfo) 接口可以多次被调用。新的状态信息将覆盖旧的状态信息。  
如果异步返回器的 [sendAnswer](APIs/core/IAsyncAnswer.md#sendAnswer) 接口被成功调用，则 [cacheTraceInfo](APIs/core/IAsyncAnswer.md#cacheTraceInfo) 的状态信息将被忽略。



## 独立的工作线程池

默认情况下，所有 [Client][] 实例将使用 [ClientEngine](APIs/core/ClientEngine.md) 提供的全局共享的请求**应答处理**线程池，和 Server Push **请求处理**线程池。当请求应答和 Server Push 拥堵时，可能会出现线程池任务排队，导致需要高实时响应的应答，或者请求，被引入额外的时延。此时可以对具体的 [Client][] 实例，启用专属的应答处理线程池，以及专属的请求处理线程池。

### 请求应答处理线程池

请求应答线程池，仅用于客户端侧收到请求的应答后，进行回调函数和对象的处理和调用。客户端实例启用专有请求处理线程时的接口为：

	inline void enableAnswerCallbackThreadPool(int32_t initCount, int32_t perAppendCount, int32_t perfectCount, int32_t maxCount);

具体请参见 [enableAnswerCallbackThreadPool](APIs/core/Client.md#enableAnswerCallbackThreadPool)。


### Server Push 请求处理线程池

Server Push 请求处理线程池，仅用于客户端对 Server Push 请求的处理。如果在对 Server Push 的请求的处理中，再使用 [sendQuest](APIs/core/IQuestProcessor.md#sendQuest) 发送请求，则该请求的应答依旧是由 [请求应答处理线程池](#请求应答处理线程池) 处理。

客户端可以通过以下接口启用专有的 Server Push 请求处理线程池：

	inline void enableQuestProcessThreadPool(int32_t initCount, int32_t perAppendCount, int32_t perfectCount, int32_t maxCount, size_t maxQueueSize);

具体请参见 [enableQuestProcessThreadPool](APIs/core/Client.md#enableQuestProcessThreadPool)。

亦可通过以下接口设置其他的线程池：

	inline void setQuestProcessThreadPool(TaskThreadPoolPtr questProcessPool);

具体请参见 [setQuestProcessThreadPool](APIs/core/Client.md#setQuestProcessThreadPool)。



## 可靠 UDP 链接

FPNN 框架默认提供可靠 UDP 链接，包括但不限于：

+ 丢包自动重传
+ 顺序自动重排
+ 自动去除重复包
+ 大包子切分传输，对端自动重组
+ MTU 适应
+ 链接自动保活
+ 防止篡改

默认情况下，只要使用 [Client::sendQuest](APIs/core/Client.md#sendQuest) 发送的 twoway 类型的请求和应答数据，以及 [UDPClient::sendQuestEx](APIs/core/UDPClient.md#sendQuestEx) 指定 `discardable = false` 的数据包，均是使用的可靠 UDP 连接的方式。只有使用 [Client::sendQuest](APIs/core/Client.md#sendQuest) 发送的 oneway 类型的请求，以及 [UDPClient::sendQuestEx](APIs/core/UDPClient.md#sendQuestEx) 指定 `discardable = true` 的数据包，才是非可靠的方式。

如果链接所有数据包均是使用 [Client::sendQuest](APIs/core/Client.md#sendQuest) 发送的 oneway 类型的请求，或者使用 [UDPClient::sendQuestEx](APIs/core/UDPClient.md#sendQuestEx) 发送 `discardable = true` 的数据包，且没有开启链接保活，则 UDP 链接将工作在非可靠模式。一旦发送可靠数据报，链接将转到可靠 UDP 模式。但此时依旧可以混合发送不可靠数据报。


## UDP 混发模式

UDP 混发模式，指的是在可靠 UDP 的连接上，可以混合发送传统的，不可靠的，可丢弃的数据报。对于不可靠的数据报，目前没有进行自动去重和顺序重排的处理。

具体的混合发送，请参见 [可靠 UDP 链接](#可靠-UDP-链接)。


[Client]: APIs/core/Client.md
[TCPClient]: APIs/core/TCPClient.md
[UDPClient]: APIs/core/UDPClient.md
[sendQuest]: APIs/core/Client.md#sendQuest
[IQuestProcessor]: APIs/core/IQuestProcessor.md