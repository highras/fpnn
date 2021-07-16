# FPNN 注意事项

1. 平台配置

	FPNN 在 亚马逊 AWS、谷歌 GCP、微软 Azure、腾讯云、阿里云 五个平台上，将自动获取网络相关配置。

	FPNN 默认启动 亚马逊 AWS 支持。如果需要更改云平台支持，请修改 FPNN 全局预置配置文件 [def.mk](../../def.mk) 中，DEFAULTPLATFORM 参数即可。

	如果非以上平台：

	* 对于多 IP 设备，请在配置文件中增加并配置以下配置项：

			# 服务标识自身的内网 IPv4 地址（非监听地址）
			FP.server.local.ip4 =

	* 需要使用 [FPZKClient](APIs/extends/FPZKClient.md) 或者 [TCPFPZKProxy 系列模块](APIs/extends/)时，请在配置文件中增加并配置以下配置项：

			# 服务的主机名称
			FP.server.hostname =

			# 服务的区域/地域名称/编号
			FP.server.zone.name = 
			# 或
			FP.server.region.name =

			# 对于多 IP 设备，服务标识自身的内网 IPv4 地址（非监听地址）
			FP.server.local.ip4 =

			# 对于多 IP 设备，服务对外提供服务的外网 IPv4 地址（非监听地址）
			FP.server.public.ip4 =

	**注意**

	**错误的配置可能导致 FPNN 服务无响应**

