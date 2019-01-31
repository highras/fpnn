# FPNN Server Basic Tutorial

## 1. 服务代码框架

假设将要开发的服务名为 **Demo**，因此，假设服务程序名为 **DemoServer**，服务的核心类名称为 **DemoQuestProcessor**。

服务器框架一般会被分为三个文件：

+ DemoServer.cpp
+ DemoQuestProcessor.h
+ DemoQuestProcessor.cpp

如果服务很简单，也可以将三个文件合并为一个文件。

文件如下：

DemoQuestProcessor.h

	#ifndef Demo_Processor_h
	#define Demo_Processor_h
	 
	#include "IQuestProcessor.h"
	 
	using namespace fpnn;
	 
	class DemoQuestProcessor: public IQuestProcessor
	{
	    QuestProcessorClassPrivateFields(DemoQuestProcessor)
	public:
	    QuestProcessorClassBasicPublicFuncs
	};
	 
	#endif


DemoQuestProcessor.cpp

	#include "DemoQuestProcessor.h"
	using namespace fpnn;


DemoServer.cpp

	#include <iostream>
	#include "TCPEpollServer.h"
	#include "DemoQuestProcessor.h"
	#include "Setting.h"
	 
	int main(int argc, char* argv[])
	{
	    if (argc != 2)
	    {
	        std::cout<<"Usage: "<<argv[0]<<" config"<<std::endl;
	        return 0;
	    }
	    if(!Setting::load(argv[1])){
	        std::cout<<"Config file error:"<< argv[1]<<std::endl;
	        return 1;
	    }
	 
	    ServerPtr server = TCPEpollServer::create();
	    server->setQuestProcessor(std::make_shared<DemoQuestProcessor>());
	    if (server->startup())
	        server->run();
	 
	    return 0;
	}

此时，以上已经是一个可以运行的FPNN 服务的所有代码。

**一般实际开发中，如果没有特殊需求，服务的三个框架文件直接拷贝使用以上代码，替换 DemoQuestProcessor 为实际服务对应的名称即可。**

从现在开始，本教程不再修改 DemoServer.cpp 中的代码，仅对 DemoQuestProcessor.h & DemoQuestProcessor.cpp 进行修改。 


## 2. 添加接口方法

1. 基本概念

	在开始添加服务接口前，需要明白几个基本概念：

	+ FPQuestPtr & FPAnswerPtr

		FPQuestPtr 为服务请求数据的 shared_ptr 指针。  
		FPAnswerPtr 为服务应答数据的 shared_ptr 指针。  

		FPQuestPtr 所指向的对象中包含有客户端向服务器发送的所有请求信息。  
		包含：

		+ 请求的方法名
		+ 方法参数
		+ 请求序号
		+ 链接性质（TCP or HTTP）
		+ 方法参数的编码协议（msgPack or Json）

		一般仅需要关心**方法参数**即可。

		FPAnswerPtr 所指向的对象中包含有服务器向客户端发送的所有应答信息。  
		包含：

		+ 应答状态(正常、异常)
		+ 对应的请求序号
		+ 回复的应答数据

		FPQuest & FPAnswer 请参见：<fpnn-folder>/proto/FPMessage.h。


	+ FPReaderPtr

		FPReaderPtr 为FPNN 数据提取器的 shared_ptr 指针。  
		通过对应的 FPReader 或 FPQReader 或 FPAReader 可以读取／提取 对应的 FPQuest 或 FPAnswer 中包含的全部数据。

		FPReader & FPQReader & FPAReader 请参见：<fpnn-folder>/proto/FPReader.h。


	+ twoway & oneway

		请求一般分为两种类型，一种是一问一答。  
		即一个请求，一个回应。这种类型称为 twoway。  
		另外一种只有请求，没有应答。换句话说，只有通知，没有响应。这种类型称为 oneway。


	+ ConnectionInfo

		ConnectionInfo 包含了当前的链接状态。  
		包含：

		+ 客户端 ip（IPv4 or IPv6）
		+ 客户端 port
		+ 连接是否加密
		+ 是否来自于IPv4内网／局域网
		+ 是否是 WebSocket

		ConnectionInfo 请参见：<fpnn-folder>/core/IQuestProcessor.h。


1. 添加接口

	1. 接口的格式

		FPNN 无需 IDL 文件，因此，所有的接口格式均统一为

			FPAnswerPtr method_name(const FPReaderPtr args, const FPQuestPtr quest, const ConnectionInfo& ci)

		用实际接口对应的函数名称替换  method_name 即可。


		假设 Demo 服务有两个接口，一个为 echo，是 twoway 类型；一个为 notify，是 oneway 类型。  
		修改后的 DemoQuestProcessor.h & DemoQuestProcessor.cpp 如下：

		DemoQuestProcessor.h

			#ifndef Demo_Processor_h
			#define Demo_Processor_h
			 
			#include "IQuestProcessor.h"
			 
			using namespace fpnn;
			 
			class DemoQuestProcessor: public IQuestProcessor
			{
			    QuestProcessorClassPrivateFields(DemoQuestProcessor)
			public:
			    FPAnswerPtr echo(const FPReaderPtr args, const FPQuestPtr quest, const ConnectionInfo& ci);
			    FPAnswerPtr notify(const FPReaderPtr args, const FPQuestPtr quest, const ConnectionInfo& ci);
			    QuestProcessorClassBasicPublicFuncs
			};
			 
			#endif


		DemoQuestProcessor.cpp

			#include "DemoQuestProcessor.h"
			 
			using namespace fpnn;
			 
			FPAnswerPtr DemoQuestProcessor::echo(const FPReaderPtr args, const FPQuestPtr quest, const ConnectionInfo& ci)
			{
			    //-- need to add some codes
			}
			 
			FPAnswerPtr DemoQuestProcessor::notify(const FPReaderPtr args, const FPQuestPtr quest, const ConnectionInfo& ci)
			{
			    //-- need to add some codes
			}



	1. 请求参数的获取

		请求参数的获取，一般使用 FPReaderPtr->wantXXX() 或 FPReaderPtr->getXXX()。

		比如，要获取一个名为 paramInt 的整型参数(int64_t, int32_t, int, int16_t, int8_t, ...)，可以使用

			int a = args->wantInt("paramInt")
			//-- or
			int a = args->getInt("paramInt")

		对于字符串，假设参数名为 paramString

			std::string str = args->wantString("paramString")
			//-- or
			std::string str = args->getString("paramString")

		对于数组，假设参数名为 IntArray 和 StringArray

			std::vector<int> arr = args->want("IntArray", std::vector<int>())
			std::vector<std::string> arr = args->want("StringArray", std::vector<std::string>())
			//-- or
			std::vector<int> arr = args->get("IntArray", std::vector<int>())
			std::vector<std::string> arr = args->get("StringArray", std::vector<std::string>())

		更多更详细的接口，请参见：<fpnn-folder>/proto/FPReader.h。  
		更多实例请参见 <fpnn-folder>/proto/test/。

		至于使用 want 还是 get，取决于要获取的参数，是否是可选参数。  
		want 系列方法，要求参数必须存在，且类型匹配。否则会抛出异常。  
		如果业务没有处理该异常，则 FPNN 框架将自动捕获处理。对于 twoway 类型的请求，FPNN 框架还将自动返回参数缺失，或者参数类型错误的异常状态应答。

		对于 get 系列方法，对应于可选参数。不要求参数必须存在。如果目标参数不存在，则返回默认值。  
		默认值可以指定，比如

			int a = args->getInt("paramInt", 100)
			std::string str = args->getString("paramString", "default")

		get 系列接口的默认值如下：

		| 类型 | 默认值 |
		|--|--|
		| 整型 | 0 |
		| 浮点型 | 0.0 |
		| bool | false |
		| 字符串 | "" |

		具体请参见：<fpnn-folder>/proto/FPReader.h。  
		更多实例请参见 <fpnn-folder>/proto/test/。

		假设 echo 接口有一个字符类型必选参数，叫做 feedback；notify 有两个参数，一个字符类型必选参数：type，一个浮点型可选参数：value。  
		则修改后的 DemoQuestProcessor.cpp 如下：

			#include "DemoQuestProcessor.h"
			 
			using namespace fpnn;
			 
			FPAnswerPtr DemoQuestProcessor::echo(const FPReaderPtr args, const FPQuestPtr quest, const ConnectionInfo& ci)
			{
			    std::string feedback = args->wantString("feedback");
			    std::cout<<"Echo interface received: "<<feedback<<std::endl;
			 
			    //-- need to add some codes
			}
			 
			FPAnswerPtr DemoQuestProcessor::notify(const FPReaderPtr args, const FPQuestPtr quest, const ConnectionInfo& ci)
			{
			    std::string type = args->wantString("type");
			    double value = args->getDouble("value");
			 
			    std::cout<<"Notify interface received: type: "<<type<<", value: "<<value<<std::endl;
			    //-- need to add some codes
			}			


	1. 接口返回

		对于 oneway 类型的请求，因为不用回应，所以接口直接返回 nullptr 即可。  
		对于 twoway 类型的请求，如果没有采用 “提前返回”(参见高级部分) 或者 “异步返回”(参见高级部分)，则须返回一个有效的 FPAnswerPtr。  
		该 FPAnswerPtr 指向的 FPAnswer 对象，包含了需要返回给客户端的应答。

		假设 echo 接口，将 feedback 原样返回，返回的参数名为 got，则修改后的 DemoQuestProcessor.cpp 如下：

			#include "DemoQuestProcessor.h"
			 
			using namespace fpnn;
			 
			FPAnswerPtr DemoQuestProcessor::echo(const FPReaderPtr args, const FPQuestPtr quest, const ConnectionInfo& ci)
			{
			    std::string feedback = args->wantString("feedback");
			    std::cout<<"Echo interface received: "<<feedback<<std::endl;
			 
			    FPAWriter aw(1, quest);
			    aw.param("got", feedback);
			    return aw.take();
			}
			 
			FPAnswerPtr DemoQuestProcessor::notify(const FPReaderPtr args, const FPQuestPtr quest, const ConnectionInfo& ci)
			{
			    std::string type = args->wantString("type");
			    double value = args->getDouble("value");
			 
			    std::cout<<"Notify interface received: type: "<<type<<", value: "<<value<<std::endl;
			    return nullptr;
			}


		其中：

		+ FPAWriter 是 FPAnswer 的生成器。需要将最表层的参数个数，和收到的 FPQuestPtr 对象作为参数传入。
		+ aw.param() 函数将自动识别参数类型。
		+ take() 操作，将会自动生成 FPAnswerPtr 对象。
		+ 一个 FPAWriter 对象，只能 take() 一次。

		具体请参见：<fpnn-folder>/proto/FPWriter.h。  
		更多实例请参见 <fpnn-folder>/proto/test/。


	1. 接口注册

		接口已经编写完毕，但如果不进行接口注册，则外界仍然无法访问。  
		接口注册一般是在 IQuestProcessor 子类实例的构造函数注册。  
		接口注册函数为：

			/*
			    attributes:
			        EncryptOnly: 只有加密链接可以调用该接口
			        PrivateIPOnly: 只有内网地址可以调用该接口（IPv4 内网地址，或者IPv4/IPv6 本地环路地址）
			*/
			inline void registerMethod(const std::string& method_name, MethodFunc func, uint32_t attributes = 0);

		注册的接口名可以和函数名不同，只要是合法字符串即可。可以包含空格。但不得以 * 号开头。

		“*” 号开头的接口，保留为**框架内置接口**。目前可用的内置接口请参考 [FPNN 内置接口](fpnn-build-in-methods.md)。

		一般情况下，为了便于维护，注册的接口名建议与接口函数名保持一致。

		修改后的 DemoQuestProcessor.h 如下：

			#ifndef Demo_Processor_h
			#define Demo_Processor_h
			 
			#include "IQuestProcessor.h"
			 
			using namespace fpnn;
			 
			class DemoQuestProcessor: public IQuestProcessor
			{
			    QuestProcessorClassPrivateFields(DemoQuestProcessor)
			public:
			    FPAnswerPtr echo(const FPReaderPtr args, const FPQuestPtr quest, const ConnectionInfo& ci);
			    FPAnswerPtr notify(const FPReaderPtr args, const FPQuestPtr quest, const ConnectionInfo& ci);
			 
			    DemoQuestProcessor()
			    {
			        registerMethod("echo", &DemoQuestProcessor::echo);
			        registerMethod("notify", &DemoQuestProcessor::notify);
			    }
			    QuestProcessorClassBasicPublicFuncs
			};
			 
			#endif


## 3. 编辑配置文件

必须的配置文件就只有一行

	FPNN.server.listening.port = 6789 

该行指定了服务器运行所需监听的 IPv4 端口。

完整的配置文件模版请参见 [FPNN 标准配置模版](../conf.template)。

假设 demo 服务的配置文件名为 demo.conf，则基本配置如下：

	FPNN.server.listening.ip = 
	FPNN.server.listening.port = 6789
	FPNN.server.name = DemoServer
	 
	FPNN.server.log.level = DEBUG
	FPNN.server.log.endpoint = std::cout
	FPNN.server.log.route = DemoServer

`FPNN.server.log.endpoint` 指定了日志输出的形式。  
std::cout 代表了日志将直接输出至终端屏幕。  
如果同一台机上还运行着 logAgent，则可配置日志输出到 logAgent。  
参照 logAgent 的配置文件配置 `FPNN.server.log.endpoint` 即可。  
logAgent 采用默认配置时，`FPNN.server.log.endpoint` 可配置为

	FPNN.server.log.endpoint = unix:///tmp/fplog.sock

如果日志配置为输出到 logAgent，还需找运维人员，在 logAgent & logServer 中配置对应的 `FPNN.server.log.route`。


## 4. 编译

建立文件夹 DemoServer，并将 DemoServer.cpp、DemoQuestProcessor.h、DemoQuestProcessor.cpp 拷贝至 DemoServer 文件夹下。  
假设 FPNN-Release 的目录为 infra-fpnn-release-path，创建文件 Makefile，内容如下：

	EXES_SERVER = DemoServer
	FPNN_DIR = infra-fpnn-release-path
	 
	CFLAGS +=
	CXXFLAGS +=
	CPPFLAGS += -I$(FPNN_DIR)/extends -I$(FPNN_DIR)/core -I$(FPNN_DIR)/proto -I$(FPNN_DIR)/base -I$(FPNN_DIR)/proto/msgpack -I$(FPNN_DIR)/proto/rapidjson
	LIBS += -L$(FPNN_DIR)/extends -L$(FPNN_DIR)/core -L$(FPNN_DIR)/proto -L$(FPNN_DIR)/base -lextends -lfpnn
	 
	OBJS_SERVER = DemoServer.o DemoQuestProcessor.o
	 
	all: $(EXES_SERVER)
	 
	clean:
	    $(RM) *.o $(EXES_SERVER)
	 
	include $(FPNN_DIR)/def.mk


然后执行 make 编译即可。

**实际项目中，将 Makefile 中的 DemoServer、DemoServer.o、DemoQuestProcessor.o 替换为对应的实际名称即可。**


## 5. 运行

执行 `./DemoServer demo.conf` 即可。

测试 echo 接口：  
运行 FPNN cmd  命令：`./cmd localhost 6789 echo '{"feedback":"Hello, FPNN!"}'`

测试 notify 接口：  
运行 FPNN cmd  命令：`./cmd localhost 6789 notify '{"type":"normal", "value":2.3}' -oneway`

FPNN cmd 命令，请参考 [FPNN 内置工具](fpnn-tools.md) 相关条目。