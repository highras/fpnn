## TCPFPZKProxyCore

### 介绍

TCPFPZKProxyCore 为与 FPZK 服务联动的 TCP Proxy 的核心基类，是 [TCPProxyCore](TCPProxyCore.md) 的子类。

### 命名空间

	namespace fpnn;

### TCPFPZKProxyCore

	class TCPFPZKProxyCore: virtual public TCPProxyCore
	{
	public:
		TCPFPZKProxyCore(FPZKClientPtr fpzkClient, const std::string& serviceName, const std::string& cluster = "", int64_t questTimeoutSeconds = -1);
		virtual ~TCPFPZKProxyCore();

		const std::string& serviceName() const;
	};

#### 构造函数

	TCPFPZKProxyCore(FPZKClientPtr fpzkClient, const std::string& serviceName, const std::string& cluster = "", int64_t questTimeoutSeconds = -1);

**参数说明**

* **`FPZKClientPtr fpzkClient`**

	关联的 [FPZKClient](FPZKClient.md) 实例。

* **`const std::string& serviceName`**

	集群名称。

* **`const std::string& cluster`**

	集群二级分组名称。

* **`int64_t questTimeoutSeconds`**

	Proxy 默认的请求超时时间。单位：秒。负值与0表示使用全局([ClientEngine](../core/ClientEngine.md))设置。

#### 成员函数

本文档仅列出基于基类所扩展的成员函数，其余成员函数请参考基类文档 [TCPProxyCore](TCPProxyCore.md)。

##### serviceName

	const std::string& serviceName() const;

返回集群全称。

**注意**：集群全称：对于一级集群，即集群名称；对于二级集群，即“集群名称@分组名称”。
