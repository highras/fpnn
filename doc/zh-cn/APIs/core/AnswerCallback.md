## AnswerCallback

### 介绍

异步请求的回调对象。

**注意：请勿使用非文档化的 API。**  
非文档化的 API，或为内部使用，或因历史原因遗留，或为将来设计，后续版本均可能存在变动。

### 命名空间

	namespace fpnn;

### 关键定义

	class AnswerCallback
	{
	public:
		AnswerCallback();
		virtual ~AnswerCallback();

		virtual void onAnswer(FPAnswerPtr) = 0;
		virtual void onException(FPAnswerPtr answer, int errorCode) = 0;
	};

**注意**

代码中，`class AnswerCallback` 的基类，和非文档化的接口可能随时会更改。因此，请仅使用本文档中，**明确列出**的接口。

### 成员函数

#### onAnswer

	virtual void onAnswer(FPAnswerPtr) = 0;

当服务器正常返回时，会触发该函数。此时 `FPAnswerPtr` 参数为有效的 FPAnswer 对象。

**注意**：`onAnswer` 和 `onException` **会且仅会**有一个被触发。

#### onException

	virtual void onException(FPAnswerPtr answer, int errorCode) = 0;

当服务器异常返回，或者发生链接中断、关闭，请求超时等事件时，将触发该函数。  
该函数触发时， `int errorCode` 为有效的错误代码。而 `FPAnswerPtr answer` 当且仅当服务器返回异常时，存在。当链接中断、关闭，请求超时等事件发生时，`FPAnswerPtr answer` 为 `nullptr`。

**注意**：`onAnswer` 和 `onException` **会且仅会**有一个被触发。