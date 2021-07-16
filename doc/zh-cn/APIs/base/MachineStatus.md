## MachineStatus

### 介绍

机器状态信息获取。

### 命名空间

	namespace fpnn::MachineStatus;

### 全局函数

#### getCPULoad

	float getCPULoad();

获取本机当前CPU负载。

#### getConnectionCount

	int getConnectionCount();

获取本机当前 TCP 连接数目（IPv4 + IPv6）。

#### getIPv4ConnectionCount

	int getIPv4ConnectionCount();

获取本机当前 IPv4 TCP 连接数目。

#### getIPv6ConnectionCount

	int getIPv6ConnectionCount();

获取本机当前 IPv6 TCP 连接数目。

#### getNetworkStatus

	void getNetworkStatus(uint64_t& recvBytes, uint64_t& sendBytes);

获取本机所有网卡当前已收发的字节数。