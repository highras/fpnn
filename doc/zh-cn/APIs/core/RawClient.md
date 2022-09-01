## RawClient

### 介绍

[RawTCPClient](RawTCPClient.md) 和 [RawUDPClient](RawUDPClient.md) 的基类。提供客户端的通用方法。

**注意：请勿使用非文档化的 API。**  
非文档化的 API，或为内部使用，或因历史原因遗留，或为将来设计，后续版本均可能存在变动。

### 命名空间

	namespace fpnn;

### 关键定义

	class RawClient
	{
	public:
		RawClient(const std::string& host, int port, bool autoReconnect = true);
		virtual ~RawClient();

		inline bool connected();
		inline const std::string& endpoint();
		inline int socket();
		inline ConnectionInfoPtr connectionInfo();

		inline void setQuestProcessor(IQuestProcessorPtr processor);
		inline void setRawDataProcessor(IRawDataProcessorPtr processor);

		virtual bool connect() = 0;
		virtual void close();
		virtual bool reconnect();

		virtual bool sendData(std::string* data) = 0;

		static RawTCPClientPtr createRawTCPClient(const std::string& host, int port, bool autoReconnect = true);
		static RawTCPClientPtr createRawTCPClient(const std::string& endpoint, bool autoReconnect = true);

		static RawUDPClientPtr createRawUDPClient(const std::string& host, int port, bool autoReconnect = true);
		static RawUDPClientPtr createRawUDPClient(const std::string& endpoint, bool autoReconnect = true);
	};

	typedef std::shared_ptr<RawClient> RawClientPtr;

### 构造函数

	RawClient(const std::string& host, int port, bool autoReconnect = true);

**参数说明**

* **`const std::string& host`**

	服务器 IP 地址或者域名。

* **`int port`**

	服务器端口。

* **`bool autoReconnect`**

	启用或者禁用自动重连功能。

### 成员函数

#### connected

	inline bool connected();

判断链接是否已经建立。

#### endpoint

	inline const std::string& endpoint();

返回要链接的对端服务器的 endpoint。

#### socket

	inline int socket();

返回当前 socket。

#### connectionInfo

	inline ConnectionInfoPtr connectionInfo();

返回当前的连接信息对象 [ConnectionInfo](ConnectionInfo.md)。

**注意**

每次链接，连接信息对象均不相同。


#### setQuestProcessor

	inline void setQuestProcessor(IQuestProcessorPtr processor);

配置链接事件和 Server Push 请求处理模块。具体可参见 [IQuestProcessor](IQuestProcessor.md)。



#### setRawDataProcessor

	inline setRawDataProcessor(IRawDataProcessorPtr processor);

配置 Raw 客户端处理接收到的数据的对象。具体可参见 [IRawDataProcessor](IRawDataProcessor.md)。



#### connect

	virtual bool connect() = 0;

同步连接服务器。

#### close

	virtual void close();

关闭当前连接。

#### reconnect

	virtual bool reconnect();

同步重连。

#### sendData

	virtual bool sendData(std::string* data) = 0;

发送数据。异步发送。

**参数说明**

* **`std::string* data`**

	需要发送的数据对象。可以是纯二进制数据。

	**注意**

	如果函数返回 false，**须要**用户负责回收 data 所指向内存；如果函数返回 true，则框架**接管** data 所指向内存，并在必要时，框架内部使用 delete 关键字进行回收。



#### createRawTCPClient

	static RawTCPClientPtr createRawTCPClient(const std::string& host, int port, bool autoReconnect = true);
	static RawTCPClientPtr createRawTCPClient(const std::string& endpoint, bool autoReconnect = true);

创建 Raw TCP 客户端。

**参数说明**

* **`const std::string& host`**

	服务器 IP 或者域名。

* **`int port`**

	服务器端口。

* **`const std::string& endpoint`**

	服务器 endpoint。

* **`bool autoReconnect`**

	是否自动重连。

#### createRawUDPClient

	static RawUDPClientPtr createRawUDPClient(const std::string& host, int port, bool autoReconnect = true);
	static RawUDPClientPtr createRawUDPClient(const std::string& endpoint, bool autoReconnect = true);

创建 Raw UDP 客户端。

**参数说明**

* **`const std::string& host`**

	服务器 IP 或者域名。

* **`int port`**

	服务器端口。

* **`const std::string& endpoint`**

	服务器 endpoint。

* **`bool autoReconnect`**

	是否自动重连。
