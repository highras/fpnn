### v.1.3.1

#### 修复

* 满尺寸的UDP包，因为重发默认是组装包格式，所以因剩余空间小2个字节，导致无法重发的问题。


-----------

### v.1.3.0

#### 增加

* 原始 TCP 客户端支持
	+ 接收和处理顺序保证
	+ 无协议处理和序列化，须自行定义
	+ 用于对接其他 RPC 协议
	+ 用于对接原始数据流
* 原始 UDP 客户端支持
	+ 无 ARQ 支持
		+ 无自动分片和自动重组支持
		+ 无顺序重整支持
		+ 无去重支持
		+ 无重传支持
	+ 无协议处理和序列化，须自行定义
	+ 用于对接其他机遇 UDP 的协议

#### 修复

* Base/FileSystemUtil 模块，FileAttrs 结构中，文件尺寸和时间相关字段，改为 64 位长度。


-----------


### v.1.2.2

#### 修复

* WebSocket 分帧支持处理笔误导致分帧支持失效的问题。


-----------


### v.1.2.1

#### 修复

* UDP 四元组重入问题
* MacOS M1 芯片和ARM服务器兼容问题


-----------


### v.1.2.0

#### 增加

* 增加 CUDA 支持和 GPU 监控
* extends/FPZKClient 增加 GPU 信息同步功能
* extends/FPZKClient 增加 自定义数据 同步功能
* 新增 UDP v2 模块
	+ 兼容 UDP v1 协议和处理
	+ 支持加密（整体进行包加密） & 强化加密（数据部分进行流加密）
	+ 支持提前拆解包：普通包、复合包、组装包
	+ 支持重发包合并与包组合（组装包）
	+ 废弃失效/超时的重发包
		* 例外：首包永不被废弃
		* 例外：启用强化加密的数据包不会被废弃
	+ 废弃超时的分片数据：分片数据取消通知

* infos 接口，线程池部分，增加 ArraySize 显示线程池阵列数量

#### 变更

* MachineStatus::getConnectionCount() 变更为 MachineStatus::getInusedSocketCount()
* extends/FPZKClient 将 connCount 及 connNum 拆分为 tcpCount 及 tcpNum 与 udpCount 及 udpNum
* extends/FPZKClient 服务变动通知相关 API 改动，将区分变动的集群和无效的（没有可用服务实例的）集群
* TCP & UDP 服务器模块、ClientEngine 模块的 I/O、worker、duplex 线程池配置修改：
	+ 如果初始线程数量小于 CPU 核心数，则线程池阵列数量为初始线程数量
	+ 如果初始线程数量大于 CPU 核心数，则线程池阵列数量为 CPU 核心数量


#### 修复

* FPTimer 任务过多，唤醒操作会卡死问题


#### 移除

* UDP v1 实现相关代码