# FPNN Server Advanced Tutorial


## * HTTP 支持

如果服务需要支持 HTTP POST & GET 访问，请在配置文件中增加一行：

	FPNN.server.http.supported = true

注意：

+ 目前，FPNN 仅支持 HTTP POST 与 GET 访问。
+ HTTP POST 与 GET 只能访问 twoway 类型的接口，访问 oneway 类型的接口，会产生异常日志。

默认情况下，FPNN 应答 HTTP POST & GET 请求后，将保持当前链接一段时间。链接保持时间取决于配置项 FPNN.server.idle.timeout。默认情况下为 60 秒。 
如果希望回复后立刻关闭当前链接，请在配置文件中加入：

	FPNN.server.http.closeAfterAnswered = true

注意：

+ 对于 HTTP 协议，FPNN 要求在同一链接内，HTTP POST 或 GET 未应答前，客户端不应发送下一请求。TCP 无此限制。
+ GET 只有一个参数 fpnn，值为 json dict 类型。所有业务参数均包含在其中。

以基础向导部分的 DemoServer 为例，测试 echo 接口：

	curl -d '{"feedback":"Hello, FPNN!"}' http://localhost:6789/service/echo 




## * IPv6 支持

启动 IPv6 支持前，请先确认以下事项：

1. 操作系统已启用 IPv6 支持
1. 操作系统已分配 IPv6 地址
1. 当前网络支持 IPv6 路由

以上都妥当后，在配置文件中加入并配置以下两项即可：

	FPNN.server.ipv6.listening.ip = 
	FPNN.server.ipv6.listening.port =

其中 FPNN.server.ipv6.listening.ip 可不填。

当服务器启动后，将监听 IPv6 端口。

**注意**：FPNN 可同时监听 IPv4 和 IPv6 端口。




## * WebSocket 支持

WebSocket 需要打开 HTTP 支持：

	FPNN.server.http.supported = true

`FPNN.server.http.closeAfterAnswered` 参数不影响 WebSocket 行为。

WebSocket 支持 server push。

注：

+ WebSocket 控制帧中如果包含 payload，payload 将会被忽略。





## * 链接建立事件

在 IQuestProcessor 的派生类中重载 connected() 接口即可。

	virtual void connected(const ConnectionInfo&) {}

+ 在链接建立后该函数将被框架调用。
+ FPNN v2 中，对于加密链接，该函数会在秘钥协商前被调用。
+ 在该函数返回前，链接将被阻塞，不能收发网络数据。




## * 链接关闭事件

链接关闭事件，FPNN v1 与 v2 略有不同。v2 兼容 v1，但计划中的 v3 将不再兼容 v1。

1. FPNN v1

	FPNN v1 中，链接关闭事件分为正常关闭，和异常关闭。

		//-- 正常关闭
		virtual void connectionClose(const ConnectionInfo&) {}  //-- Deprecated. Will be dropped in FPNN.v3.
		//-- 异常关闭
		virtual void connectionErrorAndWillBeClosed(const ConnectionInfo&) {}  //-- Deprecated. Will be dropped in FPNN.v3.

	在 IQuestProcessor 的派生类中直接重载 connectionClose() 和 connectionErrorAndWillBeClosed() 接口即可。

	注：

	+ 如果需要处理关闭事件，两类关闭事件都必须处理，否则会有遗漏。
	+ FPNN v2 兼容该两接口，但不推荐继续使用。FPNN v3 将去除该两接口。

1. FPNN v2

	FPNN v2 中，新增统一接口：

		virtual void connectionWillClose(const ConnectionInfo& connInfo, bool closeByError);

	并兼容 v1 版两个旧接口。

	在 IQuestProcessor 的派生类中直接重载 connectionWillClose() 接口即可。

	注：

	+ FPNN v1 的 connectionWillClose() 与 FPNN v2 的 connectionClose() & connectionErrorAndWillBeClosed() 是互斥关系。
	+ v1 & v2 接口都重载的情况下，v1 版的接口将不被调用。




## * 服务启动事件

在 IQuestProcessor 的派生类中重载 start() 接口即可。

	virtual void start();




## * 服务即将停止事件

在 IQuestProcessor 的派生类中重载 serverWillStop() 接口即可。

	virtual void serverWillStop();

注：

+ 该函数返回后，服务器将终止收发信息。
+ 从 2.0.3 版开始，进入此状态后，服务器将拒绝新的链接(直接关闭)，并拒绝新的请求，返回 FPNN_EC_CORE_SERVER_STOPPING (20014) 错误。





## * 服务完全停止事件

在 IQuestProcessor 的派生类中重载 serverStopped() 接口即可。

	virtual void serverStopped();

注：此时服务器的工作线程池已经停止运行。




## * 自定义服务器状态信息

FPNN 框架内置的 *infos 接口回返回很多状态信息，但默认情况下 `APP.status` 部分为空。

如果需要在这部分返回业务自身的状态，在 IQuestProcessor 的派生类中重载 infos() 接口即可。

	virtual std::string infos(); //-- 请返回 Json 的 dict 对象。





## * 自定义动态调节参数

FPNN 框架内置的 *tune 接口可以动态调节服务框架的各种动态参数。  
如果需通过该接口，动态调节业务自身的动态参数，在 IQuestProcessor 的派生类中重载 tune() 接口即可。

	virtual void tune(const std::string& key, std::string& value);




## * 提前返回

对于 twoway 类型的请求，如果为了提高响应速度，需要在注册的接口函数执行完成前就返回应答数据，可以使用提前返回。

提前返回时：

1. 生成 FPAnswerPtr 对象；
1. 直接调用 IQuestProcessor 的 sendAnswer() 接口；
1. 注册的接口函数执行完毕时，返回 nullptr 即可。

	inline bool sendAnswer(const FPQuestPtr quest, FPAnswerPtr answer);

典型的使用场景：

	FPAnswerPtr DemoQuestProcessor::keepAlive(const FPReaderPtr args, const FPQuestPtr quest, const ConnectionInfo& ci)
	{
	    int64_t userId = args->wantInt("userId");
	  
	    FPAWriter aw(1, quest);
	    aw.param("status", true);
	    sendAnswer(quest, aw.take());
	 
	    //-- Do other things for keep alive.
	 
	    return nullptr;
	}

注意：

+ 一个请求处理流程中，sendAnswer() 仅能被调用一次。其后调用都将返回失败。
+ 如果在调用 sendAnswer() 成功前，接口函数返回（比如因异常等），接口函数需要返回有效的 FPAnswerPtr 对象，不能返回 nullptr。
+ 只有在 sendAnswer() 调用成功后，接口函数才能返回 nullptr。




## * 异步返回

对于 twoway 类型的请求，如果因为 I/O 操作，状态等待等，不能在接口函数中返回，或者需要在特定状态点返回，而不能在当前接口函数中返回的情况，适用于 FPNN 的异步返回。

异步返回步骤：

1. 调用 IQuestProcessor 的 genAsyncAnswer() 接口，生成 IAsyncAnswerPtr 异步返回器对象；
1. 保存并传递异步返回器对象；
1. 需要返回时，调用异步返回器对象的 sendAnswer() 接口；

相关接口：

	class IQuestProcessor
	{
	public:
	    inline IAsyncAnswerPtr genAsyncAnswer(const FPQuestPtr quest);
	    ... ...
	};

IAsyncAnswer 基础接口：

	class IAsyncAnswer
	    {
	    public:
	        virtual const FPQuestPtr getQuest();
	        virtual bool sendAnswer(FPAnswerPtr);
	        ... ...
	    };
	 
	typedef std::shared_ptr<IAsyncAnswer> IAsyncAnswerPtr;

IAsyncAnswer 完整接口：

	class IAsyncAnswer
	    {
	    public:
	        virtual const FPQuestPtr getQuest();
	        virtual bool sendAnswer(FPAnswerPtr);
	        virtual bool isSent();
	         
	        virtual void cacheTraceInfo(const std::string&, const std::string& raiser = "");
	        virtual void cacheTraceInfo(const char *, const char* raiser = "");
	        virtual bool sendErrorAnswer(int code = 0, const std::string& ex = "", const std::string& raiser = "");
	        virtual bool sendErrorAnswer(int code = 0, const char* ex = "", const char* raiser = "");
	        virtual bool sendEmptyAnswer();
	    };
	 
	typedef std::shared_ptr<IAsyncAnswer> IAsyncAnswerPtr;

+ **追踪异步返回异常**

	如果流程过于复杂，在异步返回前，整个流程可能就因异常或者其他条件而中断、结束。  
	这个时候，异步返回器对象，将自动返回一个错误号为 20001，内容为 “Answer is lost in normal logic. The error answer is sent for instead.” 的错误。  
	这个错误指明了异步返回流程被异常终止，应答数据未正常返回。但对快速有效的定位中断位置，并无帮助。  
	如果需要在流程的异常中断发生时，更加有效的定位中断位置，可以在流程的必要环节，调用 IAsyncAnswer / IAsyncAnswerPtr 对象的 cacheTraceInfo() 函数，写入必要的流程环节信息和状态信息。该信息会替代默认的 “Answer is lost in normal logic. The error answer is sent for instead.” 作为错误内容返回。  
	
	cacheTraceInfo() 可以多次被调用。新的状态信息将覆盖老的状态信息。  
	如果异步返回器的 sendAnswer() 接口被成功调用，则 cacheTraceInfo() 的状态信息将被忽略。




## * Server Push(双工/duplex)模式

1. 相关配置

	服务器默认支持向客户端推送 oneway 消息。  
	如果服务器要向客户端 推送 twoway 消息，则需要：

	* 调用服务器 enableAnswerCallbackThreadPool() 接口，启动 answer callback 处理线程池。

			class TCPEpollServer
			{
			public:
			    inline void enableAnswerCallbackThreadPool(int32_t initCount, int32_t perAppendCount, int32_t perfectCount, int32_t maxCount);
			    ... ...
			};

	或者

	* 配置文件加入

			FPNN.server.duplex.thread.min.size = <number>
			FPNN.server.duplex.thread.max.size = <number>



1. 流程

	1. 如果在注册的接口函数中 Push：

		直接调用 IQuestProcessor 的 sendQuest() 系列函数即可。  
		函数原型请参见 “接口” 部分。

	1. 如果在注册的接口函数之外 Push：

		1. 调用 IQuestProcessor 的 genQuestSender() 函数，生成请求发送器对象；
		1. 保存和传递请求发送器对象；
		1. 调用请求发送器对象的 sendQuest() 系列函数，发送请求。
		
		函数原型请参见 “接口” 部分。

1. 接口

	注意：

	以下接口中，FPNN v1 不包含 timeout 参数。timeout 参数仅存在于 FPNN v2 中。

		class IQuestProcessor
		{
		public:
		    inline QuestSenderPtr genQuestSender(const ConnectionInfo& connectionInfo);
		 
		    /**
		        All SendQuest():
		            If return false, caller must free quest & callback.
		            If return true, don't free quest & callback.
		    */
		    virtual FPAnswerPtr sendQuest(const ConnectionInfo& connectionInfo, FPQuestPtr quest, int timeout = 0);
		    virtual bool sendQuest(const ConnectionInfo& connectionInfo, FPQuestPtr quest, AnswerCallback* callback, int timeout = 0);
		    virtual bool sendQuest(const ConnectionInfo& connectionInfo, FPQuestPtr quest, std::function<void (FPAnswerPtr answer, int errorCode)> task, int timeout = 0);
		    ... ...
		};


		class QuestSender
		{
		public:
		    /**
		        All SendQuest():
		            If return false, caller must free quest & callback.
		            If return true, don't free quest & callback.
		    */
		    virtual FPAnswerPtr sendQuest(FPQuestPtr quest, int timeout = 0);
		    virtual bool sendQuest(FPQuestPtr quest, AnswerCallback* callback, int timeout = 0);
		    virtual bool sendQuest(FPQuestPtr quest, std::function<void (FPAnswerPtr answer, int errorCode)> task, int timeout = 0);
		};
		 
		typedef std::shared_ptr<QuestSender> QuestSenderPtr;


1. 示例

	1. 如果在注册的接口函数中 Push：

			FPAnswerPtr DemoQuestProcessor::exchange(const FPReaderPtr args, const FPQuestPtr quest, const ConnectionInfo& ci)
			{
			    //-- Do other things.
			    ... ...
			  
			    FPQWriter qw(2, "auth");
			    qw.param("user", "server");
			    qw.param("hash", "123ABC");
			    FPAnswerPtr answer = sendQuest(ci, qw.take());
			 
			    //-- Do other things.
			    ... ...
			 
			    return ...;
			}

	1. 如果在注册的接口函数之外 Push：

			FPAnswerPtr DemoQuestProcessor::demo(const FPReaderPtr args, const FPQuestPtr quest, const ConnectionInfo& ci)
			{
			    //-- Do other things.
			    ... ...
			  
			    QuestSenderPtr sender = genQuestSender(ci);
			    //-- codes for deliver sender to custom realQuestFunc(). Maybe including some async operations.
			    //-- Do other things.
			    ... ...
			 
			    return nullptr;
			}
			 
			... ...
			 
			void realQuestFunc(QuestSenderPtr sender)
			{
			    FPQWriter qw(2, "auth");
			    qw.param("user", "server");
			    qw.param("hash", "123ABC");
			    FPAnswerPtr answer = sender->sendQuest(qw.take());
			 
			    //-- Do other things.
			    ... ...
			}


## * 主动关闭连接（FPNN v2 Only）

直接调用 TCPEpollServer 的 closeConnection() 接口即可。

	class TCPEpollServer
	{
	public:
	    void closeConnection(const ConnectionInfo* ci);
	    void closeConnection(const ConnectionInfo& ci);
	    ... ...
	};


## * hook：请求的预处理 & 应答的后处理

1. 相关知识回顾

	* IQuestProcessor 包含 unknownMethod() 接口，可以接管处理对任何未注册接口的请求；
	* C++ 中，父类的非虚函数，子类可以覆盖；
	* FPNN 中，对于 twoway 类型的请求，存在正常返回，提前返回，异步返回；
	* FPNN 中，server push 可以在当前接口处理函数中，也可以在任意函数中。

1. 方案

	1. 采用双层 IQuestProcessor 派生类处理。

		server 使用第一层的 IQuestProcessor 派生类实例。  
		第一层的 IQuestProcessor 派生类不注册任何接口，则所有请求都将进入第一层派生类的 unknownMethod() 接口中。  
		第二层的 IQuestProcessor 派生类注册所有的业务接口，并在第一层派生类的 unknownMethod() 接口中，调用第二层派生类实例的 processQuest() 接口。调用前后即可 hook 处理所有的消息请求，以及正常流程的返回。	

	1. 派生 class QuestSender (任意函数中的 server push 请求发送)、class IAsyncAnswer (异步返回器) 两个对象。

		两个派生类须包含原始的对象实例。在进行完相应的 hook 操作后，向原始对象转发数据。

	1. 第二层的 IQuestProcessor 派生类定义 sendAnswer()、genAsyncAnswer()、genQuestSender()、sendQuest() 系列函数，覆盖父类同名函数。

		在派生类的函数中，进行完相应的 hook 操作后，需要向父类被覆盖的接口转发数据。

1. 示例

		class SelfQuestSender: public QuestSender
		{
		    QuestSenderPtr _realSender;
		 
		public:
		    SelfQuestSender(QuestSenderPtr sender): _realSender(sender) {}
		    virtual ~SelfQuestSender() {}
		 
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
		        //-- hook codes before server push quest to client
		        ... ...
		         
		        bool status = _realSender->sendQuest(quest, callback, timeout);
		 
		        //-- hook codes after server pushed quest to client
		        ... ...
		 
		        return status;
		    }
		     
		    virtual bool sendQuest(FPQuestPtr quest, std::function<void (FPAnswerPtr answer, int errorCode)> task, int timeout = 0)
		    {
		        //-- hook codes before server push quest to client
		        ... ...
		 
		        bool status = _realSender->sendQuest(quest, std::move(task), timeout);
		 
		        //-- hook codes after server pushed quest to client
		        ... ...
		 
		        return status;
		    }
		};
		 
		class SelfAsyncAnswer: public IAsyncAnswer
		{
		    IAsyncAnswerPtr _realAsync;
		 
		public:
		    SelfAsyncAnswer(IAsyncAnswerPtr async): _realAsync(async) {}
		    virtual ~SelfAsyncAnswer() {}
		 
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
		 
		    virtual void cacheTraceInfo(const std::string& info, const std::string& raiser = "") { return _realAsync->cacheTraceInfo(info, raiser); }
		    virtual void cacheTraceInfo(const char *info, const char* raiser = "") { return _realAsync->cacheTraceInfo(info, raiser); }
		         
		    virtual bool sendErrorAnswer(int code = 0, const std::string& ex = "", const std::string& raiser = "")
		    {
		        FPAnswerPtr answer = FPAWriter::errorAnswer(getQuest(), code, ex, raiser);
		        return sendAnswer(answer);
		    }
		    virtual bool sendErrorAnswer(int code = 0, const char* ex = "", const char* raiser = "")
		    {
		        FPAnswerPtr answer = FPAWriter::errorAnswer(getQuest(), code, ex, raiser);
		        return sendAnswer(answer);
		    }
		    virtual bool sendEmptyAnswer()
		    {
		        FPAnswerPtr answer = FPAWriter::emptyAnswer(getQuest());
		        return sendAnswer(answer);
		    }
		};
		 
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
		     
		    //-- hook 提前返回
		    bool sendAnswer(const FPQuestPtr quest, FPAnswerPtr answer)
		    {
		        //-- hook codes before send answer to client
		        ... ...
		 
		        bool status = IQuestProcessor::sendAnswer(quest, answer);
		 
		        //-- hook codes before send answer to client
		        ... ...
		 
		        return status;
		    }
		 
		    //-- hook 异步返回
		    IAsyncAnswerPtr genAsyncAnswer(const FPQuestPtr quest)
		    {
		        IAsyncAnswerPtr realAsync = IQuestProcessor::genAsyncAnswer(quest);
		        IAsyncAnswerPtr async = std::make_shared<SelfAsyncAnswer>(realAsync);
		        return async;
		    }
		 
		    //-- hook 任意位置的 server push
		    QuestSenderPtr genQuestSender(const ConnectionInfo& connectionInfo)
		    {
		        QuestSenderPtr realSender = IQuestProcessor::genQuestSender(connectionInfo);
		        QuestSenderPtr sender = std::make_shared<SelfQuestSender>(realSender);
		        return sender;
		    }
		 
		    //-- hook 接口处理函数内的 server push
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
		        //-- hook codes before server push quest to client
		        ... ...
		 
		        bool status = IQuestProcessor::sendQuest(connectionInfo, quest, callback, timeout);
		 
		        //-- hook codes after server pushed quest to client
		        ... ...
		 
		        return status;
		    }
		 
		    virtual bool sendQuest(const ConnectionInfo& connectionInfo, FPQuestPtr quest, std::function<void (FPAnswerPtr answer, int errorCode)> task, int timeout = 0)
		    {
		        //-- hook codes before server push quest to client
		        ... ...
		 
		        bool status = IQuestProcessor::sendQuest(connectionInfo, quest, std::move(task), timeout);
		 
		        //-- hook codes after server pushed quest to client
		        ... ...
		 
		        return status;
		    }
		};
		 
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
		 
		//--------- main func ---------
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

注：

+ 以上示例代码为 FPNN v2 示例代码。FPNN v1 请根据 IQuestProcessor.h 做对应修改。






## * IP 白名单（FPNN v2 Only）

如果要限制或者指定可以访问服务的来源地址，或者来源网段，可以启用IP白名单功能。

注：

为了安全起见，IP 白名单功能无法通过配置文件配置，需要手动调用 TCPEpollServer 的相关接口。

相关接口：

	class TCPEpollServer
	{
	public:
	    inline bool ipWhiteListEnabled();
	    inline void enableIPWhiteList(bool enable = true);
	    inline bool addIPToWhiteList(const std::string& ip);
	    inline bool addSubnetToWhiteList(const std::string& ip, int subnetMaskBits);
	    inline void removeIPFromWhiteList(const std::string& ip);
	    ... ...
	};

相关接口可以在任何时候调用，多线程安全。


注：

* IP 白名单默认加入 IPv4 & IPv6 本地环路地址。
* 如果启动白名单，但没有加入任何 IP，则内网 IP 及本机 IP（本地环路地址除外）均无法访问。
* 可以通过 *tune 接口动态启用／禁用 IP 白名单，以及动态添加／删除 IP 地址。





## * 加密模式（FPNN v2 Only）

FPNN v2 采用 ECC(椭圆曲线算法)进行秘钥协商，AES 算法的 CFB 模式进行加密。

所有秘钥协商和加解密操作对业务透明。

注：

* 一旦启用加密，则无论是全局加密，还是部分接口加密，还是可选加密，框架内置的 *tune 接口将强制要求加密访问。

1. 使用配置文件启用加密功能

	配置文件中增加以下条目

		FPNN.server.security.ecdh.enable = true
		 
		# curve 取值: secp256k1, secp256r1, secp224r1, secp192r1
		FPNN.server.security.ecdh.curve = <curve name>
		 
		# privateKey 指向 privateKey (binary format, not Hex) 的文件路径
		FPNN.server.security.ecdh.privateKey = <private key file path>


	privateKey 请使用 eccKeyMaker 生成。  
	eccKeyMaker 请参见 [FPNN 内置工具](fpnn-tools.md)。

	此时，加密已经启用，但处于非强制加密状态。加密链接和非加密链接都可以访问任何没有指定加密属性的接口。

	如果要所有接口都必须通过加密链接访问，则增加以下配置项：

		FPNN.server.security.forceEncrypt.userMethods = ture

	注：

	+ 任何情况下，框架内置的 *status 接口和 *infos 接口均可以在内网，以非加密方式访问。

	如果只是部分接口要求加密，请参见 “选择性加密”。

	如果要检查当前链接是否加密，请调用 ConnectionInfo 对象的 isEncrypted() 方法。


1. 使用接口启用加密功能

		class TCPEpollServer
		{
		public:
		    inline bool encrpytionEnabled();
		    inline bool enableEncryptor(const std::string& curve, const std::string& privateKey);
		    static void enableForceEncryption();
		    ... ...
		};

	注：

	+ enableEncryptor 接口的 privateKey 参数要求传入二进制数据，非 base64 或者 hex 之后的可视数据。
	+ 以上函数**必须**在**服务启动前**调用。


1. 选择性加密

	在注册接口时，registerMethod() 方法的 attributes 参数指定 EncryptOnly 即可。





## * 旁路请求数据

旁路请求数据给 FPNN 框架编写的服务，可选用 <fpnn-folder>/extends/Bypass.h 中的 Bypass 对象，旁路请求数据给 HTTP 接口，可选用 <fpnn-folder>/extends/HttpBypass.h 中的 HttpBypass 对象。

相关的配置项，请参见 <fpnn-folder>/extends/Bypass.h 或 <fpnn-folder>/extends/HttpBypass.h。

如果有定制化的旁路需求，请参见 <fpnn-folder>/extends/Bypass.h 或 <fpnn-folder>/extends/HttpBypass.h 中的相应实现。




## * 服务器集群

FPZKClient 模块提供向 FPZKServer 集群注册服务，或者获取指定集群状态的功能。

可选的配置项为

* 配置文件默认参数：

	+ 如果 create 函数 fpzkSrvList 参数为空，将默认读取参数：FPZK.client.fpzkserver_list  
	+ 如果 create 函数 projectName 参数为空，将默认读取参数：FPZK.client.project_name  
	+ 如果 create 函数 projectToken 参数为空，将默认读取参数：FPZK.client.project_token  
 
 
* 其他默认参数：
 
	+ FPNN.server.cluster.name

		服务器集群分组名称。若无，留为空。
	 
	+ FPZK.client.subscribe.enable = true (默认)

		- 如果 FPZK.client.subscribe.enable 为 true，支持服务器变动实时通知，但 FPZKClient 会启动两个线程。（仅对 FPZK v2 有效）
		- 如果 FPZK.client.subscribe.enable 为 false，2秒与 FPZK Server 同步一次，但 FPZKClient 只会启动一个线程。

	+ FPZK.client.sync.syncPublicInfo = false (默认)

		如果为 true，将额外汇报 domain、ipv4、ipv6、port、port6 (如果存在)

	+ FPZK.client.debugInfo.clusterChanged.enable = false

		如果为 true，将输出集群变动的 debug 信息。


FPZKClient 主要接口

	class FPZKClient
	{
	public:
	    static FPZKClientPtr create(const std::string& fpzkSrvList = "", const std::string& projectName = "", const std::string& projectToken = "");
	 
	    //-- 注册服务。如果仅获取其他集群的数据，则无需调用该接口。
	    bool registerService(const std::string& serviceName = "", const std::string& version = "", const std::string& endpoint = "", bool online = true);
	 
	    //-- 获取其他集群的状态数据
	    int64_t getServiceRevision(const std::string& serviceName, const std::string& cluster = "");
	    const ServiceInfosPtr getServiceInfos(const std::string& serviceName, const std::string& cluster = "", const std::string& version = "", bool onlineOnly = true);
	    std::vector<std::string> getServiceEndpoints(const std::string& serviceName, const std::string& cluster = "", const std::string& version = "", bool onlineOnly = true);
	    std::vector<std::string> getServiceEndpointsWithoutMyself(const std::string& version = "", bool onlineOnly = true);
	     
	    inline void monitorDetail(bool monitor);    //-- 获取 domain、ipv4、ipv6、port、port6 (如果存在) 等信息
	    inline void monitorDetail(bool monitor, const std::string& service);    //-- 获取 domain、ipv4、ipv6、port、port6 (如果存在) 等信息
	    inline void monitorDetail(bool monitor, const std::set<std::string>& detailServices); //-- 获取 domain、ipv4、ipv6、port、port6 (如果存在) 等信息
	    inline void monitorDetail(bool monitor, const std::vector<std::string>& detailServices);  //-- 获取 domain、ipv4、ipv6、port、port6 (如果存在) 等信息
	 
	    inline void setOnline(bool online);
	    inline void unregisterService();    //-- 取消注册
	};

具体请参见 <fpnn-folder>/extends/FPZKClient.h。





## * 集群消息路由&内部负载均衡

目前支持 一致性哈希(carp)、强一致性(Consistency，包含广播)、随机(Random)、轮流(Rotatory) 4 中消息路由及负载均衡方式。
具体请参见 [FPNN framework usage](../frame-usage.txt) “三、扩展” 的 “2. Proxies” 部分。


## * HTTP 海量并发访问

请使用  <fpnn-folder>/extends/MultipleURLEngine.h 中的 MultipleURLEngine 对象。

主要接口

	class MultipleURLEngine
	{
	public:
	    static bool init(bool initSSL = true, bool initCurl = true);
	    static void cleanup();
	    /**
	     *  If maxThreadCount <= 0, maxThreadCount will be assigned 1000.
	     *  If callbackPool is nullptr, MultipleURLEngine will use the answerCallbackPool of ClientEngine.
	     */
	    MultipleURLEngine(int nConnsInPerThread = 200, int initThreadCount = 1, int perfectThreadCount = 10,
	                        int maxThreadCount = 100, int maxConcurrentCount = 25000,
	                        int tempThreadLatencySeconds = 120,
	                        std::shared_ptr<TaskThreadPool> callbackPool = nullptr);
	 
	    //-- 同步调用
	    bool visit(const std::string& url, Result &result, int timeoutSeconds = 120,
	        const std::string& postBody = std::string());
	    /* If return true, don't cleanup curl; if return false, please cleanup curl. */
	    bool visit(CURL *curl, Result &result, int timeoutSeconds = 120,
	        bool saveResponseData = false, const std::string& postBody = std::string());
	 
	    //-- 异步调用
	    bool visit(const std::string& url, ResultCallbackPtr callback, int timeoutSeconds = 120,
	        const std::string& postBody = std::string());
	    /* If return true, don't cleanup curl; if return false, please cleanup curl. */
	    inline bool visit(CURL *curl, ResultCallbackPtr callback, int timeoutSeconds = 120,
	        bool saveResponseData = false, const std::string& postBody = std::string());
	 
	    bool visit(const std::string& url, std::function<void (Result &result)> callback,
	        int timeoutSeconds = 120, const std::string& postBody = std::string());
	    /* If return true, don't cleanup curl; if return false, please cleanup curl. */
	    inline bool visit(CURL *curl, std::function<void (Result &result)> callback,
	        int timeoutSeconds = 120,
	        bool saveResponseData = false, const std::string& postBody = std::string());
	 
	    //-- 批量异步调用
	    bool addToBatch(const std::string& url, ResultCallbackPtr callback, int timeoutSeconds = 120,
	        const std::string& postBody = std::string());
	    /* If return true, don't cleanup curl; if return false, please cleanup curl. */
	    inline bool addToBatch(CURL *curl, ResultCallbackPtr callback, int timeoutSeconds = 120,
	        bool saveResponseData = false, const std::string& postBody = std::string());
	 
	    bool addToBatch(const std::string& url, std::function<void (Result &result)> callback,
	        int timeoutSeconds = 120, const std::string& postBody = std::string());
	    /* If return true, don't cleanup curl; if return false, please cleanup curl. */
	    inline bool addToBatch(CURL *curl, std::function<void (Result &result)> callback,
	        int timeoutSeconds = 120,
	        bool saveResponseData = false, const std::string& postBody = std::string());
	     
	    bool commitBatch();
	};

具体实例，可参见 <fpnn-folder>/extends/test/ 目录下

+ MultipleURLEngineBasicTest.cpp
+ MultipleURLEngineMoreConnectionInfo.cpp
+ MultipleURLEngineBatchTest.cpp

三个例子。
