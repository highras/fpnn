# FPNN 设计理念

## 1. 原则层面

1. **无 IDL & IDL-less**

	IDL (接口描述语言) 是一个很有用的工具，它提供了对接口的描述，约定了接口协议。使得通讯双方通讯时，无需再发送 scheme，有效提高了通讯数据的荷载比。

	对于非领域内人士，对 IDL 的了解也就到此止步。此时，IDL 是极其有用的工具。在这个层面上而言，FPNN 也是属于 IDL-like 的，毕竟，FPNN 有自己的[接口描述规范(参见“FPNN 协议说明” “四、FPNN 接口协议” 部分)](fpnn-protocol-introduction.md)，虽然还不到“语言”的级别。

	但是，深入邻域内，再进一步，IDL 就不再只是“接口描述语言”这么简单了。目前常见的 RPC 框架，几乎全部会依据 IDL 生成对应的客户端和接口模块，便于开发者快速开发。这样的初衷是好的，但是，问题伴随而来。

	首先，依据 IDL 生成对应的客户端和接口模块。这个本质是编译。而常规对的编译和语言的理解，导致了问题的复杂化：IDL 成了一门“编译型”的语言，有复杂的规则和语法。而且不同 RPC 框架的 IDL 语言不完全一样。每用一个新的框架，就得重新学习一遍。而 FPNN 的接口描述，仅仅只是“规范”，远未达到“语言”的层度。因此学习成本更低，上手更快。

	之后，对于可选参数和可变类型参数，以及不定长参数的支持问题。

	早期的 IDL，所有的接口字段必须存在。即使是无用的，也需要赋予一个诸如 'nil' 一类的值。否则算是违背 IDL 的规范。之后的 IDL，出现了 optional 关键字，但仅仅是不得已的情况下，才被推荐使用。而此时，被强烈推荐使用的却是关键字 require/required。比如 Thrift，ProtoBuf 1.0 都是其中的典型代表。

	直到 RPC 领域的经验积累越来越多，关键字 require/required 才不在被继续推荐，而关键字 optional 成为 IDL 的新宠。

	可选参数，到目前为止还没有遇到什么问题。直到 IDL 的编译，生成对应的客户端和接口代码。

	接口代码的生成，的确方便了网络服务和接口调用的开发。但过度复杂的接口代码，直接导致了接口的强耦合：所有的业务都依赖于 IDL 生成的客户端和服务端的接口，如果一个变动，其余的需要全部跟随变动。如果一个接口被改动，与之关联的所有服务和客户端，必须全部重新编译，否则极有可能在 IDL 生成的代码中，出现不兼容的问题。

	最典型的就是，加了一个新的参数，但不在协议数据的末尾，那绝大多数 RPC 框架原有的接口在处理新版接口数据时，便会出现兼容性错误。

	当然，这在很长一段时间内被视为理所当然。

	但这个问题，对 FPNN 完全不存在。要加就加，已有服务不用修改，一切正常，完全无所谓。

	至于参数的减少和类型的更改，FPNN 仅需换一个获取参数的 API，原有的接口处理函数即可同时处理新旧版本的数据。而传统的 IDL 生成接口代码的 RPC 框架，则往往需要两个独立的接口处理函数，才能同时处理新旧两个版本的数据。

	所以，为了更加优雅的处理参数的删除，和类型的改变，部分 IDL 使用了字段编号这一特性。而字段编号不可重复，一旦确定了，便不能修改。所以字段编号的维护，也成了开发者历史包袱的一部分。

	至于不定类型的参数，支持的 IDL 会选用 Oneof/Union 来实现，而剩下的 IDL 则直接弃疗。

	然后，对于不定长参数，类似于 C 语言的 printf 的参数，这对几乎所有的 IDL 而言，均是噩梦般的存在。因此对于不定长参数的支持，几乎都是以弃疗结束。

	而对于 FPNN，不定类型参数和不定长参数，则完全不是事。至于字段编号，那是什么？简直是完全多余的存在。

	最后，根据 IDL 生成接口代码。

	FPNN 的接口描述，虽然是“规范”级别，不是“语言”级别，但不是不能根据规范编写框架代码的生成工具。而为什么 FPNN 不提供依据接口描述“规范”生成代码的工具呢？

	生成代码是为了简化程序的开发，降低开发者的工作量。因为通常的 RPC IDL 都会生成巨量的代码。而这些代码动则上万行，对程序员而言，是极大的负担。但 FPNN 框架设计之初，就将极力简化使用的复杂度作为核心设计目标之一。所以对于 FPNN 而言，几乎没有什么代码是需要生成的。

	首先，是框架的代码。

	参考 [FPNN 服务端基础使用向导](fpnn-server-basic-tutorial.md)[“1. 服务代码框架”](fpnn-server-basic-tutorial.md#1-服务代码框架)，将发现，无论开发 TCP 服务还是 UDP 服务，算上括号与空行，标准框架一共也才 3 个文件总共 41 行代码。其中 “DemoServer.cpp” 24 行，“DemoServer.cpp” 15 行，“DemoQuestProcessor.cpp” 2 行。而参考 [FPNN 客户端基础使用向导](fpnn-client-basic-tutorial.md) 就会发现，算上括号与空行，客户端完整框架一共就 8 行代码，而以下 17 行代码（算上括号、空行、注释）已经能直接运行，并向服务器发送请求，并接收应答：

		#include <iostream>
		#include "TCPClient.h"
		 
		using namespace std;
		using namespace fpnn;
		 
		int main()
		{
		    std::shared_ptr<TCPClient> client = TCPClient::createClient("demo.example.com", 6789);
		     
		    //-- 生成请求数据
		    FPQWriter qw(1, "echo");
		    qw.param("feedback", "Example string.");
		    FPAnswerPtr answer = client->sendQuest(qw.take());
		    
		    return 0;
		}

	所以，框架几乎没有什么可以生成的代码。

	然后，是接口和对象的处理。

	假设 IDL 中规定了一个接口 demoInterface 和一个结构体 DemoData，编译成 C++ 代码可能是以下形式：

		struct DemoData
		{
			int demoInt;
			double demoDouble;
			std::string demoString;
		};

		void demoInterface(int firstValue, const std::string& secondValue, const struct DemoData& demoData);

	而在 FPNN 中，无论任何接口，接口均统一为以下形式：

		FPAnswerPtr method_name(const FPReaderPtr args, const FPQuestPtr quest, const ConnectionInfo& ci);

	那对于基本类型参数的获取，FPNN 仅只是多了一步：

		int firstValue = args->getInt("firstValue");

	和

		std::string secondValue = args->getString("secondValue");

	而对于结构体，也是仅多一步(但从 FPNN 的理念上，结构体直接做为接口参数的这种设计，因为对结构扩展的兼容性极差，非常不推荐)：

		DemoData demoData = args->get("demoData", Demodata());

	而对于结构体成员的获取：

	使用 IDL 生成的框架代码：

		double value = demoData.demoValue();

	直接使用 FPNN 框架：

		demoData.demoValue;

	没有本质性差异。

	所以，FPNN 虽然可以根据接口描述规范生成对应的框架和接口代码，但几乎毫无意义。所以 FPNN 不会提供根据接口描述规范生成对应的框架和接口代码的工具。

	因此，这也是 FPNN 被视为无 IDL，或者 IDL-less 框架的根本原因。


1. **多线程 vs 多进程**

	多进程实现简单，但过多的进程会导致系统负载过大。而且进程间的通讯，效率要远低于线程间的通讯。

	FPNN 追求的是性能和效率，因此不会考虑多进程模型。


1. **Server Push**

	没有 Server Push，那双工的 TCP 就浪费了一半，也无法有效地实时通知客户方。因此对 Server Push 的支持对 FPNN 框架而言，是一个必选项。

	参考其他能支持 Server Push 的 RPC 框架，大部分的 Server Push 实现过重，将会显著地影响服务器和客户端的性能。在海量的用户和海量的数据交换下，Server Push 实现过重，无异是一种工程灾难。因此 FPNN 决定实现轻巧，但全功能的 Server Push。



1. **提前返回**

	提前返回本质上是一种异步返回。

	在某些需求中，业务可能需要在收到请求后立刻返回。返回之后再继续处理业务。提前返回以同步的外观提供了异步的实现，同时简化了流程操作，无需退出当前的接口函数即可实现异步操作。为了提升开发效率和降低使用难度，提前返回是 FPNN 必须实现的功能需求。



1. **禁止使用 Boost 库**

	Boost 是一个很好的库，而且还是 C++ 的准官方库。C++11/17 标准中的不少内容，就来自于 Boost。但 FPNN 禁用 Boost 的原因其实相对简单：

	+ Boost 不是操作系统或者编译器自带的库。一旦使用了 Boost，那必须要求目标机也安装和部署 Boost。这无疑增加了额外的工作和负担。
	+ Boost 库在某些平台下体积巨大。对于某些小型设备，或者嵌入式来说，体积巨大的 Boost 库无疑是一个工程灾难。
	+ Boost 库太大，不少人并不能完全掌握。一旦使用，将极大的提高开发者和维护者的学习门槛和使用门槛。



1. **禁止使用正则表达式和正则匹配**

	FPNN 追求的是性能和效率，任何开发者不能因自身的懒惰而拖累 FPNN 的性能。


1. **尽量少用第三方库**

	一方面第三方库的的性能未知，另一方面，第三方库在海量压力和极限资源的情况下，稳定性未知。

	因此禁止使用未经严格测试的第三方库。

	而且过多的第三方库会导致过多的依赖，提高 FPNN 的部署、集成和使用的门槛。

	此外，过多的第三方库可能会引入库之间的冲突。无论是第三方库绑定的同一第四方库的版本冲突(感谢某知名图像识别库2011年赐予的宝贵经验)，还是名称空间的命名冲突(在C语言实现的第三方库中更加明显)。


1. **UDP 可靠连接**

	因为上层业务面临弱网环境的链接稳定性，以及需要支持实时音视频和 FPS/MOBA 等高实时性要求类型游戏的需求，所以对 UDP 链接的支持，以及对可靠 UDP 链接的支持，成了 FPNN 框架的必选项。


1. **UDP 可靠数据 vs 不可靠数据**

	对于部分上层业务，混杂有需要保证可靠性的数据，以及可以丢弃的数据。传统方案需要两个通讯链路进行处理。但两个链路较之一个链路，对于开发者，在使用上更加复杂，问题也会更多。而且常见的 RUDP 实现，并不支持可靠的 UDP 数据和不可靠的 UDP 数据在同一虚拟链接上混合发送。为了简化上层业务的开发复杂度，FPNN 框架决定支持可靠/不可丢弃 UDP 数据和不可靠/可丢弃 UDP 数据在同一可靠 UDP 连接上混合发送。



## 2. 实现层面


1. **取消 service 层次**

	某些 RPC 框架，在一个服务进层中，可能会出现一个 service 层次。service 下面的 servant 再提供接口实现。

	好比：

		scheme://host:port/serive_A/interface_A
		scheme://host:port/serive_A/interface_B
		scheme://host:port/serive_A/interface_C

		scheme://host:port/serive_B/interface_A
		scheme://host:port/serive_B/interface_B
		scheme://host:port/serive_B/interface_C

	这种行为，本质上等价于

		scheme://host:port/interface_A_A
		scheme://host:port/interface_A_B
		scheme://host:port/interface_A_C

		scheme://host:port/interface_B_A
		scheme://host:port/interface_B_B
		scheme://host:port/interface_B_C

	将一个方法名分成两部分，在 RPC 层面毫无意义。

	因此 fbThrift 等 RPC 框架，也不支持 service 这一层面。



1. **同一端口多种协议**

	FPNN 同一个端口上可以接收

	+ 未加密的 FPNN 协议
	+ HTTP 协议 1.1 版本
	+ 加密的 FPNN 协议
	+ WebSocket 协议

	或者

	+ 未加密的 FPNN 协议 over SSL/TLS
	+ HTTP 协议 1.1 版本 over SSL/TLS （HTTPS 协议）
	+ WebSocket 协议 over SSL/TLS （wss 协议）

	同一端口支持多种协议的原因很简单：

	运维、配置、使用方均简单。

	注：开发者不应该因自身的懒惰而将复杂性转嫁给使用方。


1. **IPv4、IPv6、TCP/IPv4 over SSL/TLS、TCP/IPv6 over SSL/TLS 四端口监听**

	目前 IPv4 网络还是主流。但业务上可能需要提供 IPv6 支持。因此很多情况下，IPv4 和 IPv6 运行着相同的业务。  
	为了能有效平衡 IPv4 和 IPv6 的业务负载，同时减少对系统资源的消耗，如果选择开启 IPv6 端口，则同一 FPNN 服务进程将进行 IPv4 和 IPv6 双端口监听，而不是启动两个独立进程，分别监听 IPv4 和 IPv6。

	另外，因为 IP 协议栈位于操作系统内核，所以 FPNN 目前不能在同一个端口上同时监听 IPv4 和 IPv6。

	但是，因为 TCP 和 UDP 的协议差异，FPNN 框架可以支持在**同一进程内**，**同一端口**同时监听 TCP 数据和 UDP 数据。

	此外，对于 TCP 及上层协议，FPNN 使用 OpenSSL 提供对 SSL/TLS 的支持。  
	因为 OpenSSL 的接口特性，FPNN 暂时无法在同一端口上同时支持使用 SSL/TLS 和不使用 SSL/TLS 的协议，因此普通 TCP 协议和 TCP over SSL/TLS 将使用不同的端口监听。

	**注意**

	IPv6、TCP/IPv4 over SSL/TLS、TCP/IPv6 over SSL/TLS 均可单独开启。具体请参见 [FPNN 标准配置模版](../conf.template)


1. **不支持 HTTP 1.1 Keep-Alive**

	FPNN 不建议使用 HTTP 协议。FPNN 对 HTTP 的支持，主要是为了兼顾已有的系统，和小众语言。  
	因此 FPNN 仅对 HTTP 进行选择性的支持。其中不包含 Keep-Alive。


1. **不支持 HTTP 1.1 Pipelining**

	Pipelining 要求响应按请求的顺序返回。如果某个后位请求的响应速度高于前位请求，也必须等到前位请求的响应发送后，才能发送该后位请求的响应。

	对于 FPNN 的流水线而言，Pipelining 的支持不仅复杂化 FPNN 的请求响应处理流程，而且 Pipelining 的流水线模型时效远低于 FPNN 自身的流水线模型。  
	因此 FPNN 不支持 HTTP 1.1 Pipelining 流水线处理。


1. **不支持 HTTP Header 中指定的行为**

	因为 FPNN 对 HTTP 的支持，主要是为了兼顾已有的系统，和小众语言，所以不会对 HTTP Header 中的参数做出对应处理。


1. **不支持 HTTP 2.0**

	一方面 FPNN 对 HTTP 的支持主要是出于兼容已有系统和小众语言。高压力高并发模式下强烈不建议采用 HTTP 协议。因此出于性能和效率的考虑，FPNN 也不会支持 HTTP 2.0。


1. **底层链路合并由业务决定，而不是 FPNN 框架**

	如果同一个进程中，有多个 client 实例链接同一个 endpoint，FPNN 不会自动将链接合并。主要原因是基于以下考虑：

	某些长链接业务可能需要用户身份和链接绑定。如果在提供该类服务的代理服务时，势必需要保持各个代理连接的独立性。

	因此 FPNN 不会自动将同一进程中链接同一 endpoint 的多个 client 进行链路合并。

	如果用户期望链路合并，业务层处理起来也非常简单：

		class Holder
		{
			std::mutex _locker;
			std::map<std::string, TCPClientPtr> _linkMap;	//-- map<endpoint, FPNN client>

		public:
			TCPClientPtr getClient(const std::string& endpoint);
			... ... 
		};

	即可。



1. **必须提供聚合IO操作的 client**

	作为服务，FPNN 可能执行抓取或者监控任务；作为客户端，FPNN 可能需要同时上传和下载诸多文件。无论是以上哪类情况，进程中都需要有海量的 client 实例存在。如果每个 client 都是IO双线程的形式实现，那伴随着海量的 client 实例，系统中必然会出现海量的线程。而过多的线程会导致系统的性能急速下滑。  
	FPNN 所以必须提供聚合IO操作的 client，以减少该类需求下，系统中运行的线程数量，以求获得更好的效率和稳定性。


1. **RapidJson 和 FPJson 并存**

	[RapidJson](https://github.com/Tencent/rapidjson) 是目前整体性能最好的 Json 操作库。为了性能，FPNN 选择在内部核心操作部分使用 RapidJson。

	但 [RapidJson](https://github.com/Tencent/rapidjson) 使用过于繁琐复杂，因此对标 Javascript 和 Python 的Json 操作，FPNN 提供了 [FPJson](APIs/base/FPJson.md)。

	任何 Json 操作，无论对象层次深度，[FPJson](APIs/base/FPJson.md) 理论上只需要一行代码即可，具体请参见 [FPJson 文档](APIs/base/FPJson.md)。



## 3. 框架设计

### 1. 服务器端

服务器端的核心为 [TCPEpollServer](APIs/core/TCPEpollServer.md) 和 [UDPEpollServer](APIs/core/UDPEpollServer.md)。

#### [TCPEpollServer](APIs/core/TCPEpollServer.md) 主要结构如下图：

	+------------------------------------------------------------------------------------------------+
	|                                      TCPEpollServer                                            |
	|------------------------------------------------------------------------------------------------|
	| +--------------------------------+  +----------------------+ +-------------------------------+ |
	| |                                |  |                      | |                               | |
	| |   Partitioned Connection Map   |  |I/O Thread Pool Array | |                               | |
	| |                                |  |                      | |  TCP Server Master Processor  | |
	| +--------------------------------+  | +------------------+ | |                               | |
	| +--------------------------------+  | |                  | | |                               | |
	| |                                |  | | I/O Thread Pool  | | +-------------------------------+ |
	| |       Connection Map           |  | |                  | |                                   |
	| |                                |  | +------------------+ | +---------------+ +-------------+ |
	| +--------------------------------+  +----------------------+ |               | |   Answer    | |
	|                                                              | Worker Thread | |  Callback   | |
	| +----------------------------------------------------------+ | Pool Array    | |   Thread    | |
	| |                                                          | |               | |    Pool     | |
	| |                 TCPServerConnection                      | |               | |    Array    | |
	| |                                                          | |+-------------+| |+-----------+| |
	| +----------------------------------------------------------+ ||             || ||  Answer   || |
	| +--------------------------+  +----------------------------+ ||  Worker     || || Callback  || |
	| |                          |  |                            | || Thread Pool || ||  Thread   || |
	| |    Recv Buffer           |  |     Send Buffer            | ||             || ||   Pool    || |
	| |                          |  |                            | |+-------------+| |+-----------+| |
	| +--------------------------+  +----------------------------+ +---------------+ +-------------+ |
	|                                                                                                |
	+------------------------------------------------------------------------------------------------+

+ PartitionedConnectionMap 保存和控制服务器所有的连接。包含数个 ConnectionMap，为 ConnectionMap 的容器，根据 socket 无锁 hash 后，可获得连接具体保存在哪一个 ConnectionMap 中。
+ ConnectionMap 为最终保存和控制服务器连接的容器。为了避免锁共享，每个 ConnectionMap 保存服务器的一部分连接。
+ TCPServerConnection 为服务器连接类，控制数据的收发和解码。每个连接包含一个 Recv Buffer 和一个 Send Buffer。

+ Recv Buffer 控制连接上数据的接收。
+ Send Buffer 控制连接上数据的发送。包含有一个本连接的数据发送队列。

+ I/O Thread Pool Array 为 I/O Thread Pool 的无锁 hash 封装。
+ I/O Thread Pool 提供线程，执行连接数据的收发和解码工作。

+ TCP Server Master Processor 控制解码之后数据包的分发和请求的处理，以及应答的编码和投递。
+ Worker Thread Pool Array 为 worker Thread Pool 的无锁 hash 封装。
+ Worker Thread Pool 提供线程，执行请求的处理，和应答的编码。
+ Answer Callback Thread Pool Array 为 Answer Callback Thread Pool 的无锁 hash 封装。
+ Answer Callback Thread Pool 提供线程，执行服务器收到客户端对服务器发出的请求所对应的应答的回调处理。


#### [UDPEpollServer](APIs/core/UDPEpollServer.md) 主要结构如下图：

	+------------------------------------------------------------------------------------------------+
	|                                      UDPEpollServer                                            |
	|------------------------------------------------------------------------------------------------|
	| +--------------------------------+  +----------------------+ +-------------------------------+ |
	| |                                |  |                      | |                               | |
	| |   Partitioned Connection Map   |  |I/O Thread Pool Array | |                               | |
	| |                                |  |                      | |  UDP Server Master Processor  | |
	| +--------------------------------+  | +------------------+ | |                               | |
	| +--------------------------------+  | |                  | | |                               | |
	| |                                |  | | I/O Thread Pool  | | +-------------------------------+ |
	| |       Connection Map           |  | |                  | |                                   |
	| |                                |  | +------------------+ | +---------------+ +-------------+ |
	| +--------------------------------+  +----------------------+ |               | |   Answer    | |
	|                                                              | Worker Thread | |  Callback   | |
	| +----------------------------------------------------------+ | Pool Array    | |   Thread    | |
	| |                                                          | |               | |    Pool     | |
	| |                 UDPServerConnection                      | |               | |    Array    | |
	| |                                                          | |+-------------+| |+-----------+| |
	| | +------------------------------------------------------+ | ||             || ||  Answer   || |
	| | |                        +--------------------------+  | | ||  Worker     || || Callback  || |
	| | |        UDP IO Buffer   |  UDP ARQ Protocol Parser |  | | || Thread Pool || ||  Thread   || |
	| | |                        +--------------------------+  | | ||             || ||   Pool    || |
	| | +------------------------------------------------------+ | |+-------------+| |+-----------+| |
	| +----------------------------------------------------------+ +---------------+ +-------------+ |
	|                                                                                                |
	+------------------------------------------------------------------------------------------------+


+ PartitionedConnectionMap 保存和控制服务器所有的连接。包含数个 ConnectionMap，为 ConnectionMap 的容器，根据 socket 无锁 hash 后，可获得连接具体保存在哪一个 ConnectionMap 中。
+ ConnectionMap 为最终保存和控制服务器连接的容器。为了避免锁共享，每个 ConnectionMap 保存服务器的一部分连接。
+ UDPServerConnection 为服务器连接类，控制数据的收发和解码。每个连接包含一个 UDPIOBuffer。

+ UDP IO Buffer 控制连接上数据的收发，以及发送数据的切分与合并。包含有一个 UDP ARQ Protocol Parser。
+ UDP ARQ Protocol Parser 控制数据的解析，以及数据切片的组装和合并数据的拆分。

+ I/O Thread Pool Array 为 I/O Thread Pool 的无锁 hash 封装。
+ I/O Thread Pool 提供线程，执行连接数据的收发和解码工作。

+ UDP Server Master Processor 控制解码之后数据包的分发和请求的处理，以及应答的编码和投递。
+ Worker Thread Pool Array 为 worker Thread Pool 的无锁 hash 封装。
+ Worker Thread Pool 提供线程，执行请求的处理，和应答的编码。
+ Answer Callback Thread Pool Array 为 Answer Callback Thread Pool 的无锁 hash 封装。
+ Answer Callback Thread Pool 提供线程，执行服务器收到客户端对服务器发出的请求所对应的应答的回调处理。

### 2. 客户端

客户端分为两类版本：

* 第一类为普通版本，适用于同一进程中只有少量客户端（<10）的情况
	
	该类可用任何兼容FPNN协议的简单客户端逻辑实现。如果实现恰当，性能可高于第二类版本，如果实现不当，性能将会低于第二类版本。

	FPNN 暂不提供该类客户端实现。

* 第二类为大量客户端协作版本，适用于进程中可能存在大量客户端（0～100,000）的情况。（包含不连接的客户端。链接中的客户端受系统最大可用端口数的限制(理论上最大65535)。）

	该类客户端针对大量并存的客户端，整合并优化了客户端的收发逻辑，使众多客户端在各有不同变现的情况下，共享底层的收发处理逻辑，和相关的系统资源。在收发效率不受明显影响的情况下，极大地减少了系统线程的数量和其他资源的开销。

	该类客户端由两部分组成：

	+ [Client](APIs/core/Client.md)
	+ [ClientEngine](APIs/core/ClientEngine.md)

	[Client](APIs/core/Client.md) 包含 [TCPClient](APIs/core/TCPClient.md) 和 [UDPClient](APIs/core/UDPClient.md)，分别为 TCP 客户端和 UDP 客户端的具体表现，为连接开发者和 [ClientEngine](APIs/core/ClientEngine.md) 的桥梁。
	
	[ClientEngine](APIs/core/ClientEngine.md) 为客户端的核心，为全部的 [Client](APIs/core/Client.md) 所共享，执行具体的数据收发、编解码、应答回调执行，服务器Push请求处理等操作。

[ClientEngine](APIs/core/ClientEngine.md) [Client](APIs/core/Client.md) 和 [ClientEngine](APIs/core/ClientEngine.md) 关系如下图：

	+------------------+ +-----------------+ +-----------------+               +-------------------+
	|      Client      | |      Client     | |     Client      |  ... ... ...  |       Client      |
	|                  | |                 | |                 |               |                   |
	+------------------+ +-----------------+ +-----------------+               +-------------------+
	+-----------------------------------------------------------------------------------------------+
	|                                       Client Engine                                           |
	|-----------------------------------------------------------------------------------------------|
	|                                                                                               |
	|  +-----------------------+ +------------------------+ +----------------+ +----------------+   |
	|  |     Partitioned       | |                        | |                | |                |   |
	|  |    Connection Map     | |  I/O Thread Pool Array | |    Worker      | |     Quest      |   |
	|  |                       | |                        | |                | |                |   |
	|  | +-------------------+ | | +--------------------+ | |    Thread      | |     Process    |   |
	|  | |                   | | | |                    | | |                | |                |   |
	|  | |  Connection Map   | | | |  I/O Thread Pool   | | |     Pool       | |     Thread     |   |
	|  | |                   | | | |                    | | |                | |                |   |
	|  | +-------------------+ | | +--------------------+ | |    Array       | |   Pool Array   |   |
	|  +-----------------------+ +------------------------+ |                | |                |   |
	|  +-----------------------+ +------------------------+ | +------------+ | | +------------+ |   |
	|  |                       | |                        | | |            | | | |            | |   |
	|  | TCP Client Connection | | UDP Client Connection  | | |   Worker   | | | |   Quest    | |   |
	|  | +-------------------+ | | +--------------------+ | | |            | | | |            | |   |
	|  | |    Recv Buffer    | | | |   UDP IO Buffer    | | | |   Thread   | | | |  Process   | |   |
	|  | +-------------------+ | | | +----------------+ | | | |            | | | |            | |   |
	|  | +-------------------+ | | | | UDP ARQ Parser | | | | |    Pool    | | | | Thread Pool| |   |
	|  | |    Send Buffer    | | | | +----------------+ | | | |            | | | |            | |   |
	|  | +-------------------+ | | +--------------------+ | | +------------+ | | +------------+ |   |
	|  +-----------------------+ +------------------------+ +----------------+ +----------------+   |
	+-----------------------------------------------------------------------------------------------+

在 [ClientEngine](APIs/core/ClientEngine.md) 中：

+ PartitionedConnectionMap 统一保存和控制客户端的所有的连接。包含数个 ConnectionMap，为 ConnectionMap 的容器，根据 socket 无锁 hash 后，可获得连接具体保存在哪一个 ConnectionMap 中。
+ ConnectionMap 为最终保存和控制客户端连接的容器。为了避免锁共享，每个 ConnectionMap 保存服务器的一部分连接。

+ TCPClientConnection 为客户端连接类，控制数据的收发和解码。每个连接包含一个 Recv Buffer 和一个 Send Buffer。
+ Recv Buffer 控制连接上数据的接收。
+ Send Buffer 控制连接上数据的发送。包含有一个本连接的数据发送队列。

+ UDPServerConnection 为服务器连接类，控制数据的收发和解码。每个连接包含一个 UDPIOBuffer。
+ UDP IO Buffer 控制连接上数据的收发，以及发送数据的切分与合并。包含有一个 UDP ARQ Protocol Parser (UDP ARQ Parser)。
+ UDP ARQ Protocol Parser (UDP ARQ Parser) 控制数据的解析，以及数据切片的组装和合并数据的拆分。

+ Worker Thread Pool Array 为 worker Thread Pool 的无锁 hash 封装。
+ Worker Thread Pool 提供线程，执行应答的解码和发送的请求所对应的应答的回调处理。
+ Quest Process Thread Pool Array 为 Quest Process Thread Pool 的无锁 hash 封装。
+ Quest Process Thread Pool 提供线程，执行客户端收到服务器发出的推送请求的处理。