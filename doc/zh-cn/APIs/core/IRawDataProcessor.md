## IRawDataProcessor

### 介绍

[RawClient](RawClient.md) 处理接收到的数据的对象的基类。

IRawDataProcessor 仅处理 [RawClient](RawClient.md) 接收到的数据，链接事件由 [IQuestProcessor](IQuestProcessor.md) 负责处理。

**注意：请勿使用非文档化的 API。**  
非文档化的 API，或为内部使用，或因历史原因遗留，或为将来设计，后续版本均可能存在变动。

### 命名空间

	namespace fpnn;

### ReceivedRawData

接收到的原始数据/原始数据片段。

	struct ReceivedRawData
	{
		uint8_t* data;
		uint32_t len;

		ReceivedRawData(uint32_t maxBufferLength);
		~ReceivedRawData();
	};


**注意**

`data` 字段已自动分配和释放，无需人工操作。


### IRawDataProcessor

处理接收到的数据的对象基类。同时也是数据批量处理对象 IRawDataBatchProcessor 和数据批量接管对象 IRawDataChargeProcessor 的基类。

	class IRawDataProcessor
	{
	public:
		virtual void process(ConnectionInfoPtr connectionInfo, uint8_t* buffer, uint32_t len);
		virtual ~IRawDataProcessor() {}
	};
	typedef std::shared_ptr<IRawDataProcessor> IRawDataProcessorPtr;

#### process

	virtual void process(ConnectionInfoPtr connectionInfo, uint8_t* buffer, uint32_t len);

接收到的数据的处理函数。

**参数说明**

* **`ConnectionInfoPtr connectionInfo`**

	连接信息对象 [ConnectionInfo](ConnectionInfo.md)。

* **`uint8_t* buffer`**

	接收到的数据的存储地址。相关内存由框架释放，使用者无需处理。

* **`uint32_t len`**

	接收到的数据长度。


### IRawDataBatchProcessor

批量处理接收到的数据的对象基类。同时也是数据处理对象 IRawDataProcessor 的子类。

	class IRawDataBatchProcessor: public IRawDataProcessor
	{
	public:
		virtual void process(ConnectionInfoPtr connectionInfo, const std::list<ReceivedRawData*>& dataList);	//-- DO NOT delete all ReceivedRawData* pointers.
		virtual ~IRawDataBatchProcessor() {}
	};

#### process

	virtual void process(ConnectionInfoPtr connectionInfo, const std::list<ReceivedRawData*>& dataList);

接收到的数据的处理函数。

**参数说明**

* **`ConnectionInfoPtr connectionInfo`**

	连接信息对象 [ConnectionInfo](ConnectionInfo.md)。

* **`const std::list<ReceivedRawData*>& dataList`**

	接收到的数据。**注意**：`ReceivedRawData*` 指向的内存由框架管理，使用者无需释放。


### IRawDataChargeProcessor

批量接管接收到的数据缓冲对象的对象基类。同时也是数据处理对象 IRawDataProcessor 的子类。

	class IRawDataChargeProcessor: public IRawDataProcessor
	{
	public:
		virtual void process(ConnectionInfoPtr connectionInfo, std::list<ReceivedRawData*>& dataList);	//-- MUST delete all ReceivedRawData* pointers.
		virtual ~IRawDataChargeProcessor() {}
	};

#### process

	virtual void process(ConnectionInfoPtr connectionInfo, std::list<ReceivedRawData*>& dataList);

接收到的数据的处理函数。

**参数说明**

* **`ConnectionInfoPtr connectionInfo`**

	连接信息对象 [ConnectionInfo](ConnectionInfo.md)。

* **`std::list<ReceivedRawData*>& dataList`**

	接收到的数据。**注意**：`ReceivedRawData*` 指向的内存对象的所有权在此移交给使用者，使用者需要使用 delete 释放每一个 `ReceivedRawData*` 指向的内存对象。
