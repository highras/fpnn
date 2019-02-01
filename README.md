# FPNN 介绍

**FPNN一句话总结：**

+ **客户端**：一个API解决所有操作
+ **服务器**：一个类继承解决所有RPC，同步异步，编码解码问题。

## 一、开发背景

没有人想重复制造轮子。

但，当所有的轮都不好使的时候，怎么办？

鉴于之前的项目使用Thrift开发遇到了太多的坑，耗费大量的资源解决Thrift本身问题。  
基于对公司基础架构工具的准备及技术积累，结合之前对ICE、ACE、Codra等知名RPC框架的使用经验，于是决定开发一个更适合公司业务及基础架构的RPC框架。

轮子没有最好，只有最合适。

RPC框架也一样。

## 二、FPNN框架提供功能

* 支持 IPv4
* 支持 IPv6
* 支持 TCP 二进制私有协议
* 支持 HTTP 1.0
* 支持 WebSocket
* 支持 SSL/TLS
* 支持 msgpack 编码
* 支持 json 格式
* 支持 可选参数
* 支持 不定类型参数
* 支持 不定长度不定类型参数
* 支持 接口灰度兼容
* 支持 TCP 二进制私有协议和 HTTP Json 格式互转

* 支持 Server Push
* 支持 异步操作
* 支持 同步操作
* 支持 Lambda 函数
* 支持 动态调整系统级参数
* 支持 动态调整框架级参数
* 支持 实时查看服务运行状态
* 支持 实时查看各参数状态
* 支持 failover
* 支持 统一Log汇总
* 支持 优雅退出
* 支持 统一处理异常
* 支持 同一端口多种协议(TCP/HTTP/WebSocket)
* 支持 应答提前返回
* 支持 应答异步/延后返回
* 支持 请求响应时间统计
* 支持 QPS 统计
* 支持 慢请求统计
* 支持 优先执行系统内置命令

* 支持 AES 加密
* 支持 ECDH 秘钥交换
* 支持 128 位或 256 位秘钥
* 支持 IP 白名单
* 支持 IP 段白名单
* 支持 访问用户自定义接口时的加密限制
* 支持 访问用户自定义接口时的内网限制

在 FPNN 技术生态中，提供以下额外功能

* 支持 集群注册
* 支持 集群管理
* 支持 数据协调
* 支持 数据同步
* 支持 大规模分布式测试部署、监控、协调
* 支持 FPNN 集群透明代理
* 支持 MySQL 透明代理
* 支持 数据行级缓存

## 三、FPNN框架适用范围及性能

1. FPNN 框架适用范围

	* 分布式缓存系统
	* 后台基于TCP协议的 server
	* 后台基于HTTP协议的 server
	* 业务后台系统
	* 需要高性能服务的系统
	* 其他RPC系统

1. FPNN 框架性能
	
	压力测试(v0.8.2)：

	| 机型 | CPU | 内存（GB） | 链接数量 | QPS | 平均响应时间（usec） |
	|-----|-----|-----------|---------|-----|------------------|
	| AWS m4.xlarge | 4 | 16 | 600 | 279,718 | 62,366 |
	| AWS m4.xlarge | 4 | 16 | 4,000 | 198,470 | 539,812 |
	| AWS m4.2xlarge | 8 | 32 | 900 | 417,895 | 47,861 |
	| AWS m4.2xlarge | 8 | 32 | 12,000 | 592,472 | 731,977 |

	海量链接(v0.8.2)：
	
	| 机型 | CPU | 内存（GB） | 链接数量 | QPS | 平均响应时间（usec） |
	|-----|-----|-----------|---------|-----|------------------|
	| AWS m4.xlarge | 4 | 16 | 1,200,000 | 23,978 | 435 |
	

	具体性能介绍及更多数据请参见 [FPNN 性能报告](doc/zh-cn/fpnn-performance.md)


## 四、使用

1. 环境需求

	| 工具 | 最低版本 |
	|------|--------|
	| g++ | 4.8.5 |
	| CentOS | 7.0 |

1. 第三方库依赖

	+ gcc
	+ g++
	+ libcurl
	+ tcmalloc
	+ openssl

1. 编译安装FPNN框架

	在 <fpnn-folder>/ 内执行`make`即可。

	详细可参见 [FPNN 安装与集成](doc/zh-cn/fpnn-install.md)

1. 使用 FPNN 框架开发

	请参见

	1. [FPNN 服务端基础使用说明](doc/zh-cn/fpnn-server-basic-tutorial.md)
	1. [FPNN 服务端高级使用说明](doc/zh-cn/fpnn-server-advanced-tutorial.md)
	1. [FPNN 客户端基础使用说明](doc/zh-cn/fpnn-client-basic-tutorial.md)
	1. [FPNN 客户端高级使用说明](doc/zh-cn/fpnn-client-advanced-tutorial.md)

1. 注意事项 & 问题排查

	注意事项请参见 [FPNN 注意事项](doc/zh-cn/fpnn-notices.md)

	问题排查请参见 [FPNN 问题排查](doc/zh-cn/fpnn-troubleshooting.md)

## 五、全部文档索引

[中文文档索引](doc/zh-cn/README.md)
 
