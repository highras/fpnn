## UDPEpollServer

### 介绍

UDP 服务器，基于 Epoll 实现。

[ServerInterface](ServerInterface.md) 的子类。

文件配置可参见 [conf.template](../../../conf.template) 相关条目。

**注意**

UDPEpollServer 需要 Linux 内核 3.9 及以上，方能有性能测试中的性能体现。

### 命名空间

	namespace fpnn;

### 关键定义

	class UDPEpollServer: virtual public ServerInterface
	{
	public:
		virtual ~UDPEpollServer();

		static UDPServerPtr create();

		static UDPEpollServer* nakedInstance();
		static UDPServerPtr instance();

		virtual bool startup();
		virtual void run();
		virtual void stop();

		virtual void setIP(const std::string& ip);
		virtual void setIPv6(const std::string& ipv6);
		virtual void setPort(unsigned short port);
		virtual void setPort6(unsigned short port);
		virtual void setBacklog(int backlog);

		virtual uint16_t port() const;
		virtual uint16_t port6() const;
		virtual std::string ip() const;
		virtual std::string ipv6() const;

		virtual void setQuestProcessor(IQuestProcessorPtr questProcessor);
		virtual IQuestProcessorPtr getQuestProcessor();
		inline void setQuestTimeout(int64_t seconds);
		inline int64_t getQuestTimeout();

		inline int currentConnections();
		inline int validARQConnections();

		inline void configWorkerThreadPool(int32_t initCount, int32_t perAppendCount, int32_t perfectCount, int32_t maxCount, size_t maxQueueSize);
		inline void enableAnswerCallbackThreadPool(int32_t initCount, int32_t perAppendCount, int32_t perfectCount, int32_t maxCount);
		virtual std::string workerPoolStatus();
		virtual std::string answerCallbackPoolStatus();

		static void enableForceEncryption();
		inline bool encrpytionEnabled();
		inline bool enableEncryptor(const std::string& curve, const std::string& privateKey);

		inline bool ipWhiteListEnabled();
		inline void enableIPWhiteList(bool enable = true);
		inline bool addIPToWhiteList(const std::string& ip);
		inline bool addSubnetToWhiteList(const std::string& ip, int subnetMaskBits);
		inline void removeIPFromWhiteList(const std::string& ip);

		void closeConnection(int socket, bool force = false);
	};

	typedef std::shared_ptr<UDPEpollServer> UDPServerPtr;

**注意**

代码中，`class UDPEpollServer` 的非文档化基类，和非文档化的接口可能随时会更改。因此，请仅使用本文档中，**明确列出**的接口。

### 成员函数

#### create

	static UDPServerPtr create();

创建 UDPEpollServer 实例。

目前 FPNN Framework 是单例。如果 UDPEpollServer 已经创建，将返回已创建的实例。

#### nakedInstance

	static UDPEpollServer* nakedInstance();

获取 UDPEpollServer 单例的裸指针。如果单例没有被创建，则返回 `NULL`。

#### instance

	static UDPServerPtr instance();

获取 UDPEpollServer 单例的智能指针。如果单例没有被创建，则返回 `nullptr`。

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

#### setQuestTimeout

	inline void setQuestTimeout(int64_t seconds);

设置 TCP Server Push 请求超时的全局默认值。

#### getQuestTimeout

	inline int64_t getQuestTimeout();

获取 TCP Server Push 请求超时的全局默认值。

#### currentConnections

	inline int currentConnections();

获取当前服务器 UDP 链接的数量。

#### validARQConnections

	inline int currentConnections();

获取当前服务器有效的 ARQ 链接的数量。

#### configWorkerThreadPool

	inline void configWorkerThreadPool(int32_t initCount, int32_t perAppendCount, int32_t perfectCount, int32_t maxCount, size_t maxQueueSize);

配置 UDPEpollServer 的工作线程池。

**注意**

+ 一般情况采用默认配置即可。
+ 该接口必须在 UDPEpollServer 启动前调用。否则将会被忽略。
+ 如果在显示调用该接口，则配置文件中的相关配置将被忽略。

**参数说明**

+ **`int32_t initCount`**

	初始的线程数量。

+ **`int32_t perAppendCount`**

	追加线程时，单次的追加线程数量。

	**注意**，当线程数超过 `perfectCount` 的限制后，单次追加数量仅为 `1`。

+ **`int32_t perfectCount`**

	线程池常驻线程的最大数量。

+ **`int32_t maxCount`**

	线程池线程最大数量。如果为 `0`，则表示不限制。

#### enableAnswerCallbackThreadPool

	inline void enableAnswerCallbackThreadPool(int32_t initCount, int32_t perAppendCount, int32_t perfectCount, int32_t maxCount);

配置 UDPEpollServer 处理 Server Push 应答的线程池。

**注意**

+ 一般情况采用默认配置即可。
+ 该接口必须在 UDPEpollServer 启动前调用。否则将会被忽略。
+ 如果在显示调用该接口，则配置文件中的相关配置将被忽略。

**参数说明**

+ **`int32_t initCount`**

	初始的线程数量。

+ **`int32_t perAppendCount`**

	追加线程时，单次的追加线程数量。

	**注意**，当线程数超过 `perfectCount` 的限制后，单次追加数量仅为 `1`。

+ **`int32_t perfectCount`**

	线程池常驻线程的最大数量。

+ **`int32_t maxCount`**

	线程池线程最大数量。如果为 `0`，则表示不限制。


#### workerPoolStatus

	virtual std::string workerPoolStatus();

返回 UDPEpollServer 的工作线程池的状态。Json 格式。

#### answerCallbackPoolStatus

	virtual std::string answerCallbackPoolStatus();

返回 UDPEpollServer 处理 Server Push 应答的线程池状态。Json 格式。

#### encrpytionEnabled

	inline bool encrpytionEnabled();

判断 TCP 服务器是否启动了加密。

#### enableEncryptor

	inline bool enableEncryptor(const std::string& curve, const std::string& privateKey);

启用链接加密。

**参数说明**

* **`const std::string& curve`**

	ECDH (椭圆曲线密钥交换) 所用曲线名称。

	可用值：

	+ "secp256k1"
	+ "secp256r1"
	+ "secp224r1"
	+ "secp192r1"

* **`const std::string& privateKey`**

	服务器私钥（二进制数据）。

	**注意**

	该私钥为裸密钥，由 FPNN 框架内置工具 [eccKeyMaker](../../fpnn-tools.md#eccKeyMaker) 生成。

#### enableForceEncryption

	static void enableForceEncryption();

强制所有接口必须加密访问。


#### ipWhiteListEnabled

	inline bool ipWhiteListEnabled();

判断白名单是否启用。

#### enableIPWhiteList

	inline void enableIPWhiteList(bool enable = true);

启用或者禁用白名单。

#### addIPToWhiteList

	inline bool addIPToWhiteList(const std::string& ip);

添加 IP 地址 （IP v4 或者 IP v6）到白名单中。

#### addSubnetToWhiteList

	inline bool addSubnetToWhiteList(const std::string& ip, int subnetMaskBits);

添加 IP v4 网段到白名单中。

**参数说明**

* **`const std::string& ip`**

	网段 IP。

* **`int subnetMaskBits`**

	子网掩码位数。

#### removeIPFromWhiteList

	inline void removeIPFromWhiteList(const std::string& ip);

将指定 IP 地址从白名单中删除。

#### closeConnection

	void closeConnection(int socket, bool force = false);

关闭指定的 UDP 链接。

**参数说明**

* **`bool force`**

	是否强制关闭。强制关闭不会向客户端发送关闭信号。
	