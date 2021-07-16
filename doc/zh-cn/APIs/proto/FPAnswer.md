## FPAnswer

### 介绍

FPAnswer 为 FPNN 协议的应答数据对象类型。为 [FPMessage](FPMessage.md) 的子类。

**注意：请勿使用非文档化的 API。**  
非文档化的 API，或为内部使用，或因历史原因遗留，或为将来设计，后续版本均可能存在变动。

### 命名空间

	namespace fpnn;

### 关键定义

	class FPAnswer: public FPMessage {
		public:
			enum {
				FP_ST_OK				= 0,  
				FP_ST_ERROR				= 1,  
				FP_ST_HTTP_OK			= 200,  
				FP_ST_HTTP_ERROR		= 500,  
			};
		public:
			//called by FPWriter
			FPAnswer(const FPQuestPtr quest);
			FPAnswer(uint16_t status, const FPQuestPtr quest);

			//create from raw data
			FPAnswer(const Header& hdr, uint32_t seq, const std::string& payload);
			FPAnswer(const char* data, size_t len);
			FPAnswer(const std::string& data);
			virtual ~FPAnswer();

			void setStatus(uint16_t status);
			uint16_t status() const;

			std::string* raw();
			std::string info();
	};

	typedef std::shared_ptr<FPAnswer> FPAnswerPtr;

### 构造函数

	//called by FPWriter
	FPAnswer(const FPQuestPtr quest);
	FPAnswer(uint16_t status, const FPQuestPtr quest);

	//create from raw data
	FPAnswer(const Header& hdr, uint32_t seq, const std::string& payload);
	FPAnswer(const char* data, size_t len);
	FPAnswer(const std::string& data);

建议使用 [FPAWriter](FPWriter.md#FPAWriter) 生成 FPAnswer/FPAnswerPtr 对象。

### 成员函数

#### setStatus

	void setStatus(uint16_t status);

设置应答数据包状态码。

#### status

	uint16_t status() const;

获取应答数据包状态码。

#### raw

	std::string* raw();

将 FPAnswer 对象序列化为 FPNN 协议二进制数据返回。

**返回值**

FPNN 协议二进制封包数据。

**注意**

如果需要释放返回的指针，请用 `delete` 操作。

#### info

	std::string info();

返回可阅读的文本数据。

**注意**：如果 body 数据不是 Json 兼容数据，则返回的可阅读文本中，`body` 显示为空字符串，而不是实际数据。