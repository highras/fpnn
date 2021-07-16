## ServerInfo

### 介绍

服务器信息数据模块。

### 命名空间

	namespace fpnn;

### ServerInfo

	class ServerInfo
	{
	public:
		static void getAllInfos();

		static const std::string& getServerHostName();
		static const std::string& getServerRegionName();
		static const std::string& getServerZoneName();
		static const std::string& getServerLocalIP4();
		static const std::string& getServerPublicIP4();
		static const std::string& getServerLocalIP6();
		static const std::string& getServerPublicIP6();
	};

#### getAllInfos

	static void getAllInfos();

一次性准备好其余接口所属数据的缓存，使其余接口调用时，可以直接返回，而无须调用网络访问，获取所需数据（云主机，需首先在[def.mk](../../../../def.mk)中配置默认的云主机服务商）。

#### getServerHostName

	static const std::string& getServerHostName();

获取本机绑定的域名或主机名（云主机，需首先在[def.mk](../../../../def.mk)中配置默认的云主机服务商）。

#### getServerRegionName

	static const std::string& getServerRegionName();

获取本机所在区域（云主机，需首先在[def.mk](../../../../def.mk)中配置默认的云主机服务商）。


#### getServerZoneName

	static const std::string& getServerZoneName();

获取本机所在区域的分区（云主机，需首先在[def.mk](../../../../def.mk)中配置默认的云主机服务商）。

#### getServerLocalIP4

	static const std::string& getServerLocalIP4();

获取本机内网 IPv4 地址（云主机，需首先在[def.mk](../../../../def.mk)中配置默认的云主机服务商）。

#### getServerPublicIP4

	static const std::string& getServerPublicIP4();

获取本机公网 IPv4 地址（云主机，需首先在[def.mk](../../../../def.mk)中配置默认的云主机服务商）。

#### getServerLocalIP6

	static const std::string& getServerLocalIP6();

获取本机本地 IPv6 地址（云主机，需首先在[def.mk](../../../../def.mk)中配置默认的云主机服务商）。

#### getServerPublicIP6

	static const std::string& getServerPublicIP6();

获取本机公网 IPv6 地址（云主机，需首先在[def.mk](../../../../def.mk)中配置默认的云主机服务商）。