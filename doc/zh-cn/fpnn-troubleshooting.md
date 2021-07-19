# FPNN 问题排查

1. FPNN 服务无法响应 Ctrl + C 中断

	请依次检查以下项目：

	* [def.mk](../../def.mk) 中对于平台参数 DEFAULTPLATFORM 的配置是否正确
	* 若当前平台不属于 亚马逊 AWS、谷歌 GCP、微软 Azure、腾讯云、阿里云 五个云平台，则配置文件中是否配置了以下条目：

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

		**高级操作**

		* 若因以上项目未配置，而导致 FPNN 服务无法响应 Ctrl + C 中断，可使用 pstack 命令查看服务进程主线程，看是否因为 curl 访问 AWS/GCP/Azure 的服务而导致 FPNN 服务挂起。  
		若是，则在主线程堆栈中查找对应的 ServerInfo 类调用，然后在 [/base/serverInfo.cpp](../../base/ServerInfo.cpp) 中查找对应的接口实现，以便确认需要配置以上哪项参数。

			*pstack 可通过 yum install gdb 安装*

	* 应用是否捕获并接管了 Ctrl + C 中断


1. FPNN 服务无响应

	请使用 [/core/tools/cmd](fpnn-tools.md#cmd) 或 [/core/tools/cmdinfos](fpnn-tools.md#cmdinfos) 访问 FPNN 服务，查看是否有响应。

	* 若无响应，请依此检查以下项目：

		+ 请使用 pstack 命令查看服务进程主线程堆栈，是否包含对 ServerInfo 类的调用

			若包含，请参考“FPNN 服务无法响应 Ctrl + C 中断”一节。

		+ 请使用 pstack 命令查看服务进程所有线程堆栈，具体分析业务代码堆栈

		*pstack 可通过 yum install gdb 安装*

	* 若 FPNN 服务返回超时，表明短时间内 FPNN 服务内部工作线程已被业务接口长时间占用，当前已无可用工作线程。

		请使用 pstack 命令查看服务进程所有线程堆栈，具体分析业务代码堆栈

	* 若 FPNN 服务返回错误，请根据具体错误信息，或错误代码进行检查

	* 若 FPNN 服务正确响应，请分析返回结果的 FPNN.status/server/thread/tcp 节点，查看 workThreadStatus/ioThreadStatus/duplexThreadStatus 三个节点对象的以下域：

		+ taskSize

			大于 0 表示有请求堆积。具体数值为待处理的请求数量。

		+ tempCount

			大于 0 表示有临时线程被起用。表示业务压力过大，或者服务器配置参数需要调整/优化

		+ busyCount

			如果 busyCount 值大于 normalCount 值，表示业务压力过大，或者服务器配置参数需要调整/优化


