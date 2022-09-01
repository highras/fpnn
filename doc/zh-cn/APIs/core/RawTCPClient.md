## RawTCPClient

### 介绍

Raw TCP 客户端。[RawClient](RawClient.md) 的子类。

**注意：请勿使用非文档化的 API。**  
非文档化的 API，或为内部使用，或因历史原因遗留，或为将来设计，后续版本均可能存在变动。

### 命名空间

	namespace fpnn;

### 关键定义

	class RawTCPClient: public RawClient, public std::enable_shared_from_this<RawTCPClient>
	{
	public:
		virtual ~RawTCPClient();

		inline void setIOChunckSize(int ioChunkSize);
		virtual bool connect();
		virtual bool sendData(std::string* data);

		inline static RawTCPClientPtr createClient(const std::string& host, int port, bool autoReconnect = true);
		inline static RawTCPClientPtr createClient(const std::string& endpoint, bool autoReconnect = true);
	};

	typedef std::shared_ptr<RawTCPClient> RawTCPClientPtr;

### 创建与构造

RawTCPClient 的构造函数为私有成员，无法直接调用。请使用静态成员函数

	inline static RawTCPClientPtr RawTCPClient::createClient(const std::string& host, int port, bool autoReconnect = true);
	inline static RawTCPClientPtr RawTCPClient::createClient(const std::string& endpoint, bool autoReconnect = true);

或基类静态成员函数

	static RawTCPClientPtr RawClient::createRawTCPClient(const std::string& host, int port, bool autoReconnect = true);
	static RawTCPClientPtr RawClient::createRawTCPClient(const std::string& endpoint, bool autoReconnect = true);

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

#### setIOChunckSize

	inline void setIOChunckSize(int ioChunkSize);

设置在接收数据时，接收用的链式缓存 (ChainBuffer)[../base/ChainBuffer.md] 单位缓存块的大小。
