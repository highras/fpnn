# FPNN 安全体系

**任何框架和服务，首先依赖于系统的安全体系。在系统的安全体系完备的情况下，框架和服务的安全体系才具有实际意义。**

## FPNN 框架提供的安全措施

1. 加密系统

	FPNN 支持 SSL/TLS，并提供独立于 SSL/TLS 之外的加密和秘钥交换功能。

	FPNN 使用 [OpenSSL](https://www.openssl.org) 提供对 SSL/TLS 的支持。OpenSSL 参见 [OpenSSL 官方网站](https://www.openssl.org) [中文维基百科](https://baike.baidu.com/item/openssl) [英文维基百科](https://en.wikipedia.org/wiki/OpenSSL)

	SSL/TLS 的支持可以独立开启或者关闭。

	在 SSL/TLS 之外，FPNN 提供可独立使用的加密和密钥交换功能。

	1. 加密

		FPNN 采用 AES 加密标准，CFB 加密模式。

		AES，即美国政府采用的高级加密标准。加密强度、安全性可参见 [中文维基百科](https://zh.wikipedia.org/wiki/高级加密标准) [英文维基百科](https://en.wikipedia.org/wiki/Advanced_Encryption_Standard)

		CFB 请参考 [中文维基百科](https://zh.wikipedia.org/wiki/分组密码工作模式) [英文维基百科](https://en.wikipedia.org/wiki/Block_cipher_mode_of_operation)

		FPNN 框架支持 128 位秘钥，以及 256 位秘钥。

		1. 流加密

			+ 将整个会话，从链接创建到链接关闭之间的所有数据视为一个数据流进行加密。
			+ 每个请求和回应都是这个数据流不可分割的一部分。
			+ 任何两个相同的数据包不会产生相同的密文。

		1. 包加密

			由于不少语言的加密库不能对流加密模式提供很好的支持，因此 FPNN 也支持包加密模式。

			+ 每个 FPNN 封包是一个独立的个体，进行独立的加密。
			+ 如果在同一个会话中，两个封包完全一模一样，那将产生相同的密文。

		FPNN 的加密策略与连接绑定。  
		不同的链接，可以使用不同长度的秘钥，以及不同的加密方式（流加密或者包加密）。


	1. 秘钥交换

		FPNN 采用 ECDH 进行秘钥交换。

		ECDH 的背后是 ECC (椭圆曲线加密算法)。关于 ECC 和 RSA 的安全性和加密强度，可参考网上搜索结果。

		关于 ECC 请参见：[英文维基百科](https://en.wikipedia.org/wiki/Elliptic-curve_cryptography)

		关于 ECDH 请参见：[英文维基百科](https://en.wikipedia.org/wiki/Elliptic-curve_Diffie–Hellman)


		**风险**：

		+ **中间人攻击**

			- 实现方式：

				1. 劫持交换机/路由器等网关类设备/服务，安装劫持代理；

				1. 截取设备上秘钥交换的通讯，分别建立客户端与自己，自己与服务器两条链接。冒充客户端与服务器交换秘钥；

				1. 将服务器发往客户端的数据解密，修改后转发客户端；将客户端发往服务器的数据解密，修改后转发服务器。

			- 防止方式：

				客户端发布时，**内置**服务器公钥。

1. 访问限制

	1. FPNN 框架内置接口访问限制

		FPNN 框架[内置接口](fpnn-build-in-methods.md)，除[秘钥交换接口](fpnn-build-in-methods.md#key)和[TCP保活接口](fpnn-build-in-methods.md#ping)外，其余仅允许内网访问。外网访问将被自动阻止。

	1. 用户接口访问限制

		用户可以指定业务自定义接口的[安全等级](APIs/core/IQuestProcessor.md#MethodAttribute)，是必须加密链接才能访问，还是内网才能访问，还是两者兼具。

	1. IP/IP段白名单

		FPNN 服务可以指定只允许哪些 IP，以及哪些 IP 段的设备可以访问。



## FPNN 生态环境提供的安全措施

* **Dispatch 服务**

	Dispatcher 服务提供访问限制。  
	只有被允许分发的服务类别，才会进行分发。

	同时，Dispatcher 作为 FPNN 框架所开发的服务，因此拥有 FPNN 框架提供的所有安全措施。

* **集群管理服务**

	FPZK 集群管理服务，提供密码配置。  
	如果不同的项目共享共一个 FPZK 集群，需要正确的项目密码，才能加入，或者访问对应的项目集群。

	同时，FPZK 作为 FPNN 框架所开发的服务，因此拥有 FPNN 框架提供的所有安全措施。


* **数据库路由代理服务**

	对于 [MySQL DBProxy](https://github.com/highras/dbproxy) 提供以下额外的安全策略：

	+ 对业务屏蔽所有的 MySQL 网络拓扑，和分库分表逻辑

		业务开发者，无需、也不应该知道具体的数据库拓扑，和分库分表逻辑。  
		数据库网络拓扑和分库分表逻辑只应该由 DBA 掌握和管理。

	+ 业务不能创建、删除和修改 数据库、数据表

		这部分权限仅能由 DBA 处理。  
		DBA 可以使用 DBProxy Manager 版进行操作。

	+ 业务可以仅通过一个请求进行多 sharding 查询并举和查询结果，但对每个 sharding 的修改操作需要单独进行

		DBA 可以通过 DBProxy Manager 进行多 sharding 的批量修改操作。

	+ 业务库和 DBProxy 配置库账号和权限隔离
	+ 业务库账号混淆

	同时，数据库路由代理服务作为 FPNN 框架所开发的服务，因此拥有 FPNN 框架提供的所有安全措施。



* **通用集群网关**

	通用集群网关自身对外屏蔽了内部网络所有的拓扑结构和部署细节。  
	外部访问仅需指定路由模式和需要访问的服务类型，其他无需了解。

	通用集群网关还支持限制模式，在限制模式下，只有被允许的服务能通过被允许的路由模式访问。

	同时，通用集群网关作为 FPNN 框架所开发的服务，因此拥有 FPNN 框架提供的所有安全措施。


* **实时消息系统服务**

	具体请参见 [云上曲率](https://www.ilivedata.com/) 关于 RTM 系统的相关介绍。

	同时，实时消息系统作为 FPNN 框架所开发的服务，因此拥有 FPNN 框架提供的所有安全措施。
