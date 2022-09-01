## RawUDPClient

### 介绍

Raw UDP 客户端。[RawClient](RawClient.md) 的子类。

**注意：请勿使用非文档化的 API。**  
非文档化的 API，或为内部使用，或因历史原因遗留，或为将来设计，后续版本均可能存在变动。

### 命名空间

	namespace fpnn;

### 关键定义

	class RawUDPClient: public RawClient, public std::enable_shared_from_this<RawUDPClient>
	{
	public:
		virtual ~RawUDPClient() { close(); }

		inline void enableRawDataProcessPool(int32_t initCount, int32_t perAppendCount, int32_t perfectCount, int32_t maxCount);

		virtual bool connect();
		virtual void close();

		virtual bool sendData(std::string* data);

		inline static RawUDPClientPtr createClient(const std::string& host, int port, bool autoReconnect = true);
		inline static RawUDPClientPtr createClient(const std::string& endpoint, bool autoReconnect = true);
	};


	typedef std::shared_ptr<RawUDPClient> RawUDPClientPtr;

### 创建与构造

UDPClient 的构造函数为私有成员，无法直接调用。请使用静态成员函数

	inline static RawUDPClientPtr RawUDPClient::createClient(const std::string& host, int port, bool autoReconnect = true);
	inline static RawUDPClientPtr RawUDPClient::createClient(const std::string& endpoint, bool autoReconnect = true);

或基类静态成员函数

	static RawUDPClientPtr RawClient::createRawUDPClient(const std::string& host, int port, bool autoReconnect = true);
	static RawUDPClientPtr RawClient::createRawUDPClient(const std::string& endpoint, bool autoReconnect = true);

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

本文档仅列出基于基类所扩展的成员函数，其余成员函数请参考基类文档 [RawClient](RawClient.md)。

#### enableRawDataProcessPool

	inline void enableRawDataProcessPool(int32_t initCount, int32_t perAppendCount, int32_t perfectCount, int32_t maxCount);

配置当前实例私有的、独立的、数据接收事件和链接关闭事件线程池。默认将使用 ClientEngine 提供的应答处理和 Callback 执行的任务线程池。

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
