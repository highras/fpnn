# FPNN 内置接口

### status

	=> *status {}
	<= { status:%b }

运行状态确认接口。

业务如须修改该接口行为，可重载 [IQuestProcessor](APIs/core/IQuestProcessor.md) 的 [status](APIs/core/IQuestProcessor.md#status) 方法即可。

TCP 与 UDP 服务均内置该接口。

**注意：该接口仅能内网访问。**

### tune

	=> *tune { key:%s, value:%s }
	<= {}

动态修改框架和业务参数。

+ 框架参数

	框架可修改的参数请参见 [FPNN 框架可动态调节参数列表](fpnn-tune-items.md)。

+ 业务参数

	业务如需通过该接口动态调整参数，可重载 [IQuestProcessor](APIs/core/IQuestProcessor.md) 的 [tune](APIs/core/IQuestProcessor.md#tune) 方法即可。

TCP 与 UDP 服务均内置该接口。

**注意**

+ 该接口仅能内网访问；
+ 对于 TCP 服务实例，一旦启用加密，则无论是全局加密，还是部分接口加密，还是可选加密，框架内置的 tune 接口将强制要求加密访问。

### infos

	=> *infos {}
	<= {...}   //-- 取决于各服务器自定义的状态信息

查询框架及业务详细信息。

业务信息默认为空 JSON 对象。须业务重载 [IQuestProcessor](APIs/core/IQuestProcessor.md) 的 [infos](APIs/core/IQuestProcessor.md#infos) 方法，返回业务信息的 JSON 描述，infos 接口才可返回业务的详细信息。

TCP 与 UDP 服务均内置该接口。

**注意：该接口仅能内网访问。**


### key

	=> *key { publicKey:%B, ?streamMode:%b, ?bits:%d }
	<= {}  //-- answer is encrypted.

ECC/ECDH 方式交换秘钥。  
可以公网访问。必须是链接建立后的第一个命令，否则无效。

**注意：仅 TCP 服务支持该接口。**

**参数说明**

+ **publicKey**

	客户端公钥。

+ **streamMode**

	true：使用流加密模式；  
	false：使用包加密模式。   
	默认为 false。

	流加密 & 包加密 均采用 AES CFB 模式。

+ **bits**

	秘钥长度，仅 128 和 256 可选。默认 128。

**IV 生成规则：**

md5(secret)。

**秘钥生成规则：**

+ 128 bits：

	secret 前 16 bytes

+ 256 bits：

	如果 secret 长 32 bytes，则为 secret，否则为 sha256(secret)。


**包加密模式：**

会先以 **小端字节序** 发送一个 uint32_t 类型的长度，指明后续的整个加密包的长度。

### ping

	=> *ping {}
	<= { ts:%d }      //-- ts: 服务端，毫秒级 UTC 时间戳。

TCP 链接保活接口，可以公网访问。

**注意：仅 TCP 服务支持该接口。**