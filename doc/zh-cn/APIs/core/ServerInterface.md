## ServerInterface

TCP 服务器 [TCPEpollServer](TCPEpollServer.md) 和 UDP 服务器 [UDPEpollServer](UDPEpollServer.md) 的基类。提供服务器模块的通用方法。

### 命名空间

	namespace fpnn;

### 关键定义

	class ServerInterface
	{
	public:
		virtual ~ServerInterface() {}

		virtual bool startup() = 0;
		virtual void run() = 0;
		virtual void stop() = 0;

		virtual void setIP(const std::string& ip) = 0;
		virtual void setIPv6(const std::string& ipv6) = 0;
		virtual void setPort(unsigned short port) = 0;
		virtual void setPort6(unsigned short port) = 0;
		virtual void setBacklog(int backlog) = 0;

		virtual unsigned short port() const = 0;
		virtual unsigned short port6() const = 0;
		virtual std::string ip() const = 0;
		virtual std::string ipv6() const = 0;

		virtual void setQuestProcessor(IQuestProcessorPtr questProcessor) = 0;
		virtual IQuestProcessorPtr getQuestProcessor() = 0;

		virtual std::string workerPoolStatus() = 0;
		virtual std::string answerCallbackPoolStatus() = 0;
	};
	
	typedef std::shared_ptr<ServerInterface> ServerPtr;

### 成员函数

#### startup

	virtual bool startup();

启动服务器。

#### run

	virtual void run();

服务器主循环。

#### stop

	virtual void stop();

停止服务器。

#### setIP

	virtual void setIP(const std::string& ip);

设置需要绑定的 IP v4 地址。

#### setIPv6

	virtual void setIPv6(const std::string& ipv6);

设置需要绑定的 IP v6 地址。

#### setPort

	virtual void setPort(uint16_t port);

设置需要绑定的 IP v4 端口。

#### setPort6

	virtual void setPort6(unsigned short port);

设置需要绑定的 IP v6 端口。

#### setBacklog

	virtual void setBacklog(int backlog);

设置 accept 等候队列长度。

#### port

	virtual uint16_t port() const;

获取绑定的 IP v4 端口。

#### port6

	virtual uint16_t port6() const;

获取绑定的 IP v6 端口。

#### ip

	virtual std::string ip() const;

获取绑定的 IP v4 地址。

#### ipv6

	virtual std::string ipv6() const;

获取绑定的 IP v6 地址。

#### setQuestProcessor

	virtual void setQuestProcessor(IQuestProcessorPtr questProcessor);

配置服务器事件和请求处理模块。具体可参见 [IQuestProcessor](IQuestProcessor.md)。

#### getQuestProcessor

	virtual IQuestProcessorPtr getQuestProcessor();

获取服务器事件和请求处理模块。具体可参见 [IQuestProcessor](IQuestProcessor.md)。

#### workerPoolStatus

	virtual std::string workerPoolStatus();

返回 FPNN Server 的工作线程池的状态。Json 格式。

#### answerCallbackPoolStatus

	virtual std::string answerCallbackPoolStatus();

返回 FPNN Server 处理 Server Push 应答的线程池状态。Json 格式。

