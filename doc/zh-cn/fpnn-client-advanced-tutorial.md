# FPNN Client Advanced Tutorial

[TOC]

## * 请求超时

FPNN 中，客户端一共有3类请求超时。分别是 全局配置中的请求超时，client 实例的请求超时，sendQuest() 接口的请求超时。  
如果sendQuest() 的请求超时为 0，则应用 client 实例的请求超时；  
如果 client 实例的请求超时设置为 0，则应用全局配置的请求超时。

配置 TCPClient 实例的请求超时

	class TCPClient
	{
	public:
	    void setQuestTimeout(int64_t seconds);
	};

注：TCPClient 的配置类接口非线程安全，需要在收发数据前，配置完成。

配置全局的请求超时

	class ClientEngine
	{
	public:
	    static void setQuestTimeout(int64_t seconds);
	};

或通过配置文件配置

	FPNN.client.quest.timeout = <number>




## * 日志配置

如果是独立的客户端程序，未和服务器一同运行(在同一进程内)，则框架默认会将日志直接输出至显示终端。

配置日志模块，须根据需要，调用以下代码：

	#include "FPLog.h"
	 
	class FPLog
	{
	public:
	    static FPLogBasePtr init(std::ostream& out, const std::string& route, const std::string& level = "ERROR", const std::string& serverName = "unknown", size_t maxQueueSize = FPLOG_DEFAULT_MAX_QUEUE_SIZE);
	    static FPLogBasePtr init(const std::string& endpoint, const std::string& route, const std::string& level = "ERROR", const std::string& serverName = "unknown", size_t maxQueueSize = FPLOG_DEFAULT_MAX_QUEUE_SIZE);
	    static void setLevel(const std::string& logLevel);
	};


## * 链接建立事件

如果需要处理链接建立事件，需要向 TCPClient 指派 IQuestProcessor 的实例。  
该实例只需在 IQuestProcessor 的派生类中重载 connected() 接口即可。

	class IQuestProcessor
	{
	public:
	    virtual void connected(const ConnectionInfo&) {}
	};

TCPClient 的接口

	class TCPClient
	{
	public:
	    void setQuestProcessor(IQuestProcessorPtr processor);
	};

注：TCPClient 的配置类接口非线程安全，需要在收发数据前，配置完成。




## * 链接关闭事件

如果需要处理链接关闭事件，需要向 TCPClient 指派 IQuestProcessor 的实例。
该实例只需在 IQuestProcessor 的派生类中重载 connectionWillClose() 接口即可。

	
	class IQuestProcessor
	{
	public:
	    virtual void connectionWillClose(const ConnectionInfo& connInfo, bool closeByError);
	};

TCPClient 的接口

	class TCPClient
	{
	public:
	    void setQuestProcessor(IQuestProcessorPtr processor);
	};

注：TCPClient 的配置类接口非线程安全，需要在收发数据前，配置完成。




## * 加密链接

TCPClient 的接口

	class TCPClient
	{
	public:
		bool enableSSL(bool enable = true);
	    void enableEncryptor(const std::string& curve, const std::string& peerPublicKey, bool packageMode = true, bool reinforce = false);
	    bool enableEncryptorByDerData(const std::string &derData, bool packageMode = true, bool reinforce = false);
	    bool enableEncryptorByPemData(const std::string &PemData, bool packageMode = true, bool reinforce = false);
	    bool enableEncryptorByDerFile(const char *derFilePath, bool packageMode = true, bool reinforce = false);
	    bool enableEncryptorByPemFile(const char *pemFilePath, bool packageMode = true, bool reinforce = false);
	};

+ enableSSL() 为启用 SSL/TLS 链接。

	**注**：在同一连接上，SSL/TLS 与 FPNN 自身的加密不可同时开启。为了防止无谓的消耗系统资源，FPNN 不支持冗余的加密。

+ curve 为服务器采用的椭圆曲线名称，有四个选项：

	secp192r1、secp224r1、secp256r1、secp256k1。

+ peerPublicKey 为服务器的公钥。该参数要求传入二进制数据，非 base64 或者 hex 之后的可视数据。
+ packageMode 为加密模式。true 表示采用包加密模式，false 表示采用流加密模式。
+ reinforce 为加密强度。true 表示采用 256 bits 秘钥，false 表示采用 128 位秘钥。

注：TCPClient 的配置类接口非线程安全，需要在收发数据前，配置完成。




## * 双工(duplex)模式

如果客户端启动了双工模式，客户端便可以处理服务器发过来的请求，并进行应答。

客户端默认不会启动双工模式。启动双工模式需要两个步骤

1. 实现并注册服务器将调用的接口

	实现并注册服务器将调用的接口需要向 TCPClient 指派 IQuestProcessor 的实例，并在实例中注册服务器将调用的接口。  
	接口的注册和使用，与服务端完全相同。请参考 [FPNN 服务端基础使用说明](fpnn-server-basic-tutorial.md) 的“添加接口方法” 部分。

	TCPClient 的相关接口

		class TCPClient
		{
		public:
		    void setQuestProcessor(IQuestProcessorPtr processor);
		};

	注：TCPClient 的配置类接口非线程安全，需要在收发数据前，配置完成。


1. 启动客户端请求处理线程池

	服务器端 push 过来的请求需要线程池处理，但因客户端默认不会启动双工，所以也不会启动请求处理线程池。

	请求处理线程池的启动有两种方法：  
	通过配置文件配置，或者调用API启动。

	配置文件配置，需增加以下两项：

		FPNN.client.duplex.thread.min.size = <number>
		FPNN.client.duplex.thread.max.size = <number>

	调用 API 为：

		class ClientEngine
		{
		public:
		    static void configQuestProcessThreadPool(int32_t initCount, int32_t perAppendCount, int32_t perfectCount, int32_t maxCount, size_t maxQueueSize);
		};

	注：

	+ [FPNN framework usage](../frame-usage.txt) 中提到的第三种方法，调用 TCPClient 的 enableQuestProcessThreadPool() 接口，启动客户端私有请求处理线程池。或调用 TCPClient 的 setQuestProcessThreadPool() 接口等，如果无必要，请勿采用该类方法。  
	TCPClient 实例的私有线程池为特定目的存在，需要仔细处理。如无必要，请勿启用。

* duplex 的提前返回

	具体细节与服务端的提前返回完全相同，请参考 [FPNN 服务端高级使用说明](fpnn-server-advanced-tutorial.md) 相关部分。


* duplex 的异步返回

	具体细节与服务端的异步返回完全相同，请参考 [FPNN 服务端高级使用说明](fpnn-server-advanced-tutorial.md) 相关部分。


	+ 追踪异步返回异常

		具体细节与服务端的异步返回完全相同，请参考 [FPNN 服务端高级使用说明](fpnn-server-advanced-tutorial.md) 相关部分。

