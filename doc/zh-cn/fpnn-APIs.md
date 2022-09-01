# FPNN API 索引

FPNN 提供的 API 分成 4 个模块类别：

+ **base**

	base 是基础组件库，提供包含内存、加密、线程、文件系统等各种各样的基础功能。

	base 库的 API 请参见 [base API Index](#base-API-Index)。

+ **proto**

	proto 是协议数据处理模块库，负责 FPNN 协议数据的封包解包，和编解码处理。

	proto 库的 API 请参见 [proto API Index](#proto-API-Index)。

+ **core**

	core 是 FPNN RPC 框架的核心部分，提供了 TCP & UDP 服务器和客户端模块。

	core 库的 API 请参见 [core API Index](#core-API-Index)。

+ **extends**

	extends 是 FPNN RPC 的扩展库，提供了对服务发现和集群访问的支持，以及海量 HTTP 并发访问的支持。  
	extends 和 core 模块，共同组成了 FPNN 微服务框架的核心模块。

	extends 库的 API 请参见 [extends API Index](#extends-API-Index)。


## base API Index

| 类别 | 对象 | 描述 | 头文件 | API 文档 |
|-----|------|-----|-------|---------|
| 时间 | 系列函数 | 时间格式化工具 | [TimeUtil.h](../../base/TimeUtil.h) | [TimeUtil](APIs/base/TimeUtil.md) |
| 时间 | 系列函数 | 高性能时间函数 | [msec.h](../../base/msec.h) | [msec](APIs/base/msec.md) |
| 标识 | 系列函数 | uuid 生成函数 | [uuid.h](../../base/uuid.h) | [uuid](APIs/base/uuid.md) |
| JSON | Json | 便捷的 JSON 库 | [FPJson.h](../../base/FPJson.h) | [FPJson](APIs/base/FPJson.md) |
| 字符串处理 | 系列函数 | 转义字符串 | [escapeString.h](../../base/escapeString.h) | [escapeString](APIs/base/escapeString.md) |
| 字符串处理 | 系列函数和对象 | 字符串处理工具集合 | [StringUtil.h](../../base/StringUtil.h) | [StringUtil](APIs/base/StringUtil.md) |
| 日志 | 系列函数 | 日志接口。日志系统的客户端模块 | [FPLog.h](../../base/FPLog.h) | [FPLog](APIs/base/FPLog.md) |
| 格式化 | 全局函数 | 格式化输出 | [FormattedPrint.h](../../base/FormattedPrint.h) | [FormattedPrint](APIs/base/FormattedPrint.md) |
| 配置 | Setting | 配置信息容器 | [Setting.h](../../base/Setting.h) | [Setting](APIs/base/Setting.md) |
| 异常 | FpnnError | FPNN 异常，及错误代码定义 | [FpnnError.h](../../base/FpnnError.h) | [FpnnError](APIs/base/FpnnError.md) |
| 定时器 | FPTimer | 定时器 | [FPTimer.h](../../base/FPTimer.h) | [FPTimer](APIs/base/FPTimer.md) |
| 数据容器 | LruHashMap | 最近最少使用的 hash 字典容器 | [LruHashMap.h](../../base/LruHashMap.h) | [LruHashMap](APIs/base/LruHashMap.md) |
| 数据容器 | SafeQueue | 线程安全的队列 | [SafeQueue.hpp](../../base/SafeQueue.hpp) | [SafeQueue](APIs/base/SafeQueue.md) |
| 哈希 | 系列函数 | jenkins 哈希算法 | [jenkins.h](../../base/jenkins.h) | [jenkins](APIs/base/jenkins.md) |
| 哈希 | 系列函数 | 字符串哈希 | [strhash.h](../../base/strhash.h) | [strhash](APIs/base/strhash.md) |
| 编码 | 系列函数 | base64 编解码 | [base64.h](../../base/base64.h) | [base64](APIs/base/base64.md) |
| 编码 | 系列函数 | 二进制内容十六进制文本化 | [hex.h](../../base/hex.h) | [hex](APIs/base/hex.md) |
| 摘要算法 | 系列函数 | crc 摘要 | [crc.h](../../base/crc.h) [crc64.h](../../base/crc64.h) | [crc](APIs/base/crc.md) |
| 摘要算法 | 系列函数 | md5 摘要 | [md5.h](../../base/md5.h) | [md5](APIs/base/md5.md) |
| 摘要算法 | 系列函数 | sha1 摘要 | [sha1.h](../../base/sha1.h) | [sha1](APIs/base/sha1.md) |
| 摘要算法 | 系列函数 | sha2 摘要 | [sha256.h](../../base/sha256.h) | [sha256](APIs/base/sha256.md) |
| 加密 | 系列函数 | AES 加密算法 | [rijndael.h](../../base/rijndael.h) | [rijndael](APIs/base/rijndael.md) |
| 压缩 | 系列函数 | gzip 压缩解压 | [gzpipe.h](../../base/gzpipe.h) | [gzpipe](APIs/base/gzpipe.md) |
| 内存 | CachedChainBuffer | 内建内存池和对象池的链式缓存 | [CachedChainBuffer.h](../../base/CachedChainBuffer.h) | [CachedChainBuffer](APIs/base/CachedChainBuffer.md) |
| 内存 | ChainBuffer | 链式缓存 | [ChainBuffer.h](../../base/ChainBuffer.h) | [ChainBuffer](APIs/base/ChainBuffer.md) |
| 内存 | IChainBuffer | 链式缓存接口 | [IChainBuffer.h](../../base/IChainBuffer.h) | [IChainBuffer](APIs/base/IChainBuffer.md) |
| 内存 | IMemoryPool | 定长内存池接口 | [IMemoryPool.h](../../base/IMemoryPool.h) | [IMemoryPool](APIs/base/IMemoryPool.md) |
| 内存 | IObjectPool | 对象池接口 | [IObjectPool.h](../../base/IObjectPool.h) | [IObjectPool](APIs/base/IObjectPool.md) |
| 内存 | MemoryPool | 线程安全的定长内存池 | [MemoryPool.h](../../base/MemoryPool.h) | [MemoryPool](APIs/base/MemoryPool.md) |
| 内存 | ObjectPool | 线程安全的对象池 | [ObjectPool.h](../../base/ObjectPool.h) | [ObjectPool](APIs/base/ObjectPool.md) |
| 内存 | UnlockedMemoryPool | 非线程安全的定长内存池 | [UnlockedMemoryPool.h](../../base/UnlockedMemoryPool.h) | [UnlockedMemoryPool](APIs/base/UnlockedMemoryPool.md) |
| 内存 | UnlockedObjectPool | 非线程安全的对象池 | [UnlockedObjectPool.h](../../base/UnlockedObjectPool.h) | [UnlockedObjectPool](APIs/base/UnlockedObjectPool.md) |
| 资源管理 | 一系列对象 | 离开作用域后，自动释放资源，或执行指定内容 | [AutoRelease.h](../../base/AutoRelease.h) | [AutoRelease](APIs/base/AutoRelease.md) |
| 线程池 | ITaskThreadPool | FPNN 标准线程池接口 | [ITaskThreadPool.h](../../base/ITaskThreadPool.h) | [ITaskThreadPool](APIs/base/ITaskThreadPool.md) |
| 线程池 | ParamTemplateThreadPool | 参数化模板线程池 | [ParamTemplateThreadPool.h](../../base/ParamTemplateThreadPool.h) | [ParamTemplateThreadPool](APIs/base/ParamTemplateThreadPool.md) |
| 线程池 | ParamTemplateThreadPoolArray | 参数化模板线程池阵列 | [ParamTemplateThreadPoolArray.h](../../base/ParamTemplateThreadPoolArray.h) | [ParamTemplateThreadPoolArray](APIs/base/ParamTemplateThreadPoolArray.md) |
| 线程池 | ParamThreadPool | 参数化线程池 | [ParamThreadPool.h](../../base/ParamThreadPool.h) | [ParamThreadPool](APIs/base/ParamThreadPool.md) |
| 线程池 | TaskThreadPool | FPNN 标准线程池 | [TaskThreadPool.h](../../base/TaskThreadPool.h) | [TaskThreadPool](APIs/base/TaskThreadPool.md) |
| 线程池 | TaskThreadPoolArray | FPNN 标准线程池阵列 | [TaskThreadPoolArray.h](../../base/TaskThreadPoolArray.h) | [TaskThreadPoolArray](APIs/base/TaskThreadPoolArray.md) |
| 线程 | 系列对象 | Pthread 读写锁的 C++11 封装 | [RWLocker.hpp](../../base/RWLocker.hpp) | [RWLocker](APIs/base/RWLocker.md) |
| 文件系统 | 系列函数 | 文件系统相关工具函数 | [FileSystemUtil.h](../../base/FileSystemUtil.h) | [FileSystemUtil](APIs/base/FileSystemUtil.md) |
| 文件系统 | 系列函数 | 文件读写相关工局函数 | [unixfs.h](../../base/unixfs.h) | [unixfs](APIs/base/unixfs.md) |
| 系统 | CommandLineUtil | 命令行解析工具 | [CommandLineUtil.h](../../base/CommandLineUtil.h) | [CommandLineUtil](APIs/base/CommandLineUtil.md) |
| 系统 | 全局函数 | 忽略常见的信号中断 | [ignoreSignals.h](../../base/ignoreSignals.h) | [ignoreSignals](APIs/base/ignoreSignals.md) |
| 系统 | 系列函数 | 当前系统状态信息 | [MachineStatus.h](../../base/MachineStatus.h) | [MachineStatus](APIs/base/MachineStatus.md) |
| 系统 | ServerInfo | 当前服务器地址信息 | [ServerInfo.h](../../base/ServerInfo.h) | [ServerInfo](APIs/base/ServerInfo.md) |
| 网络 | Endian | 大小端处理 | [Endian.h](../../base/Endian.h) | [Endian](APIs/base/Endian.md) |
| 网络 | HostLookup | 域名解析 | [HostLookup.h](../../base/HostLookup.h) | [HostLookup](APIs/base/HostLookup.md) |
| 网络 | 全局函数 | HTTP 访问 | [HttpClient.h](../../base/HttpClient.h) | [HttpClient](APIs/base/HttpClient.md) |
| 网络 | 全局函数 | HTTP 状态码含义查询 | [httpcode.h](../../base/httpcode.h) | [httpcode](APIs/base/httpcode.md) |
| 网络 | 全局函数 | 网络相关工具函数 | [NetworkUtility.h](../../base/NetworkUtility.h) | [NetworkUtility](APIs/base/NetworkUtility.md) |
| 调试 | 全局函数 | 打印内存数据 | [PrintMemory.h](../../base/PrintMemory.h) | [PrintMemory](APIs/base/PrintMemory.md) |
| 分析 | 系列对象 | 耗时分析工具 | [TimeAnalyst.h](../../base/TimeAnalyst.h) | [TimeAnalyst](APIs/base/TimeAnalyst.md) |

## proto API Index

| 对象 | 描述 | 头文件 | API 文档 |
|-----|------|------|----------|
| FPMessage | FPNN 请求和应答基类对象 | [FPMessage.h](../../proto/FPMessage.h) | [FPMessage](APIs/proto/FPMessage.md) |
| FPQuest | FPNN 请求对象 | [FPMessage.h](../../proto/FPMessage.h) | [FPQuest](APIs/proto/FPQuest.md) |
| FPAnswer | FPNN 应答对象 | [FPMessage.h](../../proto/FPMessage.h)  | [FPAnswer](APIs/proto/FPAnswer.md) |
| FPQReader | FPNN 请求读取器 | [FPReader.h](../../proto/FPReader.h)  | [FPQReader](APIs/proto/FPReader.md#FPQReader) |
| FPAReader | FPNN 应答读取器 | [FPReader.h](../../proto/FPReader.h)  | [FPAReader](APIs/proto/FPReader.md#FPAReader) |
| FPQWriter | FPNN 请求生成器 | [FPWriter.h](../../proto/FPWriter.h)  | [FPQWriter](APIs/proto/FPWriter.md#FPQWriter) |
| FPAWriter | FPNN 应答生成器 | [FPWriter.h](../../proto/FPWriter.h)  | [FPAWriter](APIs/proto/FPWriter.md#FPAWriter) |


## core API Index

| 对象 | 描述 | 头文件 | API 文档 |
|-----|------|------|----------|
| AnswerCallback | FPNN 异步请求回调对象 | [AnswerCallbacks.h](../../core/AnswerCallbacks.h)  | [AnswerCallback](APIs/core/AnswerCallback.md) |
| Client | FPNN 客户端基类 | [ClientInterface.h](../../core/ClientInterface.h)  | [Client](APIs/core/Client.md) |
| ClientEngine | FPNN 客户端引擎 | [ClientEngine.h](../../core/ClientEngine.h)  | [ClientEngine](APIs/core/ClientEngine.md) |
| ConnectionInfo | FPNN 链接信息对象 | [IQuestProcessor.h](../../core/IQuestProcessor.h)  | [ConnectionInfo](APIs/core/ConnectionInfo.md) |
| IAsyncAnswer | FPNN 异步应答对象 | [IQuestProcessor.h](../../core/IQuestProcessor.h)  | [IAsyncAnswer](APIs/core/IAsyncAnswer.md) |
| IQuestProcessor | FPNN 事件处理基类 | [IQuestProcessor.h](../../core/IQuestProcessor.h)  | [IQuestProcessor](APIs/core/IQuestProcessor.md) |
| QuestSender | FPNN 双工请求发送对象 | [IQuestProcessor.h](../../core/IQuestProcessor.h)  | [QuestSender](APIs/core/QuestSender.md) |
| ServerInterface | FPNN 服务组件标准接口 | [ServerInterface.h](../../core/ServerInterface.h)  | [ServerInterface](APIs/core/ServerInterface.md) |
| TCPClient | FPNN TCP 客户端 | [TCPClient.h](../../core/TCPClient.h)  | [TCPClient](APIs/core/TCPClient.md) |
| TCPEpollServer | FPNN TCP 服务器 | [TCPEpollServer.h](../../core/TCPEpollServer.h)  | [TCPEpollServer](APIs/core/TCPEpollServer.md) |
| UDPClient | FPNN UDP 客户端 | [UDPClient.h](../../core/UDPClient.h)  | [UDPClient](APIs/core/UDPClient.md) |
| UDPEpollServer | FPNN UDP 服务器 | [UDPEpollServer.h](../../core/UDPEpollServer.h)  | [UDPEpollServer](APIs/core/UDPEpollServer.md) |
| IRawDataProcessor | 未包含协议处理模块的原始客户端的数据处理基类 | [RawTransmissionCommon.h](../../core/RawTransmission/RawTransmissionCommon.h)  | [IRawDataProcessor](APIs/core/IRawDataProcessor.md) |
| RawClient | 未包含协议处理模块的原始客户端基类 | [RawClientInterface.h](../../core/RawTransmission/RawClientInterface.h)  | [RawClient](APIs/core/RawClient.md) |
| RawTCPClient | 未包含协议处理模块的原始 TCP 客户端 | [RawTCPClient.h](../../core/RawTransmission/RawTCPClient.h)  | [RawTCPClient](APIs/core/RawTCPClient.md) |
| RawUDPClient | 未包含协议处理模块的原始 UDP 客户端 | [RawUDPClient.h](../../core/RawTransmission/RawUDPClient.h)  | [RawUDPClient](APIs/core/RawUDPClient.md) |


## extends API Index

| 类别 | 对象 | 描述 | 头文件 | API 文档 |
|-----|------|-----|-------|---------|
| 读取器 | DBResultReader | [MySQL DBProxy](https://github.com/highras/dbproxy) 结果读取器 | [DBResultReader.hpp](../../extends/DBResultReader.hpp) | [DBResultReader](APIs/extends/DBResultReader.md) |
| 消息旁路 | Bypass | FPNN 请求旁路组件 | [Bypass.h](../../extends/Bypass.h) | [Bypass](APIs/extends/Bypass.md) |
| 消息旁路 | HttpBypass | FPNN 请求转 HTTP 旁路组件 | [HttpBypass.h](../../extends/HttpBypass.h) | [HttpBypass](APIs/extends/HttpBypass.md) |
| 网络访问 | MultipleURLEngine | 海量URL异步并发访问引擎 | [MultipleURLEngine.h](../../extends/MultipleURLEngine.h) | [MultipleURLEngine](APIs/extends/MultipleURLEngine.md) |
| 服务发现 | FPZKClient | FPZK 服务发现和集群管理客户端 | [FPZKClient.h](../../extends/FPZKClient.h) | [FPZKClient](APIs/extends/FPZKClient.md) |
| 集群访问代理 | TCPProxyCore | TCP 集群访问代理基类 | [TCPProxyCore.hpp](../../extends/TCPProxyCore.hpp) | [TCPProxyCore](APIs/extends/TCPProxyCore.md) |
| 集群访问代理 | TCPFPZKProxyCore | 与 FPZK 服务联动的 TCP 集群访问代理基类 | [TCPFPZKProxyCore.hpp](../../extends/TCPFPZKProxyCore.hpp) | [TCPFPZKProxyCore](APIs/extends/TCPFPZKProxyCore.md) |
| 集群访问代理 | TCPBroadcastProxy | 固定集群 TCP 广播访问代理 | [TCPBroadcastProxy.hpp](../../extends/TCPBroadcastProxy.hpp) | [TCPBroadcastProxy](APIs/extends/TCPBroadcastProxy.md) |
| 集群访问代理 | TCPCarpProxy | 固定集群 TCP 一致性哈希访问代理 | [TCPCarpProxy.hpp](../../extends/TCPCarpProxy.hpp) | [TCPCarpProxy](APIs/extends/TCPCarpProxy.md) |
| 集群访问代理 | TCPConsistencyProxy | 固定集群 TCP 一致性访问代理 | [TCPConsistencyProxy.hpp](../../extends/TCPConsistencyProxy.hpp) | [TCPConsistencyProxy](APIs/extends/TCPConsistencyProxy.md) |
| 集群访问代理 | TCPRandomProxy | 固定集群 TCP 随机访问代理 | [TCPRandomProxy.hpp](../../extends/TCPRandomProxy.hpp) | [TCPRandomProxy](APIs/extends/TCPRandomProxy.md) |
| 集群访问代理 | TCPRotatoryProxy | 固定集群 TCP 轮询访问代理 | [TCPRotatoryProxy.hpp](../../extends/TCPRotatoryProxy.hpp) | [TCPRotatoryProxy](APIs/extends/TCPRotatoryProxy.md) |
| 集群访问代理 | TCPFPZKBroadcastProxy | 与 FPZK 服务联动的 TCP 集群广播访问代理 | [TCPFPZKBroadcastProxy.hpp](../../extends/TCPFPZKBroadcastProxy.hpp) | [TCPFPZKBroadcastProxy](APIs/extends/TCPFPZKBroadcastProxy.md) |
| 集群访问代理 | TCPFPZKCarpProxy | 与 FPZK 服务联动的 TCP 集群一致性哈希访问代理 | [TCPFPZKCarpProxy.hpp](../../extends/TCPFPZKCarpProxy.hpp) | [TCPFPZKCarpProxy](APIs/extends/TCPFPZKCarpProxy.md) |
| 集群访问代理 | TCPFPZKConsistencyProxy | 与 FPZK 服务联动的 TCP 集群一致性访问代理 | [TCPFPZKConsistencyProxy.hpp](../../extends/TCPFPZKConsistencyProxy.hpp) | [TCPFPZKConsistencyProxy](APIs/extends/TCPFPZKConsistencyProxy.md) |
| 集群访问代理 | TCPFPZKOldestProxy | 与 FPZK 服务联动的 TCP 集群最久注册节点访问代理 | [TCPFPZKOldestProxy.hpp](../../extends/TCPFPZKOldestProxy.hpp) | [TCPFPZKOldestProxy](APIs/extends/TCPFPZKOldestProxy.md) |
| 集群访问代理 | TCPFPZKRandomProxy | 与 FPZK 服务联动的 TCP 集群随机访问代理 | [TCPFPZKRandomProxy.hpp](../../extends/TCPFPZKRandomProxy.hpp) | [TCPFPZKRandomProxy](APIs/extends/TCPFPZKRandomProxy.md) |
| 集群访问代理 | TCPFPZKRotatoryProxy | 与 FPZK 服务联动的 TCP 集群轮询访问代理 | [TCPFPZKRotatoryProxy.hpp](../../extends/TCPFPZKRotatoryProxy.hpp) | [TCPFPZKRotatoryProxy](APIs/extends/TCPFPZKRotatoryProxy.md) |
