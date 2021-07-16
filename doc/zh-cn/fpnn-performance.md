# FPNN 性能报告

## 性能测试说明

本测试报告所有数据均由 FPNN 自动化分布式测试工具测试生成。  
FPNN 自动化分布式测试工具需要在 **DATS (Distributed Automated Testing Suite)** 下使用。

### 1. 压力测试

1. 目标机型

	FPNN 1.0.0 版本开始，压力测试使用亚马逊 m5.xlarge 机型。早期版本压力测试使用亚马逊 m4.xlarge 机型和 m4.2xlarge 机型。

	+ m5.xlarge 机型为虚拟 4 核 CPU，16 GB 内存
	+ m4.xlarge 机型为虚拟 4 核 CPU，16 GB 内存
	+ m4.2xlarge 机型为虚拟 8 核 CPU，32 GB 内存

1. 目标操作系统

	FPNN 1.0.0 版本开始，测试服务器使用 CentOS 8，压力源使用 CentOS 7。早期版本压力测试均使用 CentOS 7。

	**注意**

	FPNN UDP 服务端需要 Linux 3.9 及以上内核，否则性能会明显降低。

1. 测试准备

	+ 目标服务器

		1. 修改 /etc/security/limits.conf 文件，增加可用文件描述符数量。

			参考：

			增加以下条目：

					*		soft	nofile		1000000
					*		hard	nofile		1000000

			**注意**：nofile 数量不能大于机器的物理限制数量。  
			机器的物理限制数量可参见 /proc/sys/fs/file-max

		1. 根据需要修改 /etc/sysctl.conf 文件

			如果当前账户可用最大文件描述符小于修改后的 nofile 参数数量，则需要修改 /etc/sysctl.conf 文件，编辑/增加

					fs.nr_open = 1000000

			当前账户可用最大文件描述符数量可参考

					sysctl -a | grep fs.nr_open

		1. 修改以上配置文件后，重启目标服务器


1. 测试过程

	TCP 压力测试使用 TCPStressTest 模块进行，UDP 压力测试使用 UDPStressTest 模块进行。

	+ TCPStressTest 模块位置：[/core/test/AutoDistributedTest/TCPStressTest/](../../core/test/AutoDistributedTest/TCPStressTest/)
	+ UDPStressTest 模块位置：[/core/test/AutoDistributedTest/UDPStressTest/](../../core/test/AutoDistributedTest/UDPStressTest/)


	启动 TCPStressController 或者 UDPStressController，然后等待测试完成即可。测试时间一般在 1 小时以内。

	+ TCPStressController 启动参数请参见 [FPNN 内置工具](fpnn-tools.md#TCPStressTest) 对应说明；
	+ UDPStressController 启动参数请参见 [FPNN 内置工具](fpnn-tools.md#UDPStressTest) 对应说明。

1. 测试数据摘要

	* TCP 测试数据摘要

		* v1.0.0 版本测试数据:

			+ 同一局域网

				| 机型 | 虚拟 CPU | 内存（GB） | 链接数量 | QPS | 平均响应时间（usec） |
				|-----|---------|-----------|---------|-----|------------------|
				| AWS m5.xlarge | 4 | 16 | 1000 | 49,708 | 335 |
				|-----|---------|-----------|---------|-----|------------------|
				| AWS m5.xlarge | 4 | 16 | 130 | 227,919 | 12,854 |
				| AWS m5.xlarge | 4 | 16 | 1,500 | 148,959 | 10,403 |
				| AWS m5.xlarge | 4 | 16 | 2,000 | 99,552 | 356 |
				| AWS m5.xlarge | 4 | 16 | 3,000 | 149,615 | 27,456 |

			+ 洲际传输（德国法兰克福 机房到 美国西部俄勒冈 机房） 

				| 机型 | 虚拟 CPU | 内存（GB） | 链接数量 | QPS | 平均响应时间（usec） | ping/2 (msec) |
				|-----|---------|-----------|---------|-----|-------------------|---------------|
				| AWS m5.xlarge | 4 | 16 | 10 | 17,921 | 147,825 | 137 |
				|-----|---------|-----------|---------|-----|-------------------|---------------|
				| AWS m5.xlarge | 4 | 16 | 190 | 338,601 | 151,772 | 139 |
				| AWS m5.xlarge | 4 | 16 | 700 | 339,240 | 183,541 | 136 ~ 137 |
				| AWS m5.xlarge | 4 | 16 | 3,200 | 312,073 | 256,980 | 139 ~ 141 |
				| AWS m5.xlarge | 4 | 16 | 6,000 | 299,175 | 346,927 | 136 ~ 137 |


		* v0.9.0 版本测试数据 (同一局域网):

			+ 标准链接 (无加密):

				| 机型 | 虚拟 CPU | 内存（GB） | 链接数量 | QPS | 平均响应时间（usec） |
				|-----|---------|-----------|---------|-----|------------------|
				| AWS m4.xlarge | 4 | 16 | 600 | 281,384 | 73,610 |
				| AWS m4.xlarge | 4 | 16 | 700 | 327,003 | 92,278 |
				| AWS m4.xlarge | 4 | 16 | 4,000 | 197,249 | 920,704 |
				| AWS m4.2xlarge | 8 | 32 | 900 | 422,034 | 55,175 |
				| AWS m4.2xlarge | 8 | 32 | 1,400 | 648,722 | 98,224 |
				| AWS m4.2xlarge | 8 | 32 | 8,000 | 376,788 | 869,732 |

			+ SSL 加密链接:

				| 机型 | 虚拟 CPU | 内存（GB） | 链接数量 | QPS | 平均响应时间（usec） |
				|-----|---------|-----------|---------|-----|------------------|
				| AWS m4.xlarge | 4 | 16 | 500 | 235,137 | 50,778 |
				| AWS m4.xlarge | 4 | 16 | 4,000 | 189,512 | 901,040 |
				| AWS m4.2xlarge | 8 | 32 | 900 | 421,072 | 55,121 |
				| AWS m4.2xlarge | 8 | 32 | 7,000 | 348,544 | 696,445 |

			+ FPNN 加密链接 (包加密模式，256 位密钥):

				| 机型 | 虚拟 CPU | 内存（GB） | 链接数量 | QPS | 平均响应时间（usec） |
				|-----|---------|-----------|---------|-----|------------------|
				| AWS m4.xlarge | 4 | 16 | 500 | 215,388 | 56,423 |
				| AWS m4.xlarge | 4 | 16 | 4,000 | 199,099 | 916,827 |
				| AWS m4.2xlarge | 8 | 32 | 900 | 421,831 | 58,147 |
				| AWS m4.2xlarge | 8 | 32 | 7,000 | 347,387 | 797,381 |

			+ FPNN 加密链接 (流加密模式，256 位密钥):

				| 机型 | 虚拟 CPU | 内存（GB） | 链接数量 | QPS | 平均响应时间（usec） |
				|-----|---------|-----------|---------|-----|------------------|
				| AWS m4.xlarge | 4 | 16 | 500 | 235,130 | 55,825 |
				| AWS m4.xlarge | 4 | 16 | 4,000 | 197,173 | 959,657 |
				| AWS m4.2xlarge | 8 | 32 | 900 | 421,958 | 58,496 |
				| AWS m4.2xlarge | 8 | 32 | 7,000 | 298,293 | 642,534 |


		* v0.8.2 版测试数据 (同一局域网):

			| 机型 | 虚拟 CPU | 内存（GB） | 链接数量 | QPS | 平均响应时间（usec） |
			|-----|---------|-----------|---------|-----|------------------|
			| AWS m4.xlarge | 4 | 16 | 600 | 279,718 | 62,366 |
			| AWS m4.xlarge | 4 | 16 | 4,000 | 198,470 | 539,812 |
			| AWS m4.2xlarge | 8 | 32 | 900 | 417,895 | 47,861 |
			| AWS m4.2xlarge | 8 | 32 | 12,000 | 592,472 | 731,977 |
		

	* UDP 测试数据摘要

		* v1.0.0 版本测试数据:

			+ 同一局域网

				| 机型 | 虚拟 CPU | 内存（GB） | 链接数量 | QPS | 平均响应时间（usec） |
				|-----|---------|-----------|---------|-----|------------------|
				| AWS m5.xlarge | 4 | 16 | 10 | 17,628 | 199 |
				|-----|---------|-----------|---------|-----|------------------|
				| AWS m5.xlarge | 4 | 16 | 70 | 123,009 | 1,174 |
				| AWS m5.xlarge | 4 | 16 | 80 | 122,641 | 24,623 |
				| AWS m5.xlarge | 4 | 16 | 800 | 73,030 | 1,306 |
				| AWS m5.xlarge | 4 | 16 | 200 | 95,894 | 400 |
				| AWS m5.xlarge | 4 | 16 | 1,000 | 49,590 | 3,832 |


			+ 洲际传输（德国法兰克福 机房到 美国西部俄勒冈 机房）

				| 机型 | 虚拟 CPU | 内存（GB） | 链接数量 | QPS | 平均响应时间（usec） | ping/2 (msec) |
				|-----|---------|-----------|---------|-----|-------------------|---------------|
				| AWS m5.xlarge | 4 | 16 | 100 | 48,493 | 138,859 | 138 |
				|-----|---------|-----------|---------|-----|-------------------|---------------|
				| AWS m5.xlarge | 4 | 16 | 60 | 107,528 | 139,968 | 140 |
				| AWS m5.xlarge | 4 | 16 | 800 | 79,503 | 139,974 | 138 ~ 139 |



1. 完整测试数据

	以下为 CSV 格式文件，请使用电子表格软件打开。

	**注意**：如果使用 Micorsoft Excel 导入，请选择 “**分隔符号**” 而非默认的 “**固定宽度**”。

	**TCP 测试数据**

	| 版本 | 物理距离 | 机型 | 虚拟 CPU | 内存 (GB) | 增压单位 | 加密模式 | 测试数据 |
	|-----|---------|-----|---------|-----------|--------|--------|---------|
	| v1.0.0 | 同一局域网 | m5.xlarge | 4 | 16 |   10 链接，20,000 QPS，4 工作线程 | N/A | [tcp.stress.aws.m5.xlarge.10.2w.csv](../performances/1.0.0/TCP/localStress/tcp.stress.aws.m5.xlarge.10.2w.csv) |
	| v1.0.0 | 同一局域网 | m5.xlarge | 4 | 16 |  100 链接，10,000 QPS，4 工作线程 | N/A | [tcp.stress.aws.m5.xlarge.100.1w.csv](../performances/1.0.0/TCP/localStress/tcp.stress.aws.m5.xlarge.100.1w.csv) |
	| v1.0.0 | 同一局域网 | m5.xlarge | 4 | 16 |  100 链接，50,000 QPS，4 工作线程 | N/A | [tcp.stress.aws.m5.xlarge.100.5w.csv](../performances/1.0.0/TCP/localStress/tcp.stress.aws.m5.xlarge.100.5w.csv) |
	| v1.0.0 | 同一局域网 | m5.xlarge | 4 | 16 | 1000 链接，50,000 QPS，4 工作线程 | N/A | [tcp.stress.aws.m5.xlarge.1000.5w.csv](../performances/1.0.0/TCP/localStress/tcp.stress.aws.m5.xlarge.1000.5w.csv) |
	| v1.0.0 | 同一局域网 | m4.xlarge | 4 | 16 |   10 链接，20,000 QPS，4 工作线程 | N/A | [tcp.stress.aws.m4.xlarge.10.2w.csv](../performances/1.0.0/TCP/localStress/tcp.stress.aws.m4.xlarge.10.2w.csv) |
	| v1.0.0 | 同一局域网 | m4.xlarge | 4 | 16 |  100 链接，10,000 QPS，4 工作线程 | N/A | [tcp.stress.aws.m4.xlarge.100.1w.csv](../performances/1.0.0/TCP/localStress/tcp.stress.aws.m4.xlarge.100.1w.csv) |
	| v1.0.0 | 同一局域网 | m4.xlarge | 4 | 16 |  100 链接，50,000 QPS，4 工作线程 | N/A | [tcp.stress.aws.m4.xlarge.100.5w.csv](../performances/1.0.0/TCP/localStress/tcp.stress.aws.m4.xlarge.100.5w.csv) |
	| v1.0.0 | 同一局域网 | m4.xlarge | 4 | 16 | 1000 链接，50,000 QPS，4 工作线程 | N/A | [tcp.stress.aws.m4.xlarge.1000.5w.csv](../performances/1.0.0/TCP/localStress/tcp.stress.aws.m4.xlarge.1000.5w.csv) |
	| v1.0.0 | 德国法兰克福 机房到 美国西部俄勒冈 机房 | m5.xlarge | 4 | 16 |   10 链接，20,000 QPS，4 工作线程 | N/A | [tcp.stress.aws.m5.xlarge.10.2w.csv](../performances/1.0.0/TCP/IntercontinentalStress/tcp.stress.aws.m5.xlarge.10.2w.csv) |
	| v1.0.0 | 德国法兰克福 机房到 美国西部俄勒冈 机房 | m5.xlarge | 4 | 16 |  100 链接，10,000 QPS，4 工作线程 | N/A | [tcp.stress.aws.m5.xlarge.100.1w.csv](../performances/1.0.0/TCP/IntercontinentalStress/tcp.stress.aws.m5.xlarge.100.1w.csv) |
	| v1.0.0 | 德国法兰克福 机房到 美国西部俄勒冈 机房 | m5.xlarge | 4 | 16 |  100 链接，50,000 QPS，4 工作线程 | N/A | [tcp.stress.aws.m5.xlarge.100.5w.csv](../performances/1.0.0/TCP/IntercontinentalStress/tcp.stress.aws.m5.xlarge.100.5w.csv) |
	| v1.0.0 | 德国法兰克福 机房到 美国西部俄勒冈 机房 | m5.xlarge | 4 | 16 | 1000 链接，50,000 QPS，4 工作线程 | N/A | [tcp.stress.aws.m5.xlarge.1000.5w.csv](../performances/1.0.0/TCP/IntercontinentalStress/tcp.stress.aws.m5.xlarge.1000.5w.csv) |
	| v1.0.0 | 德国法兰克福 机房到 美国西部俄勒冈 机房 | m4.xlarge | 4 | 16 |   10 链接，20,000 QPS，4 工作线程 | N/A | [tcp.stress.aws.m4.xlarge.10.2w.csv](../performances/1.0.0/TCP/IntercontinentalStress/tcp.stress.aws.m4.xlarge.10.2w.csv) |
	| v1.0.0 | 德国法兰克福 机房到 美国西部俄勒冈 机房 | m4.xlarge | 4 | 16 |  100 链接，10,000 QPS，4 工作线程 | N/A | [tcp.stress.aws.m4.xlarge.100.1w.csv](../performances/1.0.0/TCP/IntercontinentalStress/tcp.stress.aws.m4.xlarge.100.1w.csv) |
	| v1.0.0 | 德国法兰克福 机房到 美国西部俄勒冈 机房 | m4.xlarge | 4 | 16 |  100 链接，50,000 QPS，4 工作线程 | N/A | [tcp.stress.aws.m4.xlarge.100.5w.csv](../performances/1.0.0/TCP/IntercontinentalStress/tcp.stress.aws.m4.xlarge.100.5w.csv) |
	| v1.0.0 | 德国法兰克福 机房到 美国西部俄勒冈 机房 | m4.xlarge | 4 | 16 | 1000 链接，50,000 QPS，4 工作线程 | N/A | [tcp.stress.aws.m4.xlarge.1000.5w.csv](../performances/1.0.0/TCP/IntercontinentalStress/tcp.stress.aws.m4.xlarge.1000.5w.csv) |
	|-----|---------|-----|---------|-----------|--------|--------|---------|
	| v0.9.0 | 同一局域网 | AWS m4.xlarge | 4 | 16 |  100 链接，50,000 QPS，4 工作线程 | N/A | [stress.aws.m4.xlarge.100.w4.csv](../performances/0.9.0/standard/stress.aws.m4.xlarge.100.w4.csv) |
	| v0.9.0 | 同一局域网 | AWS m4.xlarge | 4 | 16 | 1000 链接，50,000 QPS，4 工作线程 | N/A | [stress.aws.m4.xlarge.1000.w4.csv](../performances/0.9.0/standard/stress.aws.m4.xlarge.1000.w4.csv) |
	| v0.9.0 | 同一局域网 | AWS m4.2xlarge | 8 | 32 |  100 链接，50,000 QPS，8 工作线程 | N/A | [stress.aws.m4.2xlarge.100.w8.csv](../performances/0.9.0/standard/stress.aws.m4.2xlarge.100.w8.csv) |
	| v0.9.0 | 同一局域网 | AWS m4.2xlarge | 8 | 32 | 1000 链接，50,000 QPS，8 工作线程 | N/A | [stress.aws.m4.2xlarge.1000.w8.csv](../performances/0.9.0/standard/stress.aws.m4.2xlarge.1000.w8.csv) |
	| v0.9.0 | 同一局域网 | AWS m4.xlarge | 4 | 16 |  100 链接，50,000 QPS，4 工作线程 | SSL/TLS | [stress.aws.m4.xlarge.100.w4.ssl.csv](../performances/0.9.0/ssl/stress.aws.m4.xlarge.100.w4.ssl.csv) |
	| v0.9.0 | 同一局域网 | AWS m4.xlarge | 4 | 16 | 1000 链接，50,000 QPS，4 工作线程 | SSL/TLS | [stress.aws.m4.xlarge.1000.w4.ssl.csv](../performances/0.9.0/ssl/stress.aws.m4.xlarge.1000.w4.ssl.csv) |
	| v0.9.0 | 同一局域网 | AWS m4.2xlarge | 8 | 32 |  100 链接，50,000 QPS，8 工作线程 | SSL/TLS | [stress.aws.m4.2xlarge.100.w8.ssl.csv](../performances/0.9.0/ssl/stress.aws.m4.2xlarge.100.w8.ssl.csv) |
	| v0.9.0 | 同一局域网 | AWS m4.2xlarge | 8 | 32 | 1000 链接，50,000 QPS，8 工作线程 | SSL/TLS | [stress.aws.m4.2xlarge.1000.w8.ssl.csv](../performances/0.9.0/ssl/stress.aws.m4.2xlarge.1000.w8.ssl.csv) |
	| v0.9.0 | 同一局域网 | AWS m4.xlarge | 4 | 16 |  100 链接，50,000 QPS，4 工作线程 | FPNN 加密，包加密，128 位密钥 | [stress.aws.m4.xlarge.100.w4.ecc.package.128bits.csv](../performances/0.9.0/eccEncrypted/stress.aws.m4.xlarge.100.w4.ecc.package.128bits.csv) |
	| v0.9.0 | 同一局域网 | AWS m4.xlarge | 4 | 16 | 1000 链接，50,000 QPS，4 工作线程 | FPNN 加密，包加密，128 位密钥 | [stress.aws.m4.xlarge.1000.w4.ecc.package.128bits.csv](../performances/0.9.0/eccEncrypted/stress.aws.m4.xlarge.1000.w4.ecc.package.128bits.csv) |
	| v0.9.0 | 同一局域网 | AWS m4.2xlarge | 8 | 32 |  100 链接，50,000 QPS，8 工作线程 | FPNN 加密，包加密，128 位密钥 | [stress.aws.m4.2xlarge.100.w8.ecc.package.128bits.csv](../performances/0.9.0/eccEncrypted/stress.aws.m4.2xlarge.100.w8.ecc.package.128bits.csv) |
	| v0.9.0 | 同一局域网 | AWS m4.2xlarge | 8 | 32 | 1000 链接，50,000 QPS，8 工作线程 | FPNN 加密，包加密，128 位密钥 | [stress.aws.m4.2xlarge.1000.w8.ecc.package.128bits.csv](../performances/0.9.0/eccEncrypted/stress.aws.m4.2xlarge.1000.w8.ecc.package.128bits.csv) |
	| v0.9.0 | 同一局域网 | AWS m4.xlarge | 4 | 16 |  100 链接，50,000 QPS，4 工作线程 | FPNN 加密，包加密，256 位密钥 | [stress.aws.m4.xlarge.100.w4.ecc.package.256bits.csv](../performances/0.9.0/eccEncrypted/stress.aws.m4.xlarge.100.w4.ecc.package.256bits.csv) |
	| v0.9.0 | 同一局域网 | AWS m4.xlarge | 4 | 16 | 1000 链接，50,000 QPS，4 工作线程 | FPNN 加密，包加密，256 位密钥 | [stress.aws.m4.xlarge.1000.w4.ecc.package.256bits.csv](../performances/0.9.0/eccEncrypted/stress.aws.m4.xlarge.1000.w4.ecc.package.256bits.csv) |
	| v0.9.0 | 同一局域网 | AWS m4.2xlarge | 8 | 32 |  100 链接，50,000 QPS，8 工作线程 | FPNN 加密，包加密，256 位密钥 | [stress.aws.m4.2xlarge.100.w8.ecc.package.256bits.csv](../performances/0.9.0/eccEncrypted/stress.aws.m4.2xlarge.100.w8.ecc.package.256bits.csv) |
	| v0.9.0 | 同一局域网 | AWS m4.2xlarge | 8 | 32 | 1000 链接，50,000 QPS，8 工作线程 | FPNN 加密，包加密，256 位密钥 | [stress.aws.m4.2xlarge.1000.w8.ecc.package.256bits.csv](../performances/0.9.0/eccEncrypted/stress.aws.m4.2xlarge.1000.w8.ecc.package.256bits.csv) |
	| v0.9.0 | 同一局域网 | AWS m4.xlarge | 4 | 16 |  100 链接，50,000 QPS，4 工作线程 | FPNN 加密，流加密，128 位密钥 | [stress.aws.m4.xlarge.100.w4.ecc.stream.128bits.csv](../performances/0.9.0/eccEncrypted/stress.aws.m4.xlarge.100.w4.ecc.stream.128bits.csv) |
	| v0.9.0 | 同一局域网 | AWS m4.xlarge | 4 | 16 | 1000 链接，50,000 QPS，4 工作线程 | FPNN 加密，流加密，128 位密钥 | [stress.aws.m4.xlarge.1000.w4.ecc.stream.128bits.csv](../performances/0.9.0/eccEncrypted/stress.aws.m4.xlarge.1000.w4.ecc.stream.128bits.csv) |
	| v0.9.0 | 同一局域网 | AWS m4.2xlarge | 8 | 32 |  100 链接，50,000 QPS，8 工作线程 | FPNN 加密，流加密，128 位密钥 | [stress.aws.m4.2xlarge.100.w8.ecc.stream.128bits.csv](../performances/0.9.0/eccEncrypted/stress.aws.m4.2xlarge.100.w8.ecc.stream.128bits.csv) |
	| v0.9.0 | 同一局域网 | AWS m4.2xlarge | 8 | 32 | 1000 链接，50,000 QPS，8 工作线程 | FPNN 加密，流加密，128 位密钥 | [stress.aws.m4.2xlarge.1000.w8.ecc.stream.128bits.csv](../performances/0.9.0/eccEncrypted/stress.aws.m4.2xlarge.1000.w8.ecc.stream.128bits.csv) |
	| v0.9.0 | 同一局域网 | AWS m4.xlarge | 4 | 16 |  100 链接，50,000 QPS，4 工作线程 | FPNN 加密，流加密，256 位密钥 | [stress.aws.m4.xlarge.100.w4.ecc.stream.256bits.csv](../performances/0.9.0/eccEncrypted/stress.aws.m4.xlarge.100.w4.ecc.stream.256bits.csv) |
	| v0.9.0 | 同一局域网 | AWS m4.xlarge | 4 | 16 | 1000 链接，50,000 QPS，4 工作线程 | FPNN 加密，流加密，256 位密钥 | [stress.aws.m4.xlarge.1000.w4.ecc.stream.256bits.csv](../performances/0.9.0/eccEncrypted/stress.aws.m4.xlarge.1000.w4.ecc.stream.256bits.csv) |
	| v0.9.0 | 同一局域网 | AWS m4.2xlarge | 8 | 32 |  100 链接，50,000 QPS，8 工作线程 | FPNN 加密，流加密，256 位密钥 | [stress.aws.m4.2xlarge.100.w8.ecc.stream.256bits.csv](../performances/0.9.0/eccEncrypted/stress.aws.m4.2xlarge.100.w8.ecc.stream.256bits.csv) |
	| v0.9.0 | 同一局域网 | AWS m4.2xlarge | 8 | 32 | 1000 链接，50,000 QPS，8 工作线程 | FPNN 加密，流加密，256 位密钥 | [stress.aws.m4.2xlarge.1000.w8.ecc.stream.256bits.csv](../performances/0.9.0/eccEncrypted/stress.aws.m4.2xlarge.1000.w8.ecc.stream.256bits.csv) |
	|-----|---------|-----|---------|-----------|--------|--------|---------|
	| v0.8.2 | 同一局域网 | AWS m4.xlarge | 4 | 16 |  100 链接，50,000 QPS， 4 工作线程 | N/A | [stress.aws.m4.xlarge.100.w4.csv](../performances/0.8.2/stress.aws.m4.xlarge.100.w4.csv) |
	| v0.8.2 | 同一局域网 | AWS m4.xlarge | 4 | 16 |  100 链接，50,000 QPS， 8 工作线程 | N/A | [stress.aws.m4.xlarge.100.w8.csv](../performances/0.8.2/stress.aws.m4.xlarge.100.w8.csv) |
	| v0.8.2 | 同一局域网 | AWS m4.xlarge | 4 | 16 | 1000 链接，50,000 QPS， 4 工作线程 | N/A | [stress.aws.m4.xlarge.1000.w4.csv](../performances/0.8.2/stress.aws.m4.xlarge.1000.w4.csv) |
	| v0.8.2 | 同一局域网 | AWS m4.xlarge | 4 | 16 | 1000 链接，50,000 QPS， 8 工作线程 | N/A | [stress.aws.m4.xlarge.1000.w8.csv](../performances/0.8.2/stress.aws.m4.xlarge.1000.w8.csv) |
	| v0.8.2 | 同一局域网 | AWS m4.2xlarge | 8 | 32 |  100 链接，50,000 QPS， 8 工作线程 | N/A | [stress.aws.m4.2xlarge.100.w8.csv](../performances/0.8.2/stress.aws.m4.2xlarge.100.w8.csv) |
	| v0.8.2 | 同一局域网 | AWS m4.2xlarge | 8 | 32 |  100 链接，50,000 QPS，16 工作线程 | N/A | [stress.aws.m4.2xlarge.100.w16.csv](../performances/0.8.2/stress.aws.m4.2xlarge.100.w16.csv) |
	| v0.8.2 | 同一局域网 | AWS m4.2xlarge | 8 | 32 | 1000 链接，50,000 QPS， 8 工作线程 | N/A | [stress.aws.m4.2xlarge.1000.w8.csv](../performances/0.8.2/stress.aws.m4.2xlarge.1000.w8.csv) |
	| v0.8.2 | 同一局域网 | AWS m4.2xlarge | 8 | 32 | 1000 链接，50,000 QPS，16 工作线程 | N/A | [stress.aws.m4.2xlarge.1000.w16.csv](../performances/0.8.2/stress.aws.m4.2xlarge.1000.w16.csv) |

	**UDP 测试数据**

	| 版本 | 物理距离 | 机型 | 虚拟 CPU | 内存 (GB) | 增压单位 | 加密模式 | 测试数据 |
	|-----|---------|-----|---------|-----------|--------|--------|---------|
	| v1.0.0 | 同一局域网 | m5.xlarge | 4 | 16 |   10 链接，20,000 QPS，4 工作线程 | N/A | [udp.stress.aws.m5.xlarge.10.2w.csv](../performances/1.0.0/UDP/localStress/udp.stress.aws.m5.xlarge.10.2w.csv) |
	| v1.0.0 | 同一局域网 | m5.xlarge | 4 | 16 |  100 链接，10,000 QPS，4 工作线程 | N/A | [udp.stress.aws.m5.xlarge.100.1w.csv](../performances/1.0.0/UDP/localStress/udp.stress.aws.m5.xlarge.100.1w.csv) |
	| v1.0.0 | 同一局域网 | m5.xlarge | 4 | 16 |  100 链接，50,000 QPS，4 工作线程 | N/A | [udp.stress.aws.m5.xlarge.100.5w.csv](../performances/1.0.0/UDP/localStress/udp.stress.aws.m5.xlarge.100.5w.csv) |
	| v1.0.0 | 同一局域网 | m5.xlarge | 4 | 16 | 1000 链接，50,000 QPS，4 工作线程 | N/A | [udp.stress.aws.m5.xlarge.1000.5w.csv](../performances/1.0.0/UDP/localStress/udp.stress.aws.m5.xlarge.1000.5w.csv) |
	| v1.0.0 | 德国法兰克福 机房到 美国西部俄勒冈 机房 | m5.xlarge | 4 | 16 |   10 链接，20,000 QPS，4 工作线程 | N/A | [udp.stress.aws.m5.xlarge.10.2w.csv](../performances/1.0.0/UDP/IntercontinentalStress/udp.stress.aws.m5.xlarge.10.2w.csv) |
	| v1.0.0 | 德国法兰克福 机房到 美国西部俄勒冈 机房 | m5.xlarge | 4 | 16 |  100 链接，10,000 QPS，4 工作线程 | N/A | [udp.stress.aws.m5.xlarge.100.1w.csv](../performances/1.0.0/UDP/IntercontinentalStress/udp.stress.aws.m5.xlarge.100.1w.csv) |
	| v1.0.0 | 德国法兰克福 机房到 美国西部俄勒冈 机房 | m5.xlarge | 4 | 16 |  100 链接，50,000 QPS，4 工作线程 | N/A | [udp.stress.aws.m5.xlarge.100.5w.csv](../performances/1.0.0/UDP/IntercontinentalStress/udp.stress.aws.m5.xlarge.100.5w.csv) |
	| v1.0.0 | 德国法兰克福 机房到 美国西部俄勒冈 机房 | m5.xlarge | 4 | 16 | 1000 链接，50,000 QPS，4 工作线程 | N/A | [udp.stress.aws.m5.xlarge.1000.5w.csv](../performances/1.0.0/UDP/IntercontinentalStress/udp.stress.aws.m5.xlarge.1000.5w.csv) |


### 2. 海量链接测试

1. 目标机型

	FPNN 1.0.0 版本开始，海量链接测试，压力源使用亚马逊 m5.xlarge 机型，测试服务器使用 m5.2xlarge 机型。早期版本海量链接测试仅使用亚马逊 m4.xlarge 机型。

	+ m5.xlarge 机型为虚拟 4 核 CPU，16 GB 内存
	+ m5.2xlarge 机型为虚拟 8 核 CPU，32 GB 内存
	+ m4.xlarge 机型为虚拟 4 核 CPU，16 GB 内存

1. 目标操作系统

	FPNN 1.0.0 版本开始，测试服务器使用 CentOS 8，压力源使用 CentOS 7。早期版本压力测试均使用 CentOS 7。

	**注意**

	FPNN UDP 服务端需要 Linux 3.9 及以上内核，否则性能会明显降低。

1. 测试准备

	+ Deployer/Actor 执行机

		1. 修改 /etc/security/limits.conf 文件，增加可用文件描述符数量。

			增加以下条目：

					*		soft	nofile		100000
					*		hard	nofile		100000

			**注意**：nofile 数量不能大于机器的物理限制数量。  
			机器的物理限制数量可参见 /proc/sys/fs/file-max

		1. 根据需要修改 /etc/sysctl.conf 文件

			如果当前账户可用最大文件描述符小于修改后的 nofile 参数数量，则需要修改 /etc/sysctl.conf 文件，编辑/增加

					fs.nr_open = 100000

			当前账户可用最大文件描述符数量可参考

					sysctl -a | grep fs.nr_open

		1. 修改以上配置文件后，重启 Deployer/Actor 执行机


	+ 目标服务器

		1. 修改 /etc/security/limits.conf 文件，增加可用文件描述符数量。

			增加以下条目：

					*		soft	nofile		2500000
					*		hard	nofile		2500000

			**注意**：nofile 数量不能大于机器的物理限制数量。  
			机器的物理限制数量可参见 /proc/sys/fs/file-max

		1. 修改 /etc/sysctl.conf 文件

			增加以下条目：

					fs.nr_open = 2620000
					net.core.somaxconn = 65535
					net.ipv4.tcp_max_syn_backlog = 10000

			当前账户可用最大文件描述符数量可参考

					sysctl -a | grep fs.nr_open

		1. 修改以上配置文件后，重启目标服务器

	+ FPNN 测试服务配置文件

		+ 修改 FPNN.server.log.level 条目为

				FPNN.server.log.level = ERROR

		+ 增加以下条目

				FPNN.server.idle.timeout = 150
				FPNN.server.rlimit.max.nofile = 2230000
				FPNN.server.max.connections = 2230000


1. 测试过程

	TCP 使用 TCPMassiveConnectionsTest 模块进行测试，UDP 使用 UDPMassiveConnectionsTest 模块进行测试。

	+ TCPMassiveConnectionsTest 模块位置： [/core/test/AutoDistributedTest/TCPMassiveConnectionsTest/](../../core/test/AutoDistributedTest/TCPMassiveConnectionsTest/)
	+ UDPMassiveConnectionsTest 模块位置： [/core/test/AutoDistributedTest/UDPMassiveConnectionsTest/](../../core/test/AutoDistributedTest/UDPMassiveConnectionsTest/)

	启动 TCPMassConnController 或者 UDPMassConnController，然后等待测试完成即可。测试时间随总链接数量和总 QPS 变化。短则 20 分钟，多则一天以上。

	+ TCPMassConnController 启动参数请参见 [FPNN 内置工具](fpnn-tools.md#TCPMassiveConnectionsTest) 对应说明；
	+ UDPMassConnController 启动参数请参见 [FPNN 内置工具](fpnn-tools.md#UDPMassiveConnectionsTest) 对应说明。

1. 测试数据摘要

	* TCP 测试数据摘要

		* v1.0.0 版测试数据 (同一局域网):

			| 机型 | 虚拟 CPU | 内存（GB） | 链接数量 | QPS | 平均响应时间（usec） |
			|-----|---------|-----------|---------|-----|------------------|
			| AWS m5.2xlarge | 8 | 32 | 2,040,000 | 81,351 | 446 |
			| AWS m5.2xlarge | 8 | 32 | 2,040,000 | 137,294 | 4,985 |
			| AWS m5.2xlarge | 8 | 32 | 2,040,000 | 179,794 | 11,345 |

		* v0.8.2 版测试数据 (同一局域网):

			| 机型 | 虚拟 CPU | 内存（GB） | 链接数量 | QPS | 平均响应时间（usec） |
			|-----|---------|-----------|---------|-----|------------------|
			| AWS m4.xlarge | 4 | 16 | 1,200,000 | 23,978 | 435 |

	* UDP 测试数据摘要

		* v1.0.0 版测试数据 (同一局域网):

			| 机型 | 虚拟 CPU | 内存（GB） | 链接数量 | QPS | 平均响应时间（usec） |
			|-----|---------|-----------|---------|-----|------------------|
			| AWS m5.2xlarge | 8 | 32 | 12,200 | 5,957 | 11,480 |
			| AWS m5.2xlarge | 8 | 32 | 22,101 | 4,388 | 86,325 |
			| AWS m5.2xlarge | 8 | 32 | 11,349 | 6,311 | 50,323 |
			| AWS m5.2xlarge | 8 | 32 | 15,865 | 4,706 | 85,613 |
			| AWS m5.2xlarge | 8 | 32 | 19,000 | 721 | 1,857 |
			| AWS m5.2xlarge | 8 | 32 | 22,000 | 815 | 10,773 |

1. 完整测试数据

	以下为 CSV 格式文件，请使用电子表格软件打开。

	**注意**：如果使用 Micorsoft Excel 导入，请选择 “**分隔符号**” 而非默认的 “**固定宽度**”。

	**TCP 测试数据**

	| 版本 | 物理距离 | 机型 | 虚拟 CPU | 内存 (GB) | 增压单位 | 加密模式 | 测试数据 |
	|-----|---------|-----|---------|-----------|--------|--------|---------|
	| v1.0.0 | 同一局域网 | m5.2xlarge | 8 | 32 | 固定压力模式，100 线程，1,000 链接，每个链接 0.04 QPS，4 工作线程 | N/A | [tcp.massClient.m5.2xlarge.100.1w.0.04.csv](../performances/1.0.0/TCP/localMassiveConnections/tcp.massClient.m5.2xlarge.100.1w.0.04.csv) |
	| v1.0.0 | 同一局域网 | m5.2xlarge | 8 | 32 | 固定压力模式，100 线程，1,000 链接，每个链接 0.07 QPS，4 工作线程 | N/A | [tcp.massClient.m5.2xlarge.100.1w.0.07.csv](../performances/1.0.0/TCP/localMassiveConnections/tcp.massClient.m5.2xlarge.100.1w.0.07.csv) |
	| v1.0.0 | 同一局域网 | m5.2xlarge | 8 | 32 | 固定压力模式，100 线程，1,000 链接，每个链接 0.10 QPS，4 工作线程 | N/A | [tcp.massClient.m5.2xlarge.100.1w.0.10.csv](../performances/1.0.0/TCP/localMassiveConnections/tcp.massClient.m5.2xlarge.100.1w.0.10.csv) |
	| v1.0.0 | 同一局域网 | m5.2xlarge | 8 | 32 | 固定压力模式，100 线程，1,000 链接，每个链接 0.50 QPS，4 工作线程 | N/A | [tcp.massClient.m5.2xlarge.100.1w.0.50.csv](../performances/1.0.0/TCP/localMassiveConnections/tcp.massClient.m5.2xlarge.100.1w.0.50.csv) |
	| v1.0.0 | 同一局域网 | m5.2xlarge | 8 | 32 | 固定压力模式，100 线程，1,000 链接，每个链接 1.00 QPS，4 工作线程 | N/A | [tcp.massClient.m5.2xlarge.100.1w.1.00.csv](../performances/1.0.0/TCP/localMassiveConnections/tcp.massClient.m5.2xlarge.100.1w.1.00.csv) |
	|-----|---------|-----|---------|-----------|--------|--------|---------|
	| v0.8.2 | 同一局域网 | m4.xlarge | 4 | 16 | 固定压力模式，100 线程，10,000 链接，每个链接 0.01 QPS，4 工作线程 | N/A | [massClient.aws.m4.xlarge.100.1w.0.01.csv](../performances/0.8.2/massClient.aws.m4.xlarge.100.1w.0.01.csv) |
	| v0.8.2 | 同一局域网 | m4.xlarge | 4 | 16 | 固定压力模式，100 线程，10,000 链接，每个链接 0.02 QPS，4 工作线程 | N/A | [massClient.aws.m4.xlarge.100.1w.0.02.csv](../performances/0.8.2/massClient.aws.m4.xlarge.100.1w.0.02.csv) |
	| v0.8.2 | 同一局域网 | m4.xlarge | 4 | 16 | 自动增压模式，100 线程，10,000 链接，每个链接 0.02 QPS，4 工作线程 | N/A | [autoBoost.massClient.aws.m4.xlarge.100.1w.0.02.csv](../performances/0.8.2/autoBoost.massClient.aws.m4.xlarge.100.1w.0.02.csv) |

	**UDP 测试数据**

	| 版本 | 物理距离 | 机型 | 虚拟 CPU | 内存 (GB) | 增压单位 | 加密模式 | 测试数据 |
	|-----|---------|-----|---------|-----------|--------|--------|---------|
	| v1.0.0 | 同一局域网 | m5.2xlarge | 8 | 32 | 固定压力模式，10 线程，1,000 链接，每个链接 0.04 QPS，4 工作线程 | N/A | [udp.massClient.m5.2xlarge.10.1k.0.04.csv](../performances/1.0.0/UDP/localMassiveConnections/udp.massClient.m5.2xlarge.10.1k.0.04.csv) |
	| v1.0.0 | 同一局域网 | m5.2xlarge | 8 | 32 | 固定压力模式，10 线程，1,000 链接，每个链接 5 QPS，4 工作线程 | N/A | [udp.massClient.m5.2xlarge.10.1k.5.0.csv](../performances/1.0.0/UDP/localMassiveConnections/udp.massClient.m5.2xlarge.10.1k.5.0.csv) |
	| v1.0.0 | 同一局域网 | m5.2xlarge | 8 | 32 | 固定压力模式，10 线程，1,000 链接，每个链接 1 QPS，4 工作线程 | N/A | [udp.massClient.m5.xlarge.10.1k.1.0.csv](../performances/1.0.0/UDP/localMassiveConnections/udp.massClient.m5.xlarge.10.1k.1.0.csv) |
	| v1.0.0 | 同一局域网 | m5.2xlarge | 8 | 32 | 固定压力模式，10 线程，1,000 链接，每个链接 20 QPS，4 工作线程 | N/A | [udp.massClient.m5.xlarge.10.1k.20.0.csv](../performances/1.0.0/UDP/localMassiveConnections/udp.massClient.m5.xlarge.10.1k.20.0.csv) |
