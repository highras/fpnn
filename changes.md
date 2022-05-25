### v.1.2.1

#### 修复

* UDP 四元组重入问题


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