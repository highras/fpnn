## TCPFPZKBroadcastProxy

### 介绍

TCPFPZKBroadcastProxy 是以集群广播为访问方式，与 FPZK 服务联动的 TCP 集群代理对象，是 [TCPFPZKProxyCore](TCPFPZKProxyCore.md) 和  [TCPBroadcastProxy](TCPBroadcastProxy.md) 的子类。

### 命名空间

	namespace fpnn;

### TCPFPZKBroadcastProxy

	class TCPFPZKBroadcastProxy: virtual public TCPBroadcastProxy, virtual public TCPFPZKProxyCore
	{
	public:
		TCPFPZKBroadcastProxy(FPZKClientPtr fpzkClient, const std::string& serviceName, const std::string& cluster = "", int64_t questTimeoutSeconds = -1);

		virtual ~TCPFPZKBroadcastProxy();

		void exceptSelf();
	};
	typedef std::shared_ptr<TCPFPZKBroadcastProxy> TCPFPZKBroadcastProxyPtr;

#### 构造函数

	TCPFPZKBroadcastProxy(FPZKClientPtr fpzkClient, const std::string& serviceName, const std::string& cluster = "", int64_t questTimeoutSeconds = -1);

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

本文档仅列出基于基类所扩展的成员函数，其余成员函数请参考基类文档:

+ [TCPProxyCore](TCPProxyCore.md)
+ [TCPFPZKProxyCore](TCPFPZKProxyCore.md)
+ [TCPBroadcastProxy](TCPBroadcastProxy.md)

##### exceptSelf

	void exceptSelf();

在集群中排除自身。

如果集群包含自身服务实例，且希望广播时，自身服务实例不收到自身发出的广播，则调用该函数，移除自身服务实例的 endpoint。