# FPNN 服务端高级使用向导

[TOC]

## HTTP/HTTPS 支持

FPNN 框架可以支持 HTTP/HTTPS 协议访问，如需开启 HTTP/HTTPS 支持，请在配置文件中加入以下配置即可：

	FPNN.server.http.supported = true

**注意**

因为 FPNN 框架并非 HTTP 服务框架，所以目前对 HTTP/HTTPS 访问有所限制。目前限制如下：

+ 目前仅支持 HTTP POST 与 GET 访问；
+ 接口的 URL 格式为:

		http://<主机名称>[:端口]/service/<接口名称>
		https://<主机名称>[:端口]/service/<接口名称>

	例：

		http://localhost:6789/service/echo 

+ HTTP POST 与 GET 只能访问 twoway 类型的接口，访问 oneway 类型的接口，会产生异常日志；
+ POST 的数据需要以 JSON 对象形式描述；
+ GET 除 `fpnn` 参数外，其他参数会被忽略；
+ GET 的 `fpnn` 参数须为 JSON 对象形式；
+ header、cookie 等信息获取接口暂未开放。


默认情况下，FPNN 框架响应 HTTP/HTTPS 的请求后，不会关闭当前链接，直到收到关闭信号，或者链接 Idle 超时。链接 Idle 超时默认为 60 秒，可以通过配置文件条目 `FPNN.server.idle.timeout` 在配置文件中配置。或是 [TCPEpollServer](APIs/core/TCPEpollServer.md) 对象的 [setIdleTimeout](APIs/core/TCPEpollServer.md#setIdleTimeout) 接口进行设置。如果希望回复后立刻关闭当前链接，请在配置文件中加入：

	FPNN.server.http.closeAfterAnswered = true

**注意**

HTTPS 还需要配置 SSL/TLS 支持。SSL/TLS 支持请参见 [SSL/TLS 支持](#SSLTLS-支持)。



## WebSocket 支持

WebSocket 需要在配置文件中配置 HTTP 支持：

	FPNN.server.http.supported = true

`FPNN.server.http.closeAfterAnswered` 参数不影响 WebSocket 行为。

WebSocket 支持 server push。

**注意**

+ 对 wss 协议，还需要配置 SSL/TLS 支持。SSL/TLS 支持请参见 [SSL/TLS 支持](#SSLTLS-支持)。
+ WebSocket 控制帧中如果包含 payload，payload 将会被忽略。



## SSL/TLS 支持

如果服务器需要支持 HTTPS，或者 wss 协议，请在对应的 HTTP 支持，或 WebSocket 支持的基础上，在配置文件中增加以下项目：

	FPNN.server.tcp.ipv4.ssl.listening.port =
	FPNN.server.tcp.ipv4.ssl.listening.ip =

	FPNN.server.tcp.ipv6.ssl.listening.port =
	FPNN.server.tcp.ipv6.ssl.listening.ip =

	# certificate 与 PrivateKey 均为文件路径
	FPNN.server.security.ssl.certificate = 
	FPNN.server.security.ssl.privateKey =

+ 其中，对 IP 的配置可选。
	
	IP 的配置用于配置 SSL/TLS 与普通 TCP/IP 不同的监听地址。

+ port 参数，IPv4 与 IPv6 至少需要配置一个。




## IPv6 支持

启动 IPv6 支持前，请先确认以下事项：

1. 操作系统已启用 IPv6 支持
1. 操作系统已分配 IPv6 地址
1. 当前网络支持 IPv6 路由

以上都妥当后，在配置文件中加入并配置以下两项即可：

	FPNN.server.ipv6.listening.ip = 
	FPNN.server.ipv6.listening.port =

其中 FPNN.server.ipv6.listening.ip 可不填。

当服务器启动后，将监听 IPv6 端口。

**注意**

+ FPNN 可同时监听 IPv4 和 IPv6 端口。
+ 若同时配置 TCP 和 UDP 对 IPv4 和 IPv6 的监听，可以根据需要，在配置文件中加入以下条目：

		FPNN.server.tcp.ipv4.listening.port =
		FPNN.server.tcp.ipv4.listening.ip =

		FPNN.server.tcp.ipv6.listening.port =
		FPNN.server.tcp.ipv6.listening.ip =

		FPNN.server.udp.ipv4.listening.port =
		FPNN.server.udp.ipv4.listening.ip =

		FPNN.server.udp.ipv6.listening.port =
		FPNN.server.udp.ipv6.listening.ip =

	具体请参考 [配置文件模板](../conf.template) 中相关段落的注释。



## 链接保活

链接保活需要客户端开启，具体请参见 [“FPNN 客户端高级使用向导” - “链接保活”](fpnn-client-advanced-tutorial.md#链接保活)。




## 链接建立与关闭事件

### 链接建立事件

在 [IQuestProcessor][] 的派生类中重载 `connected` 接口，即可处理链接建立事件：

	virtual void connected(const ConnectionInfo&);

**注意**

+ 对于加密链接，该函数会在秘钥协商前被调用；
+ 在链接建立事件返回前，该链接上的数据收发将被阻塞，直到链接建立事件返回。

详情请参见 [connected](APIs/core/IQuestProcessor.md#connected)。


### 链接关闭事件

在 [IQuestProcessor][] 的派生类中重载 `connectionWillClose` 接口，即可处理链接关闭事件：

	virtual void connectionWillClose(const ConnectionInfo& connInfo, bool closeByError);

**注意**

+ 因为多任务并发处理的缘故，可能链接关闭事件开始处理时，本次连接的相关请求处理还未结束。

详情请参见 [connectionWillClose](APIs/core/IQuestProcessor.md#connectionWillClose)。


## 服务启动事件

在 [IQuestProcessor][] 的派生类中重载 `start` 接口，即可处理服务启动事件：

	virtual void start();

当事件函数返回后，服务器才正式开始处理客户端发送的请求。

详情请参见 [start](APIs/core/IQuestProcessor.md#start)。


## 服务即将停止事件

在 [IQuestProcessor][] 的派生类中重载 `serverWillStop` 接口，即可处理服务即将停止事件：

	virtual void serverWillStop();

当事件函数返回后，服务器将终止收发信息。

详情请参见 [serverWillStop](APIs/core/IQuestProcessor.md#serverWillStop)。



## 服务完全停止事件

在 [IQuestProcessor][] 的派生类中重载 `serverStopped` 接口，即可处理服务完全停止事件：

	virtual void serverStopped();

进入该函数时，服务器的工作线程池已经停止运行。

详情请参见 [serverStopped](APIs/core/IQuestProcessor.md#serverStopped)。




## 获取业务状态信息

FPNN 框架内置的 `*infos` 接口会返回很多 FPNN 框架运行时的状态信息，但默认情况下业务状态信息 `APP.status` 部分为空。

在 [IQuestProcessor][] 的派生类中重载 `infos` 接口，即可通过  `*infos` 接口的 `APP.status` 部分返回业务的状态信息：

	virtual std::string infos(); //-- 请返回字符串形式的 Json 对象数据。

详情请参见 [infos](APIs/core/IQuestProcessor.md#infos)。




## 动态调节参数

FPNN 框架内置的 `*tune` 接口可以动态调节服务框架的各种动态参数。  
如果需通过该接口，动态调节业务自身的动态参数，在 [IQuestProcessor][] 的派生类中重载 `tune` 接口即可：

	virtual void tune(const std::string& key, std::string& value);

详情请参见 [tune](APIs/core/IQuestProcessor.md#tune)。




## 提前返回

部分特殊业务，对于 twoway 类型的请求，可能存在以下两种特殊需求：

* 需要尽可能快的响应。具体的请求数据可以在相应后再进行处理。
* 在响应数据准备好后，还有其他后续工作需要处理。且 QPS 压力不大。

此时为了简化业务流程，可以使用提前返回，在请求处理函数执行中途，发送 twoway 请求的响应，并在响应发送后，继续执行请求处理函数的剩余内容，不必等到请求处理函数执行完成时，才返回对 twoway 请求的响应。

提前返回接口为：

	bool sendAnswer(FPAnswerPtr answer);

**注意**

+ 调用该函数后，调用的请求处理函数必须返回 `nullptr`，否则会触发多次应答的异常日志。
+ 该函数必须在请求处理函数内(或请求处理函数返回前，同一线程上，被请求处理函数调用的其他函数中)被调用，否则无效。
+ 在请求处理函数内，该函数只能调用一次，后续调用无效。
+ 提前返回后继续执行的任务会继续占用 FPNN 服务端侧的工作线程。如果业务 QPS 相对较高，服务端工作队列出现积压，可以根据具体情况，调整工作线程池的大小，或者改为使用[异步返回](#异步返回)。

	* TCPEpollServer 工作线程池配置接口：[configWorkerThreadPool](APIs/core/TCPEpollServer.md#configWorkerThreadPool)
	* UDPEpollServer 工作线程池配置接口：[configWorkerThreadPool](APIs/core/UDPEpollServer.md#configWorkerThreadPool)
	* TCPEpollServer & TUDPEpollServer 配置文件通用配置项：

			FPNN.server.work.queue.max.size
			FPNN.server.work.thread.min.size
			FPNN.server.work.thread.max.size

	* TCPEpollServer 配置文件专用配置项：

			FPNN.server.tcp.work.queue.max.size =
			FPNN.server.tcp.work.thread.min.size =
			FPNN.server.tcp.work.thread.max.size =

	* UDPEpollServer 配置文件专用配置项：

			FPNN.server.udp.work.queue.max.size =
			FPNN.server.udp.work.thread.min.size =
			FPNN.server.udp.work.thread.max.size =

**返回值说明**

如果发送成功，返回 true；如果发送失败，或者是 oneway 类型消息，或者前面已经调用过该成员函数，则返回 false。

具体请参见 [sendAnswer](APIs/core/IQuestProcessor.md#sendAnswer)。



## 异步返回

对于 twoway 类型的请求，如果需要等待其他资源就绪，才能生成响应数据并返回，此时便可使用异步返回功能。  
异步返回须先生成异步返回器，然后在资源就绪时，在其他任意线程中，调用异步返回器返回响应数据。

异步返回器生成接口为：

	IAsyncAnswerPtr genAsyncAnswer();

具体可参见 [genAsyncAnswer](APIs/core/IQuestProcessor.md#genAsyncAnswer)。

**注意**

+ 调用该函数后，请求处理函数必须返回 `nullptr`。
+ 该函数必须在请求处理函数内(或请求处理函数返回前，同一线程上，被请求处理函数调用的其他函数中)被调用，否则无效。
+ 在请求处理函数内，该函数只能调用一次，后续调用无效。

异步返回器的使用，请参见 [IAsyncAnswer](APIs/core/IAsyncAnswer.md)。


**追踪异步返回异常**

如果流程过于复杂，在异步返回前，整个流程可能就因异常或者其他条件而中断、结束。  
这个时候，异步返回器对象，将自动返回一个错误号为 20001，内容为 “Answer is lost in normal logic. The error answer is sent for instead.” 的错误。  
这个错误指明了异步返回流程被异常终止，应答数据未正常返回。但对快速有效的定位中断位置，并无帮助。  
如果需要在流程的异常中断发生时，更加有效的定位中断位置，可以在流程的必要环节，调用 [IAsyncAnswer](APIs/core/IAsyncAnswer.md) 对象的 [cacheTraceInfo](APIs/core/IAsyncAnswer.md#cacheTraceInfo) 接口，写入必要的流程环节信息和状态信息。该信息会替代默认的 “Answer is lost in normal logic. The error answer is sent for instead.” 作为错误内容返回。  

[cacheTraceInfo](APIs/core/IAsyncAnswer.md#cacheTraceInfo) 接口可以多次被调用。新的状态信息将覆盖旧的状态信息。  
如果异步返回器的 [sendAnswer](APIs/core/IAsyncAnswer.md#sendAnswer) 接口被成功调用，则 [cacheTraceInfo](APIs/core/IAsyncAnswer.md#cacheTraceInfo) 的状态信息将被忽略。




## Server Push (双工/duplex)模式

如果需要向客户端发送消息或请求，有以下两种方式：

* **便捷方式**

	便捷方式可在请求处理函数中直接调用 [IQuestProcessor][] 的 [sendQuest](APIs/core/IQuestProcessor.md#sendQuest) 接口即可：

		virtual FPAnswerPtr sendQuest(FPQuestPtr quest, int timeout = 0);
		virtual bool sendQuest(FPQuestPtr quest, AnswerCallback* callback, int timeout = 0);
		virtual bool sendQuest(FPQuestPtr quest, std::function<void (FPAnswerPtr answer, int errorCode)> task, int timeout = 0);

	详细请参见 [sendQuest](APIs/core/IQuestProcessor.md#sendQuest)。

	UDP 链接还可调用 [sendQuestEx](APIs/core/IQuestProcessor.md#sendQuestEx) 接口：

		virtual FPAnswerPtr sendQuestEx(FPQuestPtr quest, bool discardable, int timeoutMsec = 0);
		virtual bool sendQuestEx(FPQuestPtr quest, AnswerCallback* callback, bool discardable, int timeoutMsec = 0);
		virtual bool sendQuestEx(FPQuestPtr quest, std::function<void (FPAnswerPtr answer, int errorCode)> task, bool discardable, int timeoutMsec = 0);

	详细请参见 [sendQuestEx](APIs/core/IQuestProcessor.md#sendQuestEx)。


**注意**

便捷方式仅能在请求处理函数中使用，请求函数外向客户端发送请求或消息数据，须使用通用方式。


* **通用方式**

	通用方式只需在 [链接建立事件](#链接建立事件) 或者请求处理函数中，生成 [请求发送对象 QuestSender](APIs/core/QuestSender.md) 后，便可在任意地方，向客户端发送数据。

	生成请求发送对象 QuestSender：

		QuestSenderPtr genQuestSender(const ConnectionInfo& connectionInfo);

	详细请参见 [genQuestSender](APIs/core/IQuestProcessor.md#genQuestSender)。

	生成请求发送对象后，便可直接调用 [sendQuest](APIs/core/QuestSender.md#sendQuest) 接口，或者  [sendQuestEx](APIs/core/QuestSender.md#sendQuestEx) 接口，向客户端发送请求。

	[sendQuest](APIs/core/QuestSender.md#sendQuest) 接口：

		virtual FPAnswerPtr sendQuest(FPQuestPtr quest, int timeout = 0);
		virtual bool sendQuest(FPQuestPtr quest, AnswerCallback* callback, int timeout = 0);
		virtual bool sendQuest(FPQuestPtr quest, std::function<void (FPAnswerPtr answer, int errorCode)> task, int timeout = 0);

	[sendQuestEx](APIs/core/QuestSender.md#sendQuestEx) 接口：

		virtual FPAnswerPtr sendQuestEx(FPQuestPtr quest, bool discardable, int timeoutMsec = 0);
		virtual bool sendQuestEx(FPQuestPtr quest, AnswerCallback* callback, bool discardable, int timeoutMsec = 0);
		virtual bool sendQuestEx(FPQuestPtr quest, std::function<void (FPAnswerPtr answer, int errorCode)> task, bool discardable, int timeoutMsec = 0);



## 主动关闭连接

直接调用服务对象的 `closeConnection` 接口即可。

TCPEpollServer 的 `closeConnection` 接口：

	void closeConnection(const ConnectionInfo* ci);
	inline void closeConnection(const ConnectionInfo& ci);

详情请参见：[closeConnection](APIs/core/TCPEpollServer.md#closeConnection)。

UDPEpollServer 的 `closeConnection` 接口：

	void closeConnection(int socket, bool force = false);

详情请参见：[closeConnection](APIs/core/UDPEpollServer.md#closeConnection)。



## HOOK：请求的预处理 & 应答的后处理

1. 相关知识回顾

	* [IQuestProcessor][] 包含 [unknownMethod](APIs/core/IQuestProcessor.md#unknownMethod) 接口，可以接管处理对任何未注册接口的请求；
	* C++ 中，父类的非虚函数，子类可以覆盖；
	* FPNN 中，对于 twoway 类型的请求，存在正常返回，提前返回，异步返回；
	* FPNN 中，服务端向客户端发送 Server Push 请求，可以在当前接口处理函数中，也可以在任意函数中。

1. 方案

	1. 采用双层 [IQuestProcessor][] 派生类处理：

		server 使用第一层的 [IQuestProcessor][] 派生类实例。  
		第一层的 [IQuestProcessor][] 派生类不注册任何接口，则所有请求都将进入第一层派生类的 [unknownMethod](APIs/core/IQuestProcessor.md#unknownMethod) 接口中。  
		第二层的 [IQuestProcessor][] 派生类注册所有的业务接口，并在第一层派生类的 [unknownMethod](APIs/core/IQuestProcessor.md#unknownMethod) 接口中，调用第二层派生类实例的 `processQuest` 接口。调用前后即可 hook 处理所有的消息请求，以及正常流程的返回。

		**注意**

		`processQuest` 接口默认是 FPNN 框架内部接口，除了 HOOK 消息处理外，没有外部用途。因此没有该接口的公开文档。FPNN 框架保证该接口的持久性，因此可以用于 FPNN 框架使用者对请求的 HOOK 处理。  
		`processQuest` 接口可以直接参见代码文件：[IQuestProcessor.h](../../core/IQuestProcessor.h)。

	1. 派生 [QuestSender](APIs/core/QuestSender.md) (Server Push 请求发送)、[IAsyncAnswer](APIs/core/IAsyncAnswer.md) (异步返回器) 两个对象：

		两个派生类须包含原始的对象实例。在进行完相应的 hook 操作后，向原始对象转发数据。

	1. 第二层的 IQuestProcessor 派生类定义 [sendAnswer](APIs/core/IQuestProcessor.md#sendAnswer)、[genAsyncAnswer](APIs/core/IQuestProcessor.md#genAsyncAnswer)、[genQuestSender](APIs/core/IQuestProcessor.md#genQuestSender)、[sendQuest](APIs/core/IQuestProcessor.md#sendQuest)、[sendQuestEx](APIs/core/IQuestProcessor.md#sendQuestEx) 系列函数，覆盖父类同名函数。

		在派生类的函数中，进行完相应的 hook 操作后，需要向父类被覆盖的接口转发数据。

1. 示例

	class HookedQuestSender:

		class HookedQuestSender: public QuestSender
		{
		    QuestSenderPtr _realSender;
		 
		public:
		    HookedQuestSender(QuestSenderPtr sender): _realSender(sender) {}
		    virtual ~HookedQuestSender() {}
		 
		    virtual FPAnswerPtr sendQuest(FPQuestPtr quest, int timeout = 0)
		    {
		        //-- hook codes before server push quest to client
		        ... ...
		 
		        FPAnswerPtr answer = _realSender->sendQuest(quest, timeout);
		 
		        //-- hook codes after server pushed quest to client
		        ... ...
		 
		        return answer;
		    }
		     
		    virtual bool sendQuest(FPQuestPtr quest, AnswerCallback* callback, int timeout = 0)
		    {
		        ... ...
		    }
		     
		    virtual bool sendQuest(FPQuestPtr quest, std::function<void (FPAnswerPtr answer, int errorCode)> task, int timeout = 0)
		    {
		        ... ...
		    }

		    virtual FPAnswerPtr sendQuestEx(FPQuestPtr quest, bool discardable, int timeoutMsec = 0)
		    {
		    	... ...
		    }

			virtual bool sendQuestEx(FPQuestPtr quest, AnswerCallback* callback, bool discardable, int timeoutMsec = 0)
		    {
		    	... ...
		    }

			virtual bool sendQuestEx(FPQuestPtr quest, std::function<void (FPAnswerPtr answer, int errorCode)> task, bool discardable, int timeoutMsec = 0)
		    {
		    	... ...
		    }
		};

	class HookedAsyncAnswer:
		 
		class HookedAsyncAnswer: public IAsyncAnswer
		{
		    IAsyncAnswerPtr _realAsync;
		 
		public:
		    HookedAsyncAnswer(IAsyncAnswerPtr async): _realAsync(async) {}
		    virtual ~HookedAsyncAnswer() {}
		 
		    virtual const FPQuestPtr getQuest() { return _realAsync->getQuest(); }
		 
		    virtual bool sendAnswer(FPAnswerPtr answer)
		    {
		        //-- hook codes before send answer to client
		        ... ...
		 
		        bool status = _realAsync->sendAnswer(answer);
		 
		        //-- hook codes after answer sent to client
		        ... ...
		 
		        return status;
		    }
		 
		    virtual bool isSent() { return _realAsync->isSent(); }
		 
		    virtual void cacheTraceInfo(const std::string& info) { return _realAsync->cacheTraceInfo(info); }
		    virtual void cacheTraceInfo(const char *info) { return _realAsync->cacheTraceInfo(info); }
		         
		    virtual bool sendErrorAnswer(int code = 0, const std::string& ex = "")
		    {
		        FPAnswerPtr answer = FPAWriter::errorAnswer(getQuest(), code, ex);
		        return sendAnswer(answer);
		    }
		    virtual bool sendErrorAnswer(int code = 0, const char* ex = "")
		    {
		        FPAnswerPtr answer = FPAWriter::errorAnswer(getQuest(), code, ex);
		        return sendAnswer(answer);
		    }
		    virtual bool sendEmptyAnswer()
		    {
		        FPAnswerPtr answer = FPAWriter::emptyAnswer(getQuest());
		        return sendAnswer(answer);
		    }
		};

	class SecondQuestProcessor:

		class SecondQuestProcessor: public IQuestProcessor
		{
		public:
		    //-- 正常业务
		    FPAnswerPtr interfaceA(const FPReaderPtr, const FPQuestPtr, const ConnectionInfo&);
		    FPAnswerPtr interfaceB(const FPReaderPtr, const FPQuestPtr, const ConnectionInfo&);
		    ... ...
		    FPAnswerPtr interfaceN(const FPReaderPtr, const FPQuestPtr, const ConnectionInfo&);
		 
		    //-- 正常业务
		    SecondQuestProcessor()
		    {
		        registerMethod("interfaceA", &SecondQuestProcessor::interfaceA);
		        registerMethod("interfaceB", &SecondQuestProcessor::interfaceB);
		        ... ...
		        registerMethod("interfaceN", &SecondQuestProcessor::interfaceN);
		    }
		     
		    //-- Hook 提前返回
		    bool sendAnswer(const FPQuestPtr quest, FPAnswerPtr answer)
		    {
		        //-- hook codes before send answer to client
		        ... ...
		 
		        bool status = IQuestProcessor::sendAnswer(quest, answer);
		 
		        //-- hook codes before send answer to client
		        ... ...
		 
		        return status;
		    }
		 
		    //-- Hook 异步返回
		    IAsyncAnswerPtr genAsyncAnswer(const FPQuestPtr quest)
		    {
		        IAsyncAnswerPtr realAsync = IQuestProcessor::genAsyncAnswer(quest);
		        IAsyncAnswerPtr async = std::make_shared<HookedAsyncAnswer>(realAsync);
		        return async;
		    }
		 
		    //-- Hook 任意位置的 Server Push
		    QuestSenderPtr genQuestSender(const ConnectionInfo& connectionInfo)
		    {
		        QuestSenderPtr realSender = IQuestProcessor::genQuestSender(connectionInfo);
		        QuestSenderPtr sender = std::make_shared<HookedQuestSender>(realSender);
		        return sender;
		    }
		 
		    //-- hook 接口处理函数内的 Server Push
		    virtual FPAnswerPtr sendQuest(const ConnectionInfo& connectionInfo, FPQuestPtr quest, int timeout = 0)
		    {
		        //-- hook codes before server push quest to client
		        ... ...
		 
		        FPAnswerPtr answer = IQuestProcessor::sendQuest(connectionInfo, quest, timeout);
		 
		        //-- hook codes after server pushed quest to client
		        ... ...
		 
		        return answer;
		    }
		 
		    virtual bool sendQuest(const ConnectionInfo& connectionInfo, FPQuestPtr quest, AnswerCallback* callback, int timeout = 0)
		    {
		    	... ...
		    }
		 
		    virtual bool sendQuest(const ConnectionInfo& connectionInfo, FPQuestPtr quest, std::function<void (FPAnswerPtr answer, int errorCode)> task, int timeout = 0)
		    {
		        ... ...
		    }

		    virtual FPAnswerPtr sendQuestEx(FPQuestPtr quest, bool discardable, int timeoutMsec = 0)
		    {
		    	... ...
		    }

			virtual bool sendQuestEx(FPQuestPtr quest, AnswerCallback* callback, bool discardable, int timeoutMsec = 0)
		    {
		    	... ...
		    }

			virtual bool sendQuestEx(FPQuestPtr quest, std::function<void (FPAnswerPtr answer, int errorCode)> task, bool discardable, int timeoutMsec = 0)
		    {
		    	... ...
		    }
		};

	class FirstQuestProcessor:

		class FirstQuestProcessor: public IQuestProcessor
		{
		    SecondQuestProcessor _realProcessor;
		 
		public:
		    //-- hook 所有的请求输入，以及正常的请求响应
		    virtual FPAnswerPtr unknownMethod(const std::string& method_name, const FPReaderPtr args, const FPQuestPtr quest, const ConnectionInfo& connInfo)
		    {
		        //-- hook codes before processor quest
		        ... ...
		 
		        FPAnswerPtr answer = _realProcessor.processQuest(args, quest, connInfo);
		     
		        //-- hook codes after quest processed.
		        ... ...
		 
		        return answer;
		    }
		};
		 
	main.cpp:

		int main(int argc, char* argv[])
		{
		    if (argc != 2)
		    {
		        std::cout<<"Usage: "<<argv[0]<<" config"<<std::endl;
		        return 0;
		    }
		    if(!Setting::load(argv[1])){
		        std::cout<<"Config file error:"<< argv[1]<<std::endl;
		        return 1;
		    }
		    ServerPtr server = TCPEpollServer::create();
		    server->setQuestProcessor(std::make_shared<FirstQuestProcessor>());
		    if (server->startup())
		        server->run();
		    return 0;
		}



## IP 白名单

如果需要限制或者指定可以访问服务的来源地址，或者来源网段，可以启用IP白名单功能。

**注意**

为了安全起见，IP 白名单功能无法通过配置文件配置，需要分别调用 TCPEpollServer 和 UDPEpollServer 的相关接口。

* TCPEpollServer 相关接口：

	+ ipWhiteListEnabled

			inline bool ipWhiteListEnabled();

		请参见 [ipWhiteListEnabled](APIs/core/TCPEpollServer.md#ipWhiteListEnabled)。

	+ enableIPWhiteList

			inline void enableIPWhiteList(bool enable = true);

		请参见 [enableIPWhiteList](APIs/core/TCPEpollServer.md#enableIPWhiteList)。

	+ addIPToWhiteList

			inline bool addIPToWhiteList(const std::string& ip);

		请参见 [addIPToWhiteList](APIs/core/TCPEpollServer.md#addIPToWhiteList)。

	+ ipWhiteLisaddSubnetToWhiteListtEnabled

			inline bool addSubnetToWhiteList(const std::string& ip, int subnetMaskBits);

		请参见 [addSubnetToWhiteList](APIs/core/TCPEpollServer.md#addSubnetToWhiteList)。

	+ removeIPFromWhiteList

			inline void removeIPFromWhiteList(const std::string& ip);

		请参见 [removeIPFromWhiteList](APIs/core/TCPEpollServer.md#removeIPFromWhiteList)。


* UDPEpollServer 相关接口：

	+ ipWhiteListEnabled

			inline bool ipWhiteListEnabled();

		请参见 [ipWhiteListEnabled](APIs/core/UDPEpollServer.md#ipWhiteListEnabled)。

	+ enableIPWhiteList

			inline void enableIPWhiteList(bool enable = true);

		请参见 [enableIPWhiteList](APIs/core/UDPEpollServer.md#enableIPWhiteList)。

	+ addIPToWhiteList

			inline bool addIPToWhiteList(const std::string& ip);

		请参见 [addIPToWhiteList](APIs/core/UDPEpollServer.md#addIPToWhiteList)。

	+ ipWhiteLisaddSubnetToWhiteListtEnabled

			inline bool addSubnetToWhiteList(const std::string& ip, int subnetMaskBits);

		请参见 [addSubnetToWhiteList](APIs/core/UDPEpollServer.md#addSubnetToWhiteList)。

	+ removeIPFromWhiteList

			inline void removeIPFromWhiteList(const std::string& ip);

		请参见 [removeIPFromWhiteList](APIs/core/UDPEpollServer.md#removeIPFromWhiteList)。


相关接口可以在任何时候调用，多线程安全。

**注意**

* IP 白名单默认加入 IPv4 & IPv6 本地环路地址；
* 如果启动白名单，但没有加入任何 IP，则内网 IP 及本机 IP（本地环路地址除外）均无法访问；
* 可以通过 [`*tune`](fpnn-build-in-methods.md#tune) 接口动态启用／禁用 IP 白名单，以及动态添加／删除 IP 地址。



## 加密模式

FPNN 采用 ECC(椭圆曲线算法)进行秘钥协商，AES 算法的 CFB 模式进行加密。  

+ 所有秘钥协商和加解密操作对业务透明。
+ 该功能独立于对 SSL/TLS 支持。
+ 为防止**无谓的消耗**系统资源，在 SSL/TLS 的链接上，不支持 FPNN 的加密功能。

**注意**

一旦启用加密，则无论是全局加密，还是部分接口加密，还是可选加密，框架内置的 `*tune` 接口将强制要求加密访问。

* 使用配置文件启用加密功能

	请在配置文件中增加以下条目：

		FPNN.server.security.ecdh.enable = true
		 
		# curve 取值: secp256k1, secp256r1, secp224r1, secp192r1
		FPNN.server.security.ecdh.curve = <curve name>
		 
		# privateKey 指向 privateKey (binary format, not Hex) 的文件路径
		FPNN.server.security.ecdh.privateKey = <private key file path>

	以上配置条目加入后，加密已经启用，但处于**非强制加密**状态。加密链接和非加密链接都可以访问任何没有指定**加密属性**的接口。

	如果要所有接口都必须通过加密链接访问，则增加以下配置项：

		FPNN.server.security.forceEncrypt.userMethods = true

	**注意**

	+ privateKey 请使用 [eccKeyMaker](fpnn-tools.md#eccKeyMaker) 生成。
	+ 任何情况下，框架内置的 `*status` 接口和 `*infos` 接口均可以在内网，以非加密方式访问。
	+ 如果要检查当前链接是否加密，请调用 [ConnectionInfo](APIs/core/ConnectionInfo.md) 对象的 [isEncrypted](APIs/core/ConnectionInfo.md#isEncrypted) 方法。


* 使用接口启用加密功能

	目前仅 [TCPEpollServer](APIs/core/TCPEpollServer.md) 对象支持加密功能，[UDPEpollServer](APIs/core/UDPEpollServer.md) 暂不支持。

	[TCPEpollServer](APIs/core/TCPEpollServer.md) 加密相关接口：

	+ 判断加密状态

			inline bool encrpytionEnabled();

		请参见 [encrpytionEnabled](APIs/core/TCPEpollServer.md#encrpytionEnabled)。

	+ 配置加密密钥

			inline bool enableEncryptor(const std::string& curve, const std::string& privateKey);

		请参见 [enableEncryptor](APIs/core/TCPEpollServer.md#enableEncryptor)。

	+ 启用强制加密

			static void enableForceEncryption();

		请参见 [enableForceEncryption](APIs/core/TCPEpollServer.md#enableForceEncryption)。


* 选择性加密

	在注册接口时，[registerMethod](APIs/core/IQuestProcessor.md#registerMethod) 方法的 attributes 参数指定 [EncryptOnly](APIs/core/IQuestProcessor.md#MethodAttribute) 即可。




## 旁路请求数据

旁路请求数据给 FPNN 框架开发的服务，可使用 [Bypass](APIs/extends/Bypass.md) 对象，旁路请求数据给 HTTP 接口，可使用 [HttpBypass](APIs/extends/HttpBypass.md) 对象。

如果有定制化的旁路需求，可参考 [Bypass.h](../../extends/Bypass.h) 或 [HttpBypass.h](../../extends/HttpBypass.h) 中的实现。



## 服务器集群

FPNN 技术生态使用 FPZK 作为默认的集群管理和服务发现组件。

FPZK 支持全球组网，全球范围的服务发现和集群管理。且集群内支持再次分组。同时还支持网络的分裂与合并。

[FPZKClient](APIs/extends/FPZKClient.md) 是 FPZK 服务的客户端，负责业务实例的注册和状态维护，以及集群变动的订阅和监测。详细文档请参见：[FPZKClient](APIs/extends/FPZKClient.md)。

目前 FPZK 暂未开源，但计划将作为 FPNN 开源项目的一部分，在未来某个时间点开源。现在如需使用，可联系 [云上曲率开源项目](https://github.com/highras) 相关人员，或是 [云上曲率](https://www.ilivedata.com/) 的商务同学（FPZK 并非商业收费项目）。




## 集群消息路由 & 内部负载均衡

集群消息路由 & 内部负载均衡 在 FPNN 框架中，均由 Proxy 系列组件完成。

目前 FPNN 框架的 Proxy 分成 无集群管理服务版本 和 有集群管理服务(FPZK)版本 两类。

* 无集群管理服务版本

	集群的变动需要手动更新 proxy 的 endpoint 列表进行。在未更新的情况下，可视为固定集群。

	目前无集群管理服务的 proxy 一共有五种：

	+ 随机路由 & 随机负载均衡：[TCPRandomProxy](APIs/extends/TCPRandomProxy.md)
	+ 轮询路由 & 轮询负载均衡：[TCPRotatoryProxy](APIs/extends/TCPRotatoryProxy.md)
	+ 一致性哈希路由 & 一致性哈希负载均衡：[TCPCarpProxy](APIs/extends/TCPCarpProxy.md)
	+ 广播路由：[TCPBroadcastProxy](APIs/extends/TCPBroadcastProxy.md)
	+ 一致性路由：[TCPConsistencyProxy](APIs/extends/TCPConsistencyProxy.md)

* 有集群管理服务(FPZK)版本

	集群变动由 FPZK 服务和 FPZKClient 实时更新，无需手动干预。

	目前依赖于集群管理服务(FPZK)的 proxy 一共有六种：

	+ 随机路由 & 随机负载均衡：[TCPFPZKRandomProxy](APIs/extends/TCPFPZKRandomProxy.md)
	+ 轮询路由 & 轮询负载均衡：[TCPFPZKRotatoryProxy](APIs/extends/TCPFPZKRotatoryProxy.md)
	+ 一致性哈希路由 & 一致性哈希负载均衡：[TCPFPZKCarpProxy](APIs/extends/TCPFPZKCarpProxy.md)
	+ 广播路由：[TCPFPZKBroadcastProxy](APIs/extends/TCPFPZKBroadcastProxy.md)
	+ 一致性路由：[TCPFPZKConsistencyProxy](APIs/extends/TCPFPZKConsistencyProxy.md)
	+ 最久运行/最早注册路由：[TCPFPZKOldestProxy](APIs/extends/TCPFPZKOldestProxy.md)




## HTTP/HTTPS 海量并发访问引擎

FPNN 框架的 [MultipleURLEngine](APIs/extends/MultipleURLEngine.md) 组件提供了海量 HTTP/HTTPS 1.0/1.1 访问的能力。支持单机数万并发访问。详细可参见：[MultipleURLEngine](APIs/extends/MultipleURLEngine.md)。


## 可靠 UDP 链接

FPNN 框架默认提供可靠 UDP 链接，包括但不限于：

+ 丢包自动重传
+ 顺序自动重排
+ 自动去除重复包
+ 大包子切分传输，对端自动重组
+ MTU 适应
+ 链接自动保活
+ 防止篡改

默认情况下，只要使用任意 sendQuest 接口发送的 twoway 类型的 UDP 请求和应答数据，以及任意 sendQuestEx 接口指定 `discardable = false` 的 UDP 数据包，均是使用的可靠 UDP 连接的方式。只有使用 sendQuest 接口发送的 oneway 类型的 UDP 请求，以及 sendQuestEx 接口指定 `discardable = true` 的 UDP 数据包，才是非可靠的方式。

如果 UDP 链接所有数据包均是使用 sendQuest 接口发送的 oneway 类型的请求，或者使用 sendQuestEx 接口发送 `discardable = true` 的数据包，且没有开启链接保活，则 UDP 链接将工作在非可靠模式。一旦发送可靠数据报，链接将转到可靠 UDP 模式。但此时依旧可以混合发送不可靠数据报。


## UDP 混发模式

UDP 混发模式，指的是在可靠 UDP 的连接上，可以混合发送传统的，不可靠的，可丢弃的数据报。对于不可靠的数据报，目前没有进行自动去重和顺序重排的处理。

具体的混合发送，请参见 [可靠 UDP 链接](#可靠-UDP-链接)。



[IQuestProcessor]: APIs/core/IQuestProcessor.md
