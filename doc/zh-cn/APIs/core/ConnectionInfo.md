## ConnectionInfo

### 介绍

ConnectionInfo 连接信息对象。

### 命名空间

	namespace fpnn;

### 关键定义

	class ConnectionInfo
	{
	public:
		uint64_t token;
		int socket;
		int port;
		std::string ip;
		uint32_t ipv4;

		std::string str() const;
		std::string endpoint() const;

		inline bool isSSL() const;
		inline bool isTCP() const;
		inline bool isUDP() const;
		inline bool isIPv4() const;
		inline bool isIPv6() const;
		inline bool isEncrypted() const;
		inline bool isPrivateIP() const;
		inline bool isWebSocket() const;
		inline bool isServerConnection() const;
		inline uint64_t getToken() const;
		inline uint64_t uniqueId() const;
		std::string getIP() const;

		ConnectionInfo(const ConnectionInfo& ci);
		~ConnectionInfo();
	};

	typedef std::shared_ptr<ConnectionInfo> ConnectionInfoPtr;

### 构造函数

	ConnectionInfo(const ConnectionInfo& ci);

ConnectionInfo 对象无法直接生成。

**拷贝构造函数**生成的副本，仅供业务缓存和跟踪使用。FPNN Framework 内部相关接口，大部分需要使用原本实例。

### 公有域

#### token

对应的连接的**当前唯一**标识。

**注意**

+ `token` 为**当前**唯一，非**全局**唯一。全剧唯一，请调用 `uint64_t uniqueId()` 接口获取。
+ 当且仅当：
	+ 对应链接的链接建立事件开始被调用
	+ 对应链接的链接关闭事件返回前

	时，`token` 当前/全局唯一。

#### socket

Socket 套接字。

#### port

端口。

#### ip

域名解析后的 IP 地址。

#### ipv4

如果是 IPv4 链接，则为 IP v4 地址网络字节序的二进制表示。否则为无效数据。

### 成员函数

#### str

	std::string str() const;

返回 socket 与 endpoint 组成的字符串。

#### endpoint

	std::string endpoint() const;

Endpoint 接入点。

#### isSSL

	inline bool isSSL() const;

是否是 SSL/TLS 链接。

#### isTCP

	inline bool isTCP() const;

是否是 TCP 连接。

#### isUDP

	inline bool isUDP() const;

是否是 UDP 连接。

#### isIPv4

	inline bool isIPv4() const;

是否是 IP v4 链接。

#### isIPv6

	inline bool isIPv6() const;

是否是 IP v6 链接。

#### isEncrypted

	inline bool isEncrypted() const;

是否是加密链接。

#### isPrivateIP

	inline bool isPrivateIP() const;

是否是内网链接。

#### isWebSocket

	inline bool isWebSocket() const;

是否是 WebSocket 链接。

#### isServerConnection

	inline bool isServerConnection() const;

是否是服务端链接。

#### getToken

	inline uint64_t getToken() const;

获取链接 [token](#token)。请参见公有域 [token](#token)。

#### uniqueId

	inline uint64_t uniqueId();

返回**对应连接**的**全局唯一**的 id。

#### getIP

	std::string getIP() const;

返回域名解析后的 IP 地址。参见公有域 [ip](#ip)。

