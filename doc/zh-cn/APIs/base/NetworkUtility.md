## NetworkUtility

### 介绍

常用网络函数库。

### 命名空间

	namespace fpnn;

	namespace fpnn::NetworkUtil;

### fpnn 命名空间

#### nonblockedFd

	bool nonblockedFd(int fd);

将 fd/socket 设置为非阻塞模式。

#### IPV4ToString

	std::string IPV4ToString(uint32_t internalAddr);

将二进制形式的 IP v4 地址转成 '.' 分割的字符串形式。

#### checkIP4

	bool checkIP4(const std::string& ip);

检查是否是合法的 IP v4 地址。

#### EndPointType

	enum EndPointType{
		ENDPOINT_TYPE_IP4 = 1,
		ENDPOINT_TYPE_IP6 = 2,
		ENDPOINT_TYPE_DOMAIN = 3,
	};

Endpoint 类型枚举。

#### parseAddress

	bool parseAddress(const std::string& address, std::string& host, int& port);
	bool parseAddress(const std::string& address, std::string& host, int& port, EndPointType& eType);

将 endpoint 分解为 IP/域名 和 端口 两部分，并返回 endpoint/IP/域名 的具体类型。

#### getIPAddress

	bool getIPAddress(const std::string& hostname, std::string& IPAddress, EndPointType& eType);

域名解析，并返回解析后的IP类型。

**注意**
如果域名可解析为 IPv4 类型，则该函数将不再做 IPv6 解析。除非 IPv4 解析失败，才会进行 IPv6 解析。

**参数说明**

* **`const std::string& hostname`**

	需要解析的域名。

* **`std::string& IPAddress`**

	解析出的 IP 地址。

* **`EndPointType& eType`**

	解析出的 IP 地址的类型。

#### IPTypes

	enum IPTypes {
		IPv4_Public,
		IPv4_Local,
		IPv4_Loopback,
		IPv6_Global,
		IPv6_LinkLocal,
		IPv6_SiteLocal,
		IPv6_Multicast,
		IPv6_Loopback
	};

IP 类型枚举。

#### getIPs

	bool getIPs(std::map<enum IPTypes, std::set<std::string>>& ipDict);

获取本机所有 IP 及其类型。

**注意**

+ 一个类型可能含有多个IP数据。
+ 云主机可能无法获取到云主机提供商通过其他网络设备，映射的 IP 地址。这个时候请用云主机提供商提供的网络 API 获取。

### fpnn::NetworkUtil 命名空间

#### getFirstIPAddress

	std::string getFirstIPAddress(enum IPTypes type);

获取指定类型的第一个 IP （本机绑定至少一个该类型 IP 的情况下）（多个同类型 IP 的顺序由系统决定）。

#### getLocalIP4

	std::string getLocalIP4();

获取本机的内网 IPv4 地址。如果本机绑定多个内网 IPv4 地址，只会获取到第一个（多个内网 IPv4 IP 的顺序由系统决定）。

#### getPublicIP4

	std::string getPublicIP4();

获取本机的公网 IPv4 地址。如果本机绑定多个公网 IPv4 地址，只会获取到第一个（多个公网 IPv4 IP 的顺序由系统决定）。


#### getPeerName

	std::string getPeerName(int fd);

获取 fd 绑定的远端机器的 endpoint。

#### getSockName

	std::string getSockName(int fd);

获取 fd 绑定的本机的 endpoint。

#### isPrivateIP

	bool isPrivateIP(struct sockaddr_in* addr);
	bool isPrivateIP(struct sockaddr_in6* addr);

判断是否是内网地址。

#### isPrivateIPv4

	bool isPrivateIPv4(const std::string& ipv4);

判断是否是内网 IPv4 地址。

#### isPrivateIPv6

	bool isPrivateIPv6(const std::string& ipv6);

判断是否是本地 IPv6 地址。