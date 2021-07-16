## IAsyncAnswer

### 介绍

处理服务器请求(Server Push/Duplex)时，对于接收到的双向请求，由 [IQuestProcessor](IQuestProcessor.md) 生成的异步返回器。

**注意：请勿使用非文档化的 API。**  
非文档化的 API，或为内部使用，或因历史原因遗留，或为将来设计，后续版本均可能存在变动。

### 命名空间

	namespace fpnn;

### 关键定义

	class IAsyncAnswer
	{
	public:
		virtual ~IAsyncAnswer() {}

		virtual const FPQuestPtr getQuest();
		virtual bool sendAnswer(FPAnswerPtr);
		virtual bool isSent();

		//-- Extends
		virtual void cacheTraceInfo(const std::string&);
		virtual void cacheTraceInfo(const char *);

		virtual bool sendErrorAnswer(int code = 0, const std::string& ex = "");
		virtual bool sendErrorAnswer(int code = 0, const char* ex = "");
		virtual bool sendEmptyAnswer();
	};

### 构造函数

IAsyncAnswer 的实现被隐藏，因此无法直接创建 IAsyncAnswer 实例，必须通过 [IQuestProcessor](IQuestProcessor.md#genAsyncAnswer) 的 [genAsyncAnswer](IQuestProcessor.md#genAsyncAnswer) 接口生成。

### 成员函数

#### getQuest

	virtual const FPQuestPtr getQuest();

获取服务端发送来的 FPQuest 对象。

#### sendAnswer

	virtual bool sendAnswer(FPAnswerPtr);

向服务端发送应答数据。

**注意**

一个 IAsyncAnswer 实例，只允许发送一次应答。

#### isSent

	virtual bool isSent();

判断是否发送过应答数据。

**注意**

一个 IAsyncAnswer 实例，只允许发送一次应答。

#### cacheTraceInfo

	virtual void cacheTraceInfo(const std::string&);
	virtual void cacheTraceInfo(const char *);

更新 & 缓存当前执行状态的追溯信息。如果相关业务流程无法正常应答，IAsyncAnswer 将在析构时，自动发送错误码为 [FPNN_EC_CORE_UNKNOWN_ERROR](../../fpnn-error-code.md) 异常应答。该异常应答的错误描述，即为当前缓存的执行状态追溯信息。

#### sendErrorAnswer

	virtual bool sendErrorAnswer(int code = 0, const std::string& ex = "");
	virtual bool sendErrorAnswer(int code = 0, const char* ex = "");

向服务器发送异常应答。

**参数说明**

* **`int code`**

	错误代码。

* **`const std::string& ex`** & **`const char* ex`**

	错误描述。

**注意**

一个 IAsyncAnswer 实例，只允许发送一次应答。

#### sendEmptyAnswer

	virtual bool sendEmptyAnswer();

向服务器发送空回应。

**注意**

一个 IAsyncAnswer 实例，只允许发送一次应答。

