## FPZKClient

### 介绍

服务发现服务 FPZKServer 客户端。

FPZKClient 有三种工作模式：

+ 仅向 FPZKServer 注册，汇报相关信息。便于其他服务发现自身。
+ 仅向 FPZKServer 订阅信息，获取感兴趣的服务的状态和状态变化。
+ 以上两种模式混合。

FPZKClient 向 FPZKServer 汇报的信息包括：

+ 基本信息（必选）

	+ 自身服务名称
	+ 自身服务所在集群（如果有）
	+ 自身服务程序版本（如果有）
	+ 自身 endpoint（内部网络使用）
	+ 是否提供服务（online 状态）
	+ 服务启动时间
	+ 服务绑定的 TCP IP v4 端口（如果有）
	+ 服务绑定的 TCP IP v6 端口（如果有）
	+ 服务绑定的 UDP IP v4 端口（如果有）
	+ 服务绑定的 UDP IP v6 端口（如果有）

+ 附加信息（可选）

	+ 供外部访问的 endpoint（外部网络使用。如果有）
	+ 本机域名（如果有）
	+ 服务绑定的 IP v4 地址（如果有）
	+ 服务绑定的 IP v6 地址（如果有）
	+ 服务绑定的 SSL/TLS IP v4 端口（如果有）
	+ 服务绑定的 SSL/TLS IP v6 端口（如果有）

+ 性能信息（可选）

	+ 当前服务器 TCP 链接数量
	+ 当前服务器按 CPU 核心数平均后的负载
	+ 当前服务器 CPU 的平均利用率

文件配置可参见 [conf.template](../../../conf.template) 相关条目，或 [FPZKCLient.h](../../../../extends/FPZKClient.h) 的头部注释。

### 命名空间

	namespace fpnn;

### ServiceNode

	class FPZKClient
	{
	public:
		struct ServiceNode
		{
			bool online;
			int connCount;
			float CPULoad;
			float loadAvg;  //-- per CPU Usage
			int64_t registerTime;	//-- in seconds
			int64_t activedTime;	//-- in seconds
			std::string version;
			std::string region;

			int port;
			int port6;
			int sslport;
			int sslport6;
			int uport;
			int uport6;
			std::string domain;
			std::string ipv4;
			std::string ipv6;

			ServiceNode();
		};
	};

服务节点信息。

**部分成员说明**

+ **`bool online`**

	节点是否可提供服务（服务是否在线）。

+ **`int connCount`**

	节点机器层面的 TCP 链接数量。

+ **`float CPULoad`**

	CPU 使用率。

+ **`float loadAvg`**

	节点机器，按 CPU 核心数平均的系统负载。

+ **`int64_t registerTime`**

	节点在 FPZKServer 集群中的注册时间。

+ **`int64_t activedTime`**

	节点上次状态同步时间。

+ **`std::string version`**

	节点服务版本。

+ **`std::string region`**

	节点所在区域。

### ServiceInfos

	class FPZKClient
	{
	public:
		struct ServiceInfos
		{
			int onlineCount;
			int64_t revision;
			int64_t updateMsec;
			int64_t clusterAlteredMsec;
			std::map<std::string, ServiceNode> nodeMap;

			ServiceInfos();
		};
		typedef std::shared_ptr<ServiceInfos> ServiceInfosPtr;
	};

服务信息。

**成员说明**

+ **`int onlineCount`**

	本集群在线服务的节点数量。

+ **`int64_t revision`**

	集群变动版本。

+ **`int64_t updateMsec`**

	集群信息更新时间。

+ **`int64_t clusterAlteredMsec`**

	集群上次变动的时间。

+ **`std::map<std::string, ServiceNode> nodeMap`**

	集群节点字典。key 为节点 endpoint，值请参考 [ServiceNode](#ServiceNode)。

### ServicesAlteredCallback

	class FPZKClient
	{
	public:
		class ServicesAlteredCallback
		{
		public:
			virtual ~ServicesAlteredCallback() {}
			virtual void serviceAltered(std::map<std::string, ServiceInfosPtr>& serviceInfos) = 0;
		};
		typedef std::shared_ptr<ServicesAlteredCallback> ServicesAlteredCallbackPtr;
	};

服务集群/分组变动事件。

**参数说明**

* **`std::map<std::string, ServiceInfosPtr>& serviceInfos`**

	发生变动的集群全名（格式：服务名 或 服务名@分组名）和服务信息 [ServiceInfos](#ServiceInfos) 组成的字典。

### FPZKClient

	class FPZKClient
	{
	public:
		static FPZKClientPtr create(const std::string& fpzkSrvList = "", const std::string& projectName = "", const std::string& projectToken = "");
		~FPZKClient();

		bool registerService(const std::string& serviceName = "", const std::string& cluster = "", const std::string& version = "", const std::string& endpoint = "", bool online = true);
		bool registerServiceSync(const std::string& serviceName = "", const std::string& cluster = "", const std::string& version = "", const std::string& endpoint = "", bool online = true);
		int64_t getServiceRevision(const std::string& serviceName, const std::string& cluster = "");
		const ServiceInfosPtr getServiceInfos(const std::string& serviceName, const std::string& cluster = "", const std::string& version = "", bool onlineOnly = true);
		std::vector<std::string> getServiceEndpoints(const std::string& serviceName, const std::string& cluster = "", const std::string& version = "", bool onlineOnly = true);
		std::vector<std::string> getServiceEndpointsWithoutMyself(const std::string& serviceName, const std::string& cluster = "", const std::string& version = "", bool onlineOnly = true);
		int64_t getServiceChangedMSec(const std::string& serviceName, const std::string& cluster = "");
		std::string getOldestServiceEndpoint(const std::string& serviceName, const std::string& cluster = "", const std::string& version = "", bool onlineOnly = true);
		
		inline std::string registeredName() const;
		inline std::string registeredCluster() const;
		inline std::string registeredEndpoint() const;

		inline void keepAlive();
		inline void setKeepAlivePingTimeout(int seconds);
		inline void setKeepAliveInterval(int seconds);
		inline void setKeepAliveMaxPingRetryCount(int count);

		inline void setServiceAlteredCallback(ServicesAlteredCallbackPtr callback);

		inline void setServiceAlteredCallback(std::function<void (std::map<std::string, ServiceInfosPtr>& serviceInfos)> function);

		inline void setOnline(bool online);
		inline void monitorDetail(bool monitor);
		inline void monitorDetail(bool monitor, const std::string& service);
		inline void monitorDetail(bool monitor, const std::set<std::string>& detailServices);
		inline void monitorDetail(bool monitor, const std::vector<std::string>& detailServices);
		inline void unregisterService();
		inline void unregisterServiceSync();
	};

	typedef std::shared_ptr<FPZKClient> FPZKClientPtr;

FPZKServer 客户端。服务发现客户端。

### 成员函数

#### create

	static FPZKClientPtr create(const std::string& fpzkSrvList = "", const std::string& projectName = "", const std::string& projectToken = "");

创建 FPZKClient 对象实例。

**参数说明**

* **`const std::string& fpzkSrvList`**

	FPZKServer 服务器的地址列表。以空格或者半角逗号分隔。

	**注意**

	+ 跨区域部署时，仅需列出本区域的 FPZKServer 服务地址即可。

	+ 如果该参数为空，将尝试读取配置文件中，key 为 "FPZK.client.fpzkserver_list" 的条目的值。


* **`const std::string& projectName`**

	在 FPZKServer 集群中，本服务所属项目的名称。

	如果该参数为空，将尝试读取配置文件中，key 为 "FPZK.client.project_name" 的条目的值。


* **`const std::string& projectToken`**

	在 FPZKServer 集群中，本服务所属项目对应的 token。

	如果该参数为空，将尝试读取配置文件中，key 为 "FPZK.client.project_token" 的条目的值。

#### registerService

	bool registerService(const std::string& serviceName = "", const std::string& cluster = "", const std::string& version = "", const std::string& endpoint = "", bool online = true);

向 FPZKServer 注册（异步操作）。

**注意**

如果仅是需要查询其他服务，则无需向 FPZKServer 注册。如果想让其他服务发现自身，则需要注册。

**参数说明**

* **`const std::string& serviceName`**

	服务自身的名称。

	如果该参数为空，将尝试读取配置文件中，key 为 "FPNN.server.name" 的条目的值。

* **`const std::string& cluster`**

	服务所处的二级集群/分组。

	如果该参数为空，将尝试读取配置文件中，key 为 "FPNN.server.cluster.name" 的条目的值。

* **`const std::string& version`**

	当前服务的版本号。

* **`const std::string& endpoint`**

	访问本服务所用的 endpoint。

	如果该参数为空，FPZKClient 将采用本机 IP v4 地址和端口合成 endpoint。

* **`bool online`**

	目前是否提供服务/是否在线/离线。

#### registerServiceSync

	bool registerServiceSync(const std::string& serviceName = "", const std::string& cluster = "", const std::string& version = "", const std::string& endpoint = "", bool online = true);

向 FPZKServer 注册（同步操作）。

**注意**

如果仅是需要查询其他服务，则无需向 FPZKServer 注册。如果想让其他服务发现自身，则需要注册。

**参数说明**

* **`const std::string& serviceName`**

	服务自身的名称。

	如果该参数为空，将尝试读取配置文件中，key 为 "FPNN.server.name" 的条目的值。

* **`const std::string& cluster`**

	服务所处的二级集群/分组。

	如果该参数为空，将尝试读取配置文件中，key 为 "FPNN.server.cluster.name" 的条目的值。

* **`const std::string& version`**

	当前服务的版本号。

* **`const std::string& endpoint`**

	访问本服务所用的 endpoint。

	如果该参数为空，FPZKClient 将采用本机 IP v4 地址和端口合成 endpoint。

* **`bool online`**

	目前是否提供服务/是否在线/离线。

#### getServiceRevision

	int64_t getServiceRevision(const std::string& serviceName, const std::string& cluster = "");

获取指定的服务集群/分组变动版本。

**参数说明**

* **`const std::string& serviceName`**

	需要获取的服务名称。

* **`const std::string& cluster`**

	需要获取的服务所属的二级集群/分组名称。

#### getServiceInfos

	const ServiceInfosPtr getServiceInfos(const std::string& serviceName, const std::string& cluster = "", const std::string& version = "", bool onlineOnly = true);

获取指定的服务集群/分组信息。

**参数说明**

* **`const std::string& serviceName`**

	需要获取的服务名称。

* **`const std::string& cluster`**

	需要获取的服务所属的二级集群/分组名称。

* **`const std::string& version`**

	需要获取的服务的版本号。

	如果为空，表示所有版本。

* **`bool onlineOnly`**

	是否只获取可提供服务的在线节点。

**返回值**

集群节点信息。参见 [ServiceInfos](#ServiceInfos)。

#### getServiceEndpoints

	std::vector<std::string> getServiceEndpoints(const std::string& serviceName, const std::string& cluster = "", const std::string& version = "", bool onlineOnly = true);

获取指定的服务集群/分组的 endpoint 列表。

**参数说明**

* **`const std::string& serviceName`**

	需要获取的服务名称。

* **`const std::string& cluster`**

	需要获取的服务所属的二级集群/分组名称。

* **`const std::string& version`**

	需要获取的服务的版本号。

	如果为空，表示所有版本。

* **`bool onlineOnly`**

	是否只获取可提供服务的在线节点。

**返回值**

满足条件的服务节点 endpoint 列表。

#### getServiceEndpointsWithoutMyself

	std::vector<std::string> getServiceEndpointsWithoutMyself(const std::string& serviceName, const std::string& cluster = "", const std::string& version = "", bool onlineOnly = true);

获取指定的服务集群/分组的 endpoint 列表。如果列表包含自身，则排除自身。

该接口多用于获取自身所属二级集群/分组内的兄弟节点。

**参数说明**

* **`const std::string& serviceName`**

	需要获取的服务名称。

* **`const std::string& cluster`**

	需要获取的服务所属的二级集群/分组名称。

* **`const std::string& version`**

	需要获取的服务的版本号。

	如果为空，表示所有版本。

* **`bool onlineOnly`**

	是否只获取可提供服务的在线节点。

**返回值**

满足条件的服务节点 endpoint 列表。

#### getServiceChangedMSec

	int64_t getServiceChangedMSec(const std::string& serviceName, const std::string& cluster = "");

获取指定的服务集群/分组的集群变动时间。

**参数说明**

* **`const std::string& serviceName`**

	需要获取的服务名称。

* **`const std::string& cluster`**

	需要获取的服务所属的二级集群/分组名称。

#### getOldestServiceEndpoint

	std::string getOldestServiceEndpoint(const std::string& serviceName, const std::string& cluster = "", const std::string& version = "", bool onlineOnly = true);

获取指定的服务集群/分组中，服务时间最久的节点的 endpoint。

**参数说明**

* **`const std::string& serviceName`**

	需要获取的服务名称。

* **`const std::string& cluster`**

	需要获取的服务所属的二级集群/分组名称。

* **`const std::string& version`**

	需要获取的服务的版本号。

	如果为空，表示所有版本。

* **`bool onlineOnly`**

	是否只获取可提供服务的在线节点。
	
#### registeredName

	inline std::string registeredName() const;

自身注册的服务名称。

#### registeredCluster

	inline std::string registeredCluster() const;

自身注册的二级集群/分组名称。

#### registeredEndpoint

	inline std::string registeredEndpoint() const;

自身注册的 endpoint 信息。

#### keepAlive

	void keepAlive();

开启连接自动保活/保持连接。

#### setKeepAlivePingTimeout

	void setKeepAlivePingTimeout(int seconds);

设置自动保活状态下的 ping 请求超时时间，单位：秒。默认：5 秒。

#### setKeepAliveInterval

	void setKeepAliveInterval(int seconds);

设置自动保活状态下的 ping 请求间隔时间，单位：秒。默认：20 秒。

#### setKeepAliveMaxPingRetryCount

	void setKeepAliveMaxPingRetryCount(int count);

设置开启自动保活时，ping 超时后，最大重试次数。超过该次数，认为链接丢失。默认：3 次。

#### setServiceAlteredCallback

	inline void setServiceAlteredCallback(ServicesAlteredCallbackPtr callback);

设置集群变动事件通知的回调对象。

**参数说明**

* **`ServicesAlteredCallbackPtr callback`**

	集群变动事件，通知回调对象 [ServicesAlteredCallback](#ServicesAlteredCallback)。

#### setServiceAlteredCallback

	inline void setServiceAlteredCallback(std::function<void (std::map<std::string, ServiceInfosPtr>& serviceInfos)> function);

设置集群变动事件通知的回调。

**参数说明**

* **`std::map<std::string, ServiceInfosPtr>& serviceInfos`**

	发生变动的集群全名（格式：服务名 或 服务名@分组名）和服务信息 [ServiceInfos](#ServiceInfos) 组成的字典。

#### setOnline

	inline void setOnline(bool online);

修改和设置在线服务状态。

#### monitorDetail

	inline void monitorDetail(bool monitor);
	inline void monitorDetail(bool monitor, const std::string& service);
	inline void monitorDetail(bool monitor, const std::set<std::string>& detailServices);
	inline void monitorDetail(bool monitor, const std::vector<std::string>& detailServices);

订阅全部/指定服务的信息。

**参数说明**

* **`bool monitor`**

	是否订阅。

* **`const std::string& service`**

	需要订阅的集群全名（格式：服务名 或 服务名@分组名）。

* **`const std::set<std::string>& detailServices`**

	需要订阅的集群全名（格式：服务名 或 服务名@分组名）。

* **`const std::vector<std::string>& detailServices`**

	需要订阅的集群全名（格式：服务名 或 服务名@分组名）。

#### unregisterService

	inline void unregisterService();

向 FPZKServer 注销（异步操作）。

#### unregisterServiceSync

	inline void unregisterServiceSync();

向 FPZKServer 注销（同步操作）。
