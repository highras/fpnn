# FPNN 基础组件库介绍

**本列表仅列出常用模块，不是全部组件清单**  
**注：<fpnn-folder>/base 的部分组件不保证后续会继续存在**

## <fpnn-folder>/base 目录

| 一级分类 | 二级分类 | 三级分类 | 文件 | 说明 |
|---------|---------|---------|-----|------|
| 系统 | 系统用户 | 系统用户 | unix_user.h | 系统用户信息、调整。 |
| 系统 | 系统信息 | CPU | cpu.h | cpu 信息。但推荐直接使用系统API。 |
| 系统 | 信号 | 信号 | ignoreSignals.h | 忽略常见信号。 |
| 系统 | 程序执行 | 命令行参数 | CommandLineUtil.h | 命令行参数提取。 |
| 时间 | 时间获取 | 时间获取 | msec.h | 性能优化的时间获取函数。 |
| 时间 | 时间获取 | 时间获取 | TimeUtil.h | 时间信息、格式化相关。 |
| 时间 | 时间戳 | 时间戳 | rdtsc.h | CPU时间戳。 |
| 时间 | 定时器 | 定时器 | FPTimer.h | 定时器，定时任务。 |
| 文本 & 数字 | Edian | Edian | Endian.h | BOM 识别，Endian 转换 |
| 文本 & 数字 | 字符串处理 | 字符串处理 | StringUtil.h | trim、split、join。 |
| 文本 & 数字 | 十六进制文本 | 十六进制文本 | hex.h | 十六进制文本 & 数字转换。 |
| 文本 & 数字 | 字符集合匹配 | 字符集合匹配 | StringUtil.h | class CharsChecker。 |
| 文本 & 数字 | 字符集合匹配 | 字符集合匹配 | StringUtil.h | class CharMarkMap。 |
| 文件 | 文件读写 | 简化文件读写 | unixfs.h | 简化文件读写。 |
| 文件 | 文件操作 | 文件操作 | FileSystemUtil.h | 文件操作、读写、属性。 |
| 文件 | 目录操作 | 目录操作 | FileSystemUtil.h | 目录创建、获取目录内容。 |
| 配置 | 配置管理 | 配置管理 | Setting.h | 配置管理。 |
| Log | Log | FPNN Log | FPLog.h | FPNN log 接口。 |
| 数据格式 | Json | Json | FPJson.h & FPJson.Enhancement.inc.h | Json 处理。 |
| 数据处理 | 压缩 | 压缩 | gzpipe.h | zip 压缩。 |
| Hash | 算法 | 算法 | HashFunctor.h |  |
| Hash | 算法 | 算法 | hashint.h | 整型哈希。 |
| Hash | 算法 | 算法 | jenkins.h | jenkins 哈希算法。 |
| Hash | 算法 | 算法 | strhash.h | 字符串哈希、内存哈希。 |
| Hash | 容器 | 哈希数组 | HashArray.h | 哈希数组。 |
| Hash | 容器 | 哈希Map | HashMap.h | 哈希Map。 |
| Hash | 容器 | 哈希Set | HashSet.h | 哈希Set。 |
| Hash | 容器 | LRU 哈希 Map | LruHashMap.h | LRU 哈希 Map。 |
| 数据结构 & 数据容器 | 数据容器 | 哈希数组 | HashArray.h | 哈希数组。 |
| 数据结构 & 数据容器 | 数据容器 | 哈希Map | HashMap.h | 哈希Map。 |
| 数据结构 & 数据容器 | 数据容器 | 哈希Set | HashSet.h | 哈希Set。 |
| 数据结构 & 数据容器 | 数据容器 | 堆 | heap.h | c版本堆。 |
| 数据结构 & 数据容器 | 数据容器 | LRU 哈希 Map | LruHashMap.h | LRU 哈希 Map。 |
| 数据结构 & 数据容器 | 数据容器 | 队列 | queue.h | c版队列。 |
| 数据结构 & 数据容器 | 数据容器 | 线程安全队列 | SafeQueue.hpp | 线程安全队列。 |
| 数据结构 & 数据容器 | 数据容器 | 树 | tree.h | c版树结构。 |
| 内存 & 缓存 | 自动释放 | 自动释放 | AutoRelease.h | 离开作用域后，自动 delete 或者 free。 |
| 内存 & 缓存 | 内存池 & 对象池 | 内存池 | IMemoryPool.h | 内存池接口定义。 |
| 内存 & 缓存 | 内存池 & 对象池 | 内存池 | MemoryPool.h | 内存池实现（线程安全，内部带锁）。 |
| 内存 & 缓存 | 内存池 & 对象池 | 内存池 | UnlockedMemoryPool.h | 内存池实现（线程不安全，无锁）。 |
| 内存 & 缓存 | 内存池 & 对象池 | 内存池 | obstack.h | 对象栈。参见头文件内说明。 |
| 内存 & 缓存 | 内存池 & 对象池 | 内存池 | obpool.h | c版本对象/内存池实现。 |
| 内存 & 缓存 | 内存池 & 对象池 | 对象池 | IObjectPool.h | 对象池接口定义。 |
| 内存 & 缓存 | 内存池 & 对象池 | 对象池 | ObjectPool.h | 对象池实现（线程安全，内部带锁）。 |
| 内存 & 缓存 | 内存池 & 对象池 | 对象池 | UnlockedObjectPool.h | 对象池实现（线程不安全，无锁）。 |
| 内存 & 缓存 | 内存链（离散缓存) | 内存链（离散缓存) | IChainBuffer.h | 离散缓存接口定义。支持内存读写、复制、查找、比较、fd句柄读写。 |
| 内存 & 缓存 | 内存链（离散缓存) | 内存链（离散缓存) | ChainBuffer.h | 使用 new/delete 的 IChainBuffer 实现。 |
| 内存 & 缓存 | 内存链（离散缓存) | 内存链（离散缓存) | CachedChainBuffer.h | 使用内存池的 IChainBuffer 实现。 |
| 线程 | 锁 | 读写锁 | RWLocker.hpp | pthread 读写锁封装。 |
| 线程 | 线程池 | 线程池 | ParamTemplateThreadPool.h | 线程池模版。 |
| 线程 | 线程池 | 线程池 | ParamThreadPool.h | 线程池。 |
| 线程 | 线程池 | 线程池 | TaskThreadPool.h | 任务池。 |
| 线程 | 线程池 | 线程池组 | ParamTemplateThreadPoolArray.h | 线程池组模版。 |
| 线程 | 线程池 | 线程池组 | TaskThreadPoolArray.h | 任务池组。 |
| 网络 | 网络操作 | 网络操作 | net.h | 网络操作。 |
| 网络 | 辅助工具 | 地址处理 | NetworkUtility.h | 地址快速转换、endpoint 分解。 |
| 网络 | HTTP | HTTP 代码 | httpcode.h | HTTP 代码。 |
| 网络 | HTTP | HTTP 客户端 | HttpClient.h | HTTP 客户端。 |
| 错误 & 异常 | 异常定义 | 异常定义 | FpnnError.h | FPNN 异常类型定义。 |
| 错误 & 异常 | 错误代码 | 错误代码 | FpnnError.h | FPNN 错误代码定义。 |
| 集群 | 一致性哈希 | 一致性哈希 | carp.h | c版本一致性哈希模块。 |
| 集群 | 一致性哈希 | 一致性哈希 | FunCarpSequence.h | C++版本一致性哈希模块。 |
| 加密 & 校验 | 加密 | AES | rijndael.h | AES 加密。 |
| 加密 & 校验 | 校验 | CRC | crc.h | CRC 校验。 |
| 加密 & 校验 | 校验 | CRC | crc64.h | CRC 64 校验。 |
| 加密 & 校验 | 数字签名 | md5 | md5.h | MD5 签名。 |
| 加密 & 校验 | 数字签名 | sha | sha1.h | sha1 校验。 |
| 加密 & 校验 | 数字签名 | sha | sha256.h | sha256 校验。 |
| 性能 | 性能约束 | 频率约束 | FrequencyLimit.h | 时分秒天的频率限制。 |
| 性能 | 性能分析 | 耗时分析 | TimeAnalyst.h | 耗时分析。 |
| 调试 | 输出 | 输出 | FormattedPrint.h | 打印表格。 |
| 调试 | 输出 | 输出 | PrintMemory.h | 内存十六进制打印。 |


## <fpnn-folder>/proto 目录

| Module | Header file | Introduction |
|--------|-------------|--------------|
| fpnn::FPQuest | FPMessage.h | FPNN 请求包结构 |
| fpnn::FPAnswer | FPMessage.h | FPNN 应答包结构 |
| fpnn::FPQReader | FPReader.h | FPNN 请求包读取类 |
| fpnn::FPAReader | FPReader.h | FPNN 应答包读取类 |
| fpnn::FPQWriter | FPWriter.h | FPNN 请求包生成器 |
| fpnn::FPAWriter | FPWriter.h | FPNN 应答包生成器 |


## <fpnn-folder>/core 目录

| Module | Introduction |
|--------|--------------|
| fpnn::TCPEpollServer | FPNN 服务器。epoll edge trigger 模式 + multi-threads。建议通过配置文件配置。 |
| fpnn::ClientEngine | FPNN 客户端共享的全局引擎。可独立于FPNN 服务器运行。建议通过配置文件配置。 |
| fpnn::TCPClient | FPNN 客户端。该客户端为大量客户端并发链接进行优化。普通情况下，性能略低于良好实现的双线程客户端。 |
| fpnn::IQuestProcessor | FPNN 事件处理基类。适用于 FPNN 服务器，FPNN 客户端，extends 库中的 FPNN 集群代理。 |


## <fpnn-folder>/extends 目录

| 大类 | 二级分类 | 文件 | 模块 | 说明 |
|----|-----|-----|----|----|
| 读取器 | MySQL DBProxy 结果读取器 | DBResultReader.h | DBResultReader | 仅适用于读取 MySQL DBProxy 的返回结果。支持随机读取。 |
| 集群管理 | 集群服务发现客户端 | FPZKCLient.h | fpnn::FPZKClient | 用于服务发现，提交自身信息等集群管理用途。适用于 FPZK 集群发现服务。 |
| 集群管理 | 固定IP，无发现服务集群代理 | TCPBroadcastProxy.hpp | fpnn::TCPBroadcastProxy | 广播代理。 |
| 集群管理 | 固定IP，无发现服务集群代理 | TCPCarpProxy.hpp | fpnn::TCPCarpProxy | 一致性哈希代理。 |
| 集群管理 | 固定IP，无发现服务集群代理 | TCPConsistencyProxy.hpp | fpnn::TCPConsistencyProxy | 强一致性代理。全局强一致性。 |
| 集群管理 | 固定IP，无发现服务集群代理 | TCPRandomProxy.hpp | fpnn::TCPRandomProxy | 随机代理。 |
| 集群管理 | 固定IP，无发现服务集群代理 | TCPRotatoryProxy.hpp | fpnn::TCPRotatoryProxy | 循环代理。 |
| 集群管理 | FPZK集群发现服务集群代理 | TCPFPZKBroadcastProxy.hpp | fpnn::TCPFPZKBroadcastProxy | 广播代理。 |
| 集群管理 | FPZK集群发现服务集群代理 | TCPFPZKCarpProxy.hpp | fpnn::TCPFPZKCarpProxy | 一致性哈希代理。 |
| 集群管理 | FPZK集群发现服务集群代理 | TCPFPZKConsistencyProxy.hpp | fpnn::TCPFPZKConsistencyProxy | 强一致性代理。全局强一致性。 |
| 集群管理 | FPZK集群发现服务集群代理 | TCPFPZKRandomProxy.hpp | fpnn::TCPFPZKRandomProxy | 随机代理。 |
| 集群管理 | FPZK集群发现服务集群代理 | TCPFPZKRotatoryProxy.hpp | fpnn::TCPFPZKRotatoryProxy | 循环代理。 |
| 集群管理 | FPZK集群发现服务集群代理 | TCPFPZKOldestProxy.hpp | fpnn::TCPFPZKOldestProxy | 最早注册/最久运行代理。 |
| 网络访问 | 海量URL异步并发访问引擎 | MultipleURLEngine.h | MultipleURLEngine | 支持 HTTP、HTTPS。理论支持 FTP等 curl 支持的协议。 |
| 消息旁路 | 请求旁路 | Bypass.h | fpnn::Bypass | FPNN 协议旁路 |
| 消息旁路 | 请求旁路 | HttpBypass.h | fpnn::HttpBypass | FPNN 转 HTTP 旁路 |
