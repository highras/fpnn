# FPNN 客户端基础使用向导

[TOC]

## 1. 客户端代码框架

代码框架包含一个 cpp 文件和一个 Makefie 文件。

假设要开发的客户端名字为 DemoClient，建立文件夹 DemoClient。  
假设要访问的目标服务器为 `demo.example.com`，端口为 6789。  
新建 cpp 文件 DemoClient.cpp 如下：

	#include "TCPClient.h"
	 
	using namespace fpnn;
	int main()
	{
	    std::shared_ptr<TCPClient> client = TCPClient::createClient("demo.example.com", 6789);
	    return 0;
	}

如果目标服务器是 **UDP** 协议，请将将 DemoClient.cpp 中的 `TCPClient` 文本直接替换为 `UDPClient` 即可。本文后续所有操作，对 TCPClient 和 UDPClient 均适用。

假设 FPNN 的目录为 infra-fpnn-path，新建文件 Makefile 如下：

	EXES_CLIENT = DemoClient
	FPNN_DIR = infra-fpnn-path
	 
	CFLAGS +=
	CXXFLAGS +=
	CPPFLAGS += -I$(FPNN_DIR)/extends -I$(FPNN_DIR)/core -I$(FPNN_DIR)/proto -I$(FPNN_DIR)/base -I$(FPNN_DIR)/proto/msgpack -I$(FPNN_DIR)/proto/rapidjson
	LIBS += -L$(FPNN_DIR)/extends -L$(FPNN_DIR)/core -L$(FPNN_DIR)/proto -L$(FPNN_DIR)/base -lextends -lfpnn
	 
	OBJS_CLIENT = DemoClient.o
	 
	all: $(EXES_CLIENT)
	clean:
	    $(RM) *.o $(EXES_CLIENT)
	 
	include $(FPNN_DIR)/def.mk



## 2. 发送请求

发送请求有三个标准接口

	/**
	    All SendQuest():
	        If return false, caller must free quest & callback.
	        If return true, don't free quest & callback.
	    timeout in seconds.
	*/
	FPAnswerPtr sendQuest(FPQuestPtr quest, int timeout = 0);
	bool sendQuest(FPQuestPtr quest, AnswerCallback* callback, int timeout = 0);
	bool sendQuest(FPQuestPtr quest, std::function<void (FPAnswerPtr answer, int errorCode)> task, int timeout = 0);

第一个是同步接口，其余两个是异步发送接口。具体可参见 [sendQuest](APIs/core/Client.md#sendQuest)。

假设要访问的接口为 twoway 类型，接口名称为 `echo`，参数为一个字符串类型，参数名为 `feedback`。  
以第一个同步接口为例，修改 DemoClient.cpp 代码如下：

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

其中：

+ [FPQWriter][] 是 [FPQuest][] 的生成器。[FPQuest][] 包含一个字典作为核心数据结构。[FPQWriter][] 需要将 [FPQuest][] 须要包含字典的键值对个数，和要访问的接口名称作为参数传入。
+ [FPQWriter][] 使用 [param](APIs/proto/FPWriter.md#param) 系列接口添加参数。作为基础的 [param](APIs/proto/FPWriter.md#param) 接口，将自动识别参数类型。
+ [FPQWriter][] 使用 [take](APIs/proto/FPWriter.md#take) 方法生成 [FPQuestPtr][FPQuest] 对象。
+ 一个 [FPQWriter][] 对象，只能调用 [take](APIs/proto/FPWriter.md#take) 方法一次。

具体请参见：[FPWriter](APIs/proto/FPWriter.md)。  
更多实例请参见 [/proto/test/](../../proto/test/)。



## 3. 解析请求结果

假设上文访问的 `echo` 接口有一个字符串类型的返回参数，参数名为 `got`，则可以用 [FPAReader][] 获取参数。
修改 DemoClient.cpp 代码如下：

	#include <iostream>
	#include "TCPClient.h"
	 
	using namespace std;
	using namespace fpnn;
	 
	int main()
	{
	    std::shared_ptr<TCPClient> client = TCPClient::createClient("demo.example.com", 6789);
	     
	    FPQWriter qw(1, "echo");
	    qw.param("feedback", "Example string.");
	    FPAnswerPtr answer = client->sendQuest(qw.take());
	 
	    try{
	        FPAReader ar(answer);
	        std::string got = ar.wantString("got");
	        cout<<"Server return data: "<<got<<endl;
	    }
	    catch (const FpnnError& ex)
	    {
	        cout<<"Fpnn error. code: "<<ex.code()<<", msg: "<<ex.what()<<endl;
	    } 
	    catch (...)
	    {
	        cout<<"error occurred when reading answer."<<endl;
	    }
	    return 0;
	}

[FPAReader][] 需要用有效的 [FPAnswerPtr][FPAnswer] 对象初始化。然后就可以用 [FPAReader][] 的 get 和 want 系列接口，通过参数名获取对应的参数。

比如，要获取一个名为 `paramInt` 的整型参数(int64_t, int32_t, int, int16_t, int8_t, ...)，可以使用

	int a = ar.wantInt("paramInt");
	//-- or
	int a = ar.getInt("paramInt");

对于字符串，假设参数名为 `paramString`

	std::string str = ar.wantString("paramString");
	//-- or
	std::string str = ar.getString("paramString");

对于数组，假设参数名为 `IntArray` 和 `StringArray`

	//-- want 系列
	std::vector<int> arr = ar.want("IntArray", std::vector<int>())
	std::vector<std::string> arr = ar.want("IntArray", std::vector<std::string>())
	
	//-- get 系列
	std::vector<int> arr = ar.get("IntArray", std::vector<int>())
	std::vector<std::string> arr = ar.get("IntArray", std::vector<std::string>())

更多更详细的接口，请参见 [FPReader][]。  
更多实例请参见 [/proto/test/](../../proto/test/)。

**want 系列接口与 get 系列接口的区别**

使用 want 还是 get，取决于要获取的参数，是否是可选参数。  
want 系列方法，要求参数必须存在，且类型匹配。否则会抛出异常。  
对于客户端，需要开发者捕获并处理 want 系列接口抛出的异常。

get 系列方法，不要求参数必须存在。如果目标参数不存在，则返回默认值。  
默认值可以指定，比如：

	int a = ar.getInt("paramInt", 100);
	std::string str = ar.getString("paramString", "default");

get 系列接口的默认值如下：

| 类型 | 默认值 |
|--|--|
| 整型 | 0 |
| 浮点型 | 0.0 |
| bool | false |
| 字符串 | "" |


具体请参见 [FPReader][]。  
更多实例请参见 [/proto/test/](../../proto/test/)。

以上即为客户端全部完整代码。


[FPQuest]: APIs/proto/FPQuest.md
[FPAnswer]: APIs/proto/FPAnswer.md
[FPQWriter]: APIs/proto/FPWriter.md#FPQWriter
[FPReader]: APIs/proto/FPReader.md#FPReader
[FPAReader]: APIs/proto/FPReader.md#FPAReader
