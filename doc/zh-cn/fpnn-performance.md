# FPNN 性能报告

## 性能测试说明

本测试报告所有数据均由 FPNN 自动化分布式测试工具测试生成。  
FPNN 自动化分布式测试工具需要在 **DATS (Distributed Automated Testing Suite)** 下使用。

### 1. 压力测试

1. 目标机型

压力测试使用亚马逊 m4.xlarge 机型和 m4.2xlarge 机型。

	+ m4.xlarge 机型为虚拟 4 核 CPU，16 GB 内存
	+ m4.2xlarge 机型为虚拟 8 核 CPU，32 GB 内存

1. 测试准备

	+ 目标服务器

		1. 修改 /etc/security/limits.conf 文件，增加可用文件描述符数量。

			参考：

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

		1. 修改以上配置文件后，重启目标服务器


1. 测试过程

使用 StressTest 模块进行测试。

StressTest 模块位置： <fpnn-folder>/core/test/AutoDistributedTest/StressTest/

启动 StressController，然后等待测试完成即可。测试时间一般在 1 小时以内。

StressController 启动参数请参见 [FPNN 内置工具](fpnn-tools.md) 对应说明。

1. 测试数据摘要

| 机型 | CPU | 内存（GB） | 链接数量 | QPS | 平均响应时间（usec） |
|-----|-----|-----------|---------|-----|------------------|
| AWS m4.xlarge | 4 | 16 | 600 | 279,718 | 62,366 |
| AWS m4.xlarge | 4 | 16 | 4,000 | 198,470 | 539,812 |
| AWS m4.2xlarge | 8 | 32 | 900 | 417,895 | 47,861 |
| AWS m4.2xlarge | 8 | 32 | 12,000 | 592,472 | 731,977 |

1. 完整测试数据

以下为 CSV 格式文件，请使用电子表格软件打开。

**注意**：如果使用 Micorsoft Excel 导入，请选择 “**分隔符号**” 而非默认的 “**固定宽度**”。

[AWS m4.xlarge，单位压力100链接，4工作线程](../performances/0.8.2/stress.aws.m4.xlarge.100.w4.csv)

[AWS m4.xlarge，单位压力100链接，8工作线程](../performances/0.8.2/stress.aws.m4.xlarge.100.w8.csv)

[AWS m4.xlarge，单位压力1000链接，4工作线程](../performances/0.8.2/stress.aws.m4.xlarge.1000.w4.csv)

[AWS m4.xlarge，单位压力1000链接，8工作线程](../performances/0.8.2/stress.aws.m4.xlarge.1000.w8.csv)


[AWS m4.2xlarge，单位压力100链接，8工作线程](../performances/0.8.2/stress.aws.m4.2xlarge.100.w8.csv)

[AWS m4.2xlarge，单位压力100链接，16工作线程](../performances/0.8.2/stress.aws.m4.2xlarge.100.w16.csv)

[AWS m4.2xlarge，单位压力1000链接，8工作线程](../performances/0.8.2/stress.aws.m4.2xlarge.1000.w8.csv)

[AWS m4.2xlarge，单位压力1000链接，16工作线程](../performances/0.8.2/stress.aws.m4.2xlarge.1000.w16.csv)


### 2. 海量链接测试

1. 目标机型

海量链接测试仅使用亚马逊 m4.xlarge 机型。

	+ m4.xlarge 机型为虚拟 4 核 CPU，16 GB 内存

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

					*		soft	nofile		1260000
					*		hard	nofile		1260000

			**注意**：nofile 数量不能大于机器的物理限制数量。  
			机器的物理限制数量可参见 /proc/sys/fs/file-max

		1. 修改 /etc/sysctl.conf 文件

			增加以下条目：

					fs.nr_open = 1620000
					net.core.somaxconn = 65535
					net.ipv4.tcp_max_syn_backlog = 10000

			当前账户可用最大文件描述符数量可参考

					sysctl -a | grep fs.nr_open

		1. 修改以上配置文件后，重启目标服务器

	+ FPNN 测试服务配置文件

			FPNN.server.log.level = ERROR
			FPNN.server.idle.timeout = 150
			FPNN.server.rlimit.max.nofile = 1230000
			FPNN.server.max.connections = 1230000


1. 测试过程

使用 MassiveConnectionsTest 模块进行测试。

MassiveConnectionsTest 模块位置： <fpnn-folder>/core/test/AutoDistributedTest/MassiveConnectionsTest/

启动 MassConnController，然后等待测试完成即可。测试时间随总链接数量和总QPS变化。短则 20 分钟，多则一天以上。

MassConnController 启动参数请参见 [FPNN 内置工具](fpnn-tools.md) 对应说明。

1. 测试数据摘要

| 机型 | CPU | 内存（GB） | 链接数量 | QPS | 平均响应时间（usec） |
|-----|-----|-----------|---------|-----|------------------|
| AWS m4.xlarge | 4 | 16 | 1,200,000 | 23,978 | 435 |

1. 完整测试数据

以下为 CSV 格式文件，请使用电子表格软件打开。

**注意**：如果使用 Micorsoft Excel 导入，请选择 “**分隔符号**” 而非默认的 “**固定宽度**”。

[AWS m4.xlarge，固定模式，单位压力100线程，10000客户端，单客户端100秒一个请求](../performances/0.8.2/massClient.aws.m4.xlarge.100.1w.0.01.csv)

[AWS m4.xlarge，固定模式，单位压力100线程，10000客户端，单客户端50秒一个请求](../performances/0.8.2/massClient.aws.m4.xlarge.100.1w.0.02.csv)

[AWS m4.xlarge，自动增压模式，单位压力100线程，10000客户端，单客户端50秒一个请求](../performances/0.8.2/autoBoost.massClient.aws.m4.xlarge.100.1w.0.02.csv)
