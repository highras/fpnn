## MachineStatus

### 介绍

机器状态信息获取。

### 命名空间

	namespace fpnn::MachineStatus;

### GPUInfo

GPU 信息。

	struct GPUInfo
	{
		unsigned int index;
		unsigned int usage;

		struct {
			unsigned int usage;
			unsigned long long used;
			unsigned long long total;
		} memory;
	};

**成员说明**

+ `index`

	显卡索引。

+ `usage`

	显卡 GPU 使用率（单位：%）。

+ `memory.usage`

	显卡显存使用率（单位：%）。

+ `memory.used`

	显卡已用显存。单位：字节。

+ `memory.total`

	显卡显存大小。单位：字节。


### 全局函数

#### getCPULoad

	float getCPULoad();

获取本机当前CPU负载。

#### getInusedSocketCount

	int getInusedSocketCount(bool IPv4, bool TCP);

获取本机当前指定类别使用中的 Socket 数目。

#### getNetworkStatus

	void getNetworkStatus(uint64_t& recvBytes, uint64_t& sendBytes);

获取本机所有网卡当前已收发的字节数。

#### getGPUInfo

	bool getGPUInfo(std::list<struct GPUInfo>& infos);

获取显卡信息。

注意：  
1. 如果 [def.mk](../../../../def.mk) 中 `CUDA_SUPPORT=true` 没有启用，则永久返回 `false`。  
2. 首次调用前须先调用 `nvmlInit()` 初始化 CUDA 相关模块。不再调用后，可调用 `nvmlShutdown()` 释放 CUDA 相关资源。