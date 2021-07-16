## Bypass

### 介绍

FPNN 数据旁路。

**注意**

+ 本模块必须由配置文件驱动。

	配置项目可参见 [Bypass.h](../../../../extends/Bypass.h) 的头部注释。

	配置条目的格式为：

		Bypass.<tagName>.FPZK.cluster.<proxyType>.list = <list>

	+ 其中 `tagName` 为 Bypass 对象初始化时传入的标记字符串。使用不同标记字符串初始化的 Bypass 对象，将读取对应的配置条目，执行对性的旁路行为。

	+ `proxyType` 为旁路模式。目前可选值为：一致性哈希 `carp`， 最久运行 `oldest`， 随机 `random`，广播 `broadcast` 四种。

	+ 配置项的值为需要旁路到的服务名称列表，或者 `服务名称@集群分组` 的服务分组名称列表。

+ 因使用极少，本模块考虑在未来的版本中取消。

### 命名空间

	namespace fpnn;

### 关键定义

	class Bypass
	{
	public:
		Bypass(FPZKClientPtr fpzkClient, const char* tagName);
		void bypass(int64_t hintId, const FPQuestPtr quest);
	};
	typedef std::shared_ptr<Bypass> BypassPtr;


### 构造函数

	Bypass(FPZKClientPtr fpzkClient, const char* tagName);

**参数说明**

* **`FPZKClientPtr fpzkClient`**

	FPZKClient 实例指针。FPZKClient 请参考 [FPZKClient](FPZKClient.md)。

* **`const char* tagName`**

	配置文件中，配置项名称的 `tagName` 参数。Bypass 根据不同的 `tagName` 读取对应的配置信息，并进行初始化。实际旁路时，执行 `tagName` 对应配置项指定的旁路行为。

### 成员函数

#### bypass

	void bypass(int64_t hintId, const FPQuestPtr quest);

旁路指定的请求数据。

具体的旁路行为由配置文件指定。

当旁路失败时，会重试一次。如果两次均失败，相关信息将会以 `ERROR` 级别，输出到日志中。日志请参考：[FPLog](../base/FPLog.md)。

**参数说明**

* **`int64_t hintId`**

	执行一致性哈希的旁路时，需要使用的  hintId。其他形式的旁路，将忽略该参数。

* **`const FPQuestPtr quest`**

	需要旁路的请求。