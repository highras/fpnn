## FPMessage

### 介绍

FPMessage 为 [FPQuest](FPQuest.md) 和 [FPAnswer](FPAnswer.md) 的基类。提供 FPNN 协议数据对象相关的基础操作。

**注意：请勿使用非文档化的 API。**  
非文档化的 API，或为内部使用，或因历史原因遗留，或为将来设计，后续版本均可能存在变动。

### 命名空间

	namespace fpnn;

### 关键定义

#### FP_Pack_Type

FPNN 数据包编码类型枚举。

	class FPMessage {
	public:
		enum FP_Pack_Type {
			FP_PACK_MSGPACK		= 0,
			FP_PACK_JSON		= 1,
		};
	};

#### FPMessageType

FPNN 数据包包类型枚举。

	class FPMessage {
	public:
		enum FPMessageType {
			FP_MT_ONEWAY		= 0, 
			FP_MT_TWOWAY		= 1,
			FP_MT_ANSWER		= 2,
		};
	};

#### FPMessage

	typedef std::map<std::string, std::string> StringMap;

	class FPMessage {
	public:
		uint8_t version() const;
		bool isHTTP() const; 
		bool isTCP() const;
		bool isMsgPack() const;
		bool isJson() const;
		bool isOneWay() const;
		bool isTwoWay() const;
		bool isQuest() const;
		bool isAnswer() const;

		uint8_t methedSize() const;
		uint8_t answerStatus() const;

		uint32_t seqNum() const;
		uint32_t seqNumLE() const;

		const std::string& payload() const;
		std::string json();
		std::string Hex();
		void printHttpInfo();

		int64_t ctime() const;
		void setCTime(int64_t ctime);

		static uint8_t currentVersion();
		static uint8_t supportedVersion();

		const std::string& http_uri(const std::string& u);
		const std::string& http_cookie(const std::string& c);
		const std::string& http_header(const std::string& h);

		StringMap http_uri_all();
		StringMap http_cookie_all();
		StringMap http_header_all();
	};


### 成员函数

#### uint8_t version() const

返回当前数据包所使用的协议版本。

#### bool isHTTP() const

是否是 HTTP/HTTPS 数据。

#### bool isTCP() const

是否是 FPNN 协议数据。

#### bool isMsgPack() const

是否采用 msgPack 编码。

#### bool isJson() const

是否采用 Json 编码。

#### bool isOneWay() const

是否是单向请求(one way quest)。

#### bool isTwoWay() const

是否是双向请求(two way quest)。

#### bool isQuest() const

是否是请求类型(单项请求或双向请求)。

#### bool isAnswer() const

是否是应答类型。

#### uint8_t methedSize() const

对于请求数据，请求的接口名/方法名的长度。  
对于应答，将抛出异常。

#### uint8_t answerStatus() const

对于应答数据，应答数据包的状态码。  
对于请求，将抛出异常。

#### uint32_t seqNum() const

对于双向请求和应答数据，数据包的序列号（本地字节序）。

#### uint32_t seqNumLE() const

对于双向请求和应答数据，数据包的序列号（小端字节序）。

#### const std::string& payload() const

封装的数据内容（可能是二进制格式）。

#### std::string json()

以 Json 格式展现封装的数据。

**注意：**如果数据不是 Json 兼容的，该函数将会返回空字符串。

**Json 兼容：**字典的键必须是字符串类型。除此之外，可能仍存在其他不兼容的类型。

#### std::string Hex()

以大写16进制格式，输出原始数据。

#### void printHttpInfo()

如果是 HTTP/HTTPS 数据，输出 HTTP 头部信息。

#### int64_t ctime() const

获取数据包本地创建时间。  
对于发送者，是创建时间；对于接收者，是收到时间。

#### void setCTime(int64_t ctime)

设置数据包创建时间。

#### static uint8_t currentVersion()

返回当前使用的 FPNN 协议版本。

#### static uint8_t supportedVersion()

返回当前支持的 FPNN 协议版本。

#### const std::string& http_uri(const std::string& u)

查找 HTTP 的 URI 相关字段。

#### const std::string& http_cookie(const std::string& c)

查找 HTTP 的 Cookie 相关字段。

#### const std::string& http_header(const std::string& h)

查找 HTTP 的 header 相关字段。

#### StringMap http_uri_all()

返回 HTTP 的所有 URI 信息。

#### StringMap http_cookie_all()

返回 HTTP 的所有 cookie 信息。

#### StringMap http_header_all()

返回 HTTP 的所有 header 信息。

