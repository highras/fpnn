## TCPFPZKConsistencyProxy

### 介绍

TCPFPZKConsistencyProxy 是以一致性集群广播为访问方式，与 FPZK 服务联动的 TCP 集群代理对象，是 [TCPFPZKProxyCore](TCPFPZKProxyCore.md) 和  [TCPConsistencyProxy](TCPConsistencyProxy.md) 的子类。

### 命名空间

	namespace fpnn;

### TCPFPZKConsistencyProxy

	class TCPFPZKConsistencyProxy: virtual public TCPConsistencyProxy, virtual public TCPFPZKProxyCore
	{
	public:
		TCPFPZKConsistencyProxy(FPZKClientPtr fpzkClient, const std::string& serviceName,
				ConsistencySuccessCondition condition, int requiredCount = 0, int64_t questTimeoutSeconds = -1);
		TCPFPZKConsistencyProxy(FPZKClientPtr fpzkClient, const std::string& serviceName, const std::string& cluster,
				ConsistencySuccessCondition condition, int requiredCount = 0, int64_t questTimeoutSeconds = -1);
		virtual ~TCPFPZKConsistencyProxy();
	};
	typedef std::shared_ptr<TCPFPZKConsistencyProxy> TCPFPZKConsistencyProxyPtr;

#### 构造函数

	TCPFPZKConsistencyProxy(FPZKClientPtr fpzkClient, const std::string& serviceName,
			ConsistencySuccessCondition condition, int requiredCount = 0, int64_t questTimeoutSeconds = -1);
	TCPFPZKConsistencyProxy(FPZKClientPtr fpzkClient, const std::string& serviceName, const std::string& cluster,
			ConsistencySuccessCondition condition, int requiredCount = 0, int64_t questTimeoutSeconds = -1);

**参数说明**

* **`FPZKClientPtr fpzkClient`**

	关联的 [FPZKClient](FPZKClient.md) 实例。

* **`const std::string& serviceName`**

	集群名称。

* **`const std::string& cluster`**

	集群二级分组名称。

* **`ConsistencySuccessCondition condition`**

	一致性达成类型，请参见 [ConsistencySuccessCondition](TCPConsistencyProxy.md#ConsistencySuccessCondition)。

* **`int requiredCount`**

	ConsistencySuccessCondition 为 CountedQuestsSuccess 时，指定的成功应答的数量标准。

	如果 ConsistencySuccessCondition 为其他字段，该参数忽略。

* **`int64_t questTimeoutSeconds`**

	Proxy 默认的请求超时时间。单位：秒。负值与0表示使用全局([ClientEngine](../core/ClientEngine.md))设置。

#### 成员函数

本文档仅列出基于基类所扩展的成员函数，其余成员函数请参考基类文档:

+ [TCPProxyCore](TCPProxyCore.md)
+ [TCPFPZKProxyCore](TCPFPZKProxyCore.md)
+ [TCPConsistencyProxy](TCPConsistencyProxy.md)
