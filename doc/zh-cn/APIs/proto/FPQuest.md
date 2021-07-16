## FPQuest

### 介绍

FPQuest 为 FPNN 协议的请求数据对象类型。为 [FPMessage](FPMessage.md) 的子类。 

**注意：请勿使用非文档化的 API。**  
非文档化的 API，或为内部使用，或因历史原因遗留，或为将来设计，后续版本均可能存在变动。

### 命名空间

	namespace fpnn;

### 关键定义

	class FPQuest: public FPMessage {
	public:
		FPQuest(const std::string& method, bool oneway = false, FP_Pack_Type ptype = FP_PACK_MSGPACK);
		FPQuest(const char* method, bool oneway = false, FP_Pack_Type ptype = FP_PACK_MSGPACK);

		//create from raw data
		FPQuest(const Header& hdr, uint32_t seq, const std::string& method, const std::string& payload);
		FPQuest(const char* data, size_t len);

		//create HTTP, only support json
		FPQuest(const std::string& method, const std::string& payload, StringMap& infos, bool post);

		virtual ~FPQuest();

		void setMethod(const std::string& method);
		void setMethod(const char* method);

		const std::string& method() const;

		std::string* raw();
		std::string info();

	};

	typedef std::shared_ptr<FPQuest> FPQuestPtr;

### 构造函数

	FPQuest(const std::string& method, bool oneway = false, FP_Pack_Type ptype = FP_PACK_MSGPACK);
	FPQuest(const char* method, bool oneway = false, FP_Pack_Type ptype = FP_PACK_MSGPACK);

	//create from raw data
	FPQuest(const Header& hdr, uint32_t seq, const std::string& method, const std::string& payload);
	FPQuest(const char* data, size_t len);

	//create HTTP, only support json
	FPQuest(const std::string& method, const std::string& payload, StringMap& infos, bool post);

建议使用 [FPQWriter](FPWriter.md#FPQWriter) 生成 FPQuest/FPQuestPtr 对象。

### 成员函数

#### setMethod

	void setMethod(const std::string& method);
	void setMethod(const char* method);

* **`const char *method`** & **`const std::string& method`**

设置远程访问/调用的接口名/方法名。

#### method

	const std::string& method() const;

获取远程访问/调用的接口名/方法名。

#### raw
	
	std::string* raw();

将 FPQuest 对象序列化为 FPNN 协议二进制数据返回。

**返回值**

FPNN 协议二进制封包数据。

**注意**

如果需要释放返回的指针，请用 `delete` 操作。

#### info

	std::string info();

返回可阅读的文本数据。

**注意**：如果 body 数据不是 Json 兼容数据，则返回的可阅读文本中，`body` 显示为空字符串，而不是实际数据。