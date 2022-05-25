# FPNN 安装与集成

## 1. 环境需求

FPNN 框架目前适配的操作系统和编译器：

| 操作系统 | 编译器 |
|---------|-------|
| CentOS 7 | GCC/G++ 4.8.5 |
| CentOS 8 | GCC/G++ 8 |
| Ubuntu 20 | GCC/G++ 9 |
| MacOS 11.5 | XCode/Apple clang 12 |

FPNN 采用 C++11 编写，如果采用其他版本的编译器，需要支持 C++11 所有语法特性。


## 2. 第三方库依赖

必须项：

+ gcc
+ g++
+ libcurl
+ openssl

可选项：

+ tcmalloc (默认依赖)
+ nvidia-ml (默认不启用 CUDA 支持则不依赖)

MacOS 平台编译，无需 tcmalloc 库。


+ 初始化安装环境

	* CentOS 7:

			yum -y groupinstall 'Development tools'
			yum install -y libcurl-devel
			yum install -y openssl-devel
			yum install -y gperftools

	* CentOS 8:

			dnf install epel-release
			dnf -y groupinstall 'Development tools'
			dnf install -y libcurl-devel
			dnf install -y openssl-devel
			dnf install -y gperftools

	* Ubuntu:

			apt install build-essential
			apt install libcurl4-openssl-dev
			apt install openssl
			apt install libssl-dev
			apt install zlib1g-dev
			apt install -y google-perftools

			ln -s /lib/x86_64-linux-gnu/libtcmalloc.so.4 /lib/x86_64-linux-gnu/libtcmalloc.so

	* MacOS:

		* 安装 XCode
		* 安装 openssl

				brew install openssl@1.1


## 3. 平台适配

### 3.1 Linux 系统

FPNN 框架在 亚马逊 AWS、谷歌 GCP、微软 Azure、腾讯云、阿里云 五个平台上，可自动获取网络相关配置。

FPNN 默认启动 亚马逊 AWS 支持。若非 亚马逊 AWS 平台，请修改 FPNN 全局预置配置文件 [def.mk](../../def.mk) 中，DEFAULTPLATFORM 参数。

若非 亚马逊 AWS、谷歌 GCP、微软 Azure、腾讯云、阿里云 五个平台，请禁用 DEFAULTPLATFORM 参数。  
并在服务实际运行时，参考 [FPNN 注意事项](fpnn-notices.md) “平台配置”部分，增加配置文件配置条目。

### 3.2 MacOS 系统

请确认 FPNN 全局预置配置文件 [def.mk](../../def.mk) MACOS_SSL_INCLUDE_PATH 与 MACOS_SSL_LIB_PATH 两个参数是否符合当前配置。

如果 MACOS_SSL_INCLUDE_PATH 与 MACOS_SSL_LIB_PATH 两个参数默认值与 ssl 开发库实际安装路径不一致，请根据实际安装路径修改。


## 4. 编译 FPNN 框架

在 [FPNN 源代码根目录](https://github.com/highras/fpnn) 内执行 `make` 即可。


## 5. 项目开发

请参见

1. [FPNN 服务端基础使用向导](fpnn-server-basic-tutorial.md)
1. [FPNN 服务端高级使用向导](fpnn-server-advanced-tutorial.md)
1. [FPNN 客户端基础使用向导](fpnn-client-basic-tutorial.md)
1. [FPNN 客户端高级使用向导](fpnn-client-advanced-tutorial.md)
1. [FPNN API 索引](fpnn-APIs.md)