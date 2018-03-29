# FPNN 安装与集成

## 1. 环境需求

| 工具 | 最低版本 |
|------|--------|
| g++ | 4.8.5 |
| CentOS | 7.0 |

FPNN 框架目前适配于 CentOS，其他 Linux 发行版暂无适配。

FPNN 采用 C++11 编写，如果采用其他版本的编译器，需要支持 C++11 所有语法特性。


## 2. 第三方库依赖

+ gcc
+ g++
+ libcurl
+ tcmalloc
+ openssl

如果不使用 <fpnn-folder>/extends/ 下的组件，则无需 openssl。

+ 初始化安装环境

	在一台干净的 CentOS 7 上，执行一下命令，即可初始化 FPNN 编译环境：

		yum -y groupinstall 'Development tools'
		yum install -y libcurl-devel
		yum install -y openssl-devel
		yum install -y gperftools


## 3. 编译 FPNN 框架

在 <fpnn-folder>/ 内执行`make`即可。


## 4. 项目开发

请参见

1. [FPNN 服务端基础使用说明](fpnn-server-basic-tutorial.md)
1. [FPNN 服务端高级使用说明](fpnn-server-advanced-tutorial.md)
1. [FPNN 客户端基础使用说明](fpnn-client-basic-tutorial.md)
1. [FPNN 客户端高级使用说明](fpnn-client-advanced-tutorial.md)