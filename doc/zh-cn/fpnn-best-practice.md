# FPNN 最佳实践

## 1. IDL 与 协议格式

1. IDL 框架与 无IDL/IDL-less 框架的选择

FPNN 框架是无 IDL 框架，或者说，是 IDL-less 框架。IDL 框架，和 FPNN 无 IDL 框架在选择和使用上的区别，可参见 “[FPNN 设计理念](fpnn-design.md)”的“无 IDL & IDL-less”一节。

1. 接口协议中的结构体和 tuple	

	请避免在接口描述中，使用结构体，或者 tuple 类型对象。

	因为结构体和 tuple 的成员完全依赖于顺序解释，在内存中等价于混杂类型数组。在顺序变动、类型变动，或者成员增减的时候，无法判断当前结构体或 tuple 对应的正确版本，所以无法有效地做到多版本灰度兼容。因此应该尽量避免在接口协议中直接使用结构体和 tuple 类型，而应以字典对象进行替换，或者加以额外的成员索引及信息进行辅助。

## 2. 网络通讯

1. 二进制协议与文本协议

	二进制协议使用二进制编码，比如 msgPack，ProtoBuf 等。文本协议不含不可视字符，比如 XML，JSON，SOAP 等。文本协议荷载比低于二进制协议，对于大量数据网络传输而言，费效比很低。而且通常情况下，文本协议解析成本远高于二进制协议。所以一般情况下，FPNN 推荐使用 msgPack 编码，使用二进制协议，而非 JSON 编码的半文本协议（JSON 编码的 FPNN 协议数据含有二进制的 FPNN 数据报头），或者纯文本的 HTTP 协议。


1. 单向与请求双向请求

	双向请求需要应答，一般用于需要确认的请求或信息。单向请求不需要应答，因此无法获知对端处理状态，是正常，或者异常。所以单向请求一般用于不重要的通知，而重要的通知一般是用双向请求。


## 3. 服务接口

1. QuestProcessor 的共享与独享

	QuestProcessor 可以在多个对象中共享。比如 TCPEpollServer 和 UDPEpollServer 可以共享同一个 QuestProcessor实例，而多个 TCPClient 或 UDPClient 之间也可以共享同一个 QuestProcessor。Server 和 Client 之间也可以共享同一个 QuestProcessor。QuestProcessor 每一个接口都包含有 [ConnectionInfo](APIs/core/ConnectionInfo.md) 对象，通过该对象，可以知道当前请求或者事件，是属于哪一个链接（[链接全剧唯一ID](APIs/core/ConnectionInfo.md#uniqueId)），是客户端还是服务端链接（[isServerConnection 方法](APIs/core/ConnectionInfo.md#isServerConnection)），是 TCP 还是 UDP 链接（[isTCP](APIs/core/ConnectionInfo.md#isTCP) 与 [isUDP](APIs/core/ConnectionInfo.md#isUDP)），是加密还是未加密链接（[isEncrypted 方法](APIs/core/ConnectionInfo.md#isEncrypted)），是来自内网，还是来自公网（[isPrivateIP 方法](APIs/core/ConnectionInfo.md#isPrivateIP)）。

1. 接口命名

	FPNN 接口名称是二进制安全的。如果仅使用 FPNN 框架，或者 C++ SDK，是支持以二进制数据作为接口名的。但考虑到其他语言的兼容性，建议使用不带符号和特殊字符的 ASCII 字符串作为接口名称。

	此外，FPNN 保留以 `*` 号开头的接口，作为内置接口使用。且同时保留其他以特殊字符开头的接口，以备后用。 

## 4. 错误和异常

1. 使用 FPNN 标准异常应答，简化开发

	FPNN 标准异常应答（ErrorAnswer）包含两个成员，一个是错误代码 code 成员，另外一个是错误描述 ex 成员。

	FPNN 框架与相关 SDK，对异常和错误的应答，均依赖于 FPNN 标准异常应答。

	开发者使用 [errorAnswer](APIs/proto/FPWriter.md#errorAnswer) 接口或 [FpnnErrorAnswer](APIs/proto/FPWriter.md#FpnnErrorAnswer) 宏，均可生成 FPNN 标准异常应答。部分接口，比如 [IAsyncAnswer::sendErrorAnswer](APIs/core/IAsyncAnswer.md#sendErrorAnswer) 默认将生成 FPNN 标准异常应答并进行返回。

1. FPNN 错误代码规范

	FPNN 使用 5 位的数字作为框架的错误代码，并保留 5 位以内数字的代码作为以后使用。

	强烈建议业务的错误代码使用 6 位，或 6 位以上数字。


## 5. 集群管理和负载均衡

1. 服务发现与注册

	服务发现与注册，建议使用 FPNN 技术生态的 FPZK 服务。支持全球组网和服务管理。支持网络分裂和合并，同时支持全球组网的情况下，区域分治，以及集群分组。

1. 负载均衡

	负载均衡建议使用 FPNN 框架 extends 模块自带的 proxy 系列组件。

	使用 proxy 系列组件后，无需再直接使用 TCPClient 或者 FPZKClient，直接可以将 proxy 视为集群/分组的客户端，通过调用 proxy 组件的 sendQuest 发送消息即可。

1. 在线不提供服务

	部分业务可能需要服务实例在线，但暂不对外提供服务。此时对于 FPZK 管理的服务，仅需通过 FPZKClient 将自身 online 状态设置为 false，即可达到服务实例在线，但 proxy 默认不会访问的状态。
