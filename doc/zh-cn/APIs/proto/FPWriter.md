## FPWriter

### 介绍

**注意：请勿使用非文档化的 API。**  
非文档化的 API，或为内部使用，或因历史原因遗留，或为将来设计，后续版本均可能存在变动。

### 命名空间

	namespace fpnn;

### FPWriter

FPWriter 为 FPQWriter 和 FPAWriter 的基类。提供生成 FPNN 协议数据对象的能力。

#### 定义

	class FPWriter {
	public:
		template<typename VALUE>
			void param(const VALUE& v);

		void paramFormat(const char *fmt, ...);
		void paramBinary(const void *data, size_t len);
		void paramNull();
		void paramArray(const char* k, size_t size);
		void paramArray(const std::string& k, size_t size);
		void paramArray(size_t size);
		void paramMap(const char* k, size_t size);
		void paramMap(const std::string& k, size_t size);
		void paramMap(size_t size);

		template<typename VALUE>
			void param(const char *k, const VALUE& v);
		template<typename VALUE>
			void param(const std::string& k, const VALUE& v);

		void paramFormat(const char *k, const char *fmt, ...);
		void paramFormat(const std::string& k, const char *fmt, ...);

		void paramBinary(const char *k, const void *data, size_t len);
		void paramBinary(const std::string& k, const void *data, size_t len);

		void paramNull(const char *k);
		void paramNull(const std::string& k);

		void paramFile(const std::string& k, const std::string& file);
		void paramFile(const char *k, const char *file);

		virtual ~FPWriter() {}

		FPWriter(uint32_t size);
		//only support pack a map
		FPWriter();

		//only support pack raw JSON
		FPWriter(const std::string& json);
		FPWriter(const char* json);

		std::string raw();
		std::string json();
};

#### 构造函数

	FPWriter(uint32_t size);
	FPWriter();
	FPWriter(const std::string& json);
	FPWriter(const char* json);

**参数说明**

* **`uint32_t size`**

	FPMessage 第一层级的数据条目数量(C++版 MsgPack 遗留问题)。

* **`const std::string& json`** & **`const char* json`**

	用于填充 FPMessage 对象数据的 Json 数据。

#### 成员函数

##### param

	template<typename VALUE>
	void param(const VALUE& v);

	template<typename VALUE>
	void param(const char *k, const VALUE& v);

	template<typename VALUE>
	void param(const std::string& k, const VALUE& v);

向 FPMessage 对象写入数据。

**注意**

FPMessage 为字典结构，不带 `k` 参数的重载接口，用于在字典中，向已经用 `paramArray(const char* k, size_t size)` 或 `paramArray(const std::string& k, size_t size)` 初始化完成，但未填充完成的数组，写入数据使用。

**参数说明**

* **`const char *k`** & **`const std::string& k`**

	向字典结构的 FPMessage 对象写入数据时的 key。

* **`const VALUE& v`**

	向 FPMessage 对象写入的数据。

##### paramFormat

	void paramFormat(const char *fmt, ...);
	void paramFormat(const char *k, const char *fmt, ...);
	void paramFormat(const std::string& k, const char *fmt, ...);

向 FPMessage 对象写入格式化后的字符串。

**注意**

FPMessage 为字典结构，不带 `k` 参数的重载接口，用于在字典中，向已经用 `paramArray(const char* k, size_t size)` 或 `paramArray(const std::string& k, size_t size)` 初始化完成，但未填充完成的数组，写入数据使用。

**参数说明**

* **`const char *k`** & **`const std::string& k`**

	向字典结构的 FPMessage 对象写入数据时的 key。

* **`const char *fmt`**

	向 FPMessage 对象写入的字符串的格式串。

* **`...`**

	向 FPMessage 对象写入的字符串的数据。

##### paramBinary

	void paramBinary(const void *data, size_t len);
	void paramBinary(const char *k, const void *data, size_t len);
	void paramBinary(const std::string& k, const void *data, size_t len);

向 FPMessage 对象写入二进制数据。

**注意**

FPMessage 为字典结构，不带 `k` 参数的重载接口，用于在字典中，向已经用 `paramArray(const char* k, size_t size)` 或 `paramArray(const std::string& k, size_t size)` 初始化完成，但未填充完成的数组，写入数据使用。

**参数说明**

* **`const char *k`** & **`const std::string& k`**

	向字典结构的 FPMessage 对象写入数据时的 key。

* **`const void *data`**

	向 FPMessage 对象写入的二进制数据。

* **`size_t len`**

	向 FPMessage 对象写入的二进制数据的长度。

##### paramNull

	void paramNull();
	void paramNull(const char *k);
	void paramNull(const std::string& k);

向 FPMessage 对象写入`null`。

**注意**

FPMessage 为字典结构，不带 `k` 参数的重载接口，用于在字典中，向已经用 `paramArray(const char* k, size_t size)` 或 `paramArray(const std::string& k, size_t size)` 初始化完成，但未填充完成的数组，写入数据使用。

**参数说明**

* **`const char *k`** & **`const std::string& k`**

	向字典结构的 FPMessage 对象写入`null`时的 key。

##### paramArray

	void paramArray(const char* k, size_t size);
	void paramArray(const std::string& k, size_t size);
	void paramArray(size_t size);

在 FPMessage 对象中初始化一个**待填充**的数组。

**注意**

FPMessage 为字典结构，不带 `k` 参数的重载接口，用于在字典中，向已经用 `paramArray(const char* k, size_t size)` 或 `paramArray(const std::string& k, size_t size)` 初始化完成，但未填充完成的数组，写入数据使用。

**参数说明**

* **`const char *k`** & **`const std::string& k`**

	在字典结构的 FPMessage 对象中初始化数组时，数组的 key。

* **`size_t size`**

	数组大小。

	**注意**

	未足数填充，或者超量填充，均会导致 FPMessage 对象数据的错乱。

##### paramMap

	void paramMap(const char* k, size_t size);
	void paramMap(const std::string& k, size_t size);
	void paramMap(size_t size);

在 FPMessage 对象中初始化一个**待填充**的字典。

**注意**

FPMessage 为字典结构，不带 `k` 参数的重载接口，用于在字典中，向已经用 `paramArray(const char* k, size_t size)` 或 `paramArray(const std::string& k, size_t size)` 初始化完成，但未填充完成的数组，写入数据使用。

**参数说明**

* **`const char *k`** & **`const std::string& k`**

	在字典结构的 FPMessage 对象中初始化子级字典时，子级字典的 key。

* **`size_t size`**

	字典大小。

	**注意**

	未足数填充，或者超量填充，均会导致 FPMessage 对象数据的错乱。

##### paramFile

	void paramFile(const std::string& k, const std::string& file);
	void paramFile(const char *k, const char *file);

向 FPMessage 对象写入一个包含文件内容和属性的字典数据。

**注意**

写入的子级字典包含以下字段：

* name

	文件名。

* content
	
	文件内容。

* sign

	文件 md5 签名。

* ext

	文件扩展名。

* size

	文件大小

* atime

	文件访问时间。

* mtime

	文件修改时间。

* ctime

	文件创建时间。

**参数说明**

* **`const char *k`** & **`const std::string& k`**

	向字典结构的 FPMessage 对象写入文件数据字典时的 key。

* **`const char *file`** & **`const std::string& file`**

	文件在系统上的路径名。

##### raw

	std::string raw()

将 FPMessage 对象所含数据，序列化后返回。

##### json

	std::string json()

将 FPMessage 的数据转成 Json 格式返回。

**注意**

如果 FPMessage 的数据无法转成 Json 格式(不兼容)，此时将返回空字符串。


### FPQWriter

FPQWriter 为 [FPWriter](#FPWriter) 的子类。提供生成 [FPNN 协议请求对象（FPQuest）](FPQuest.md) 的能力。

#### 定义

	class FPQWriter : public FPWriter{
	public:
		static FPQuestPtr CloneQuest(const char* method, const FPQuestPtr quest);
		static FPQuestPtr CloneQuest(const std::string& method, const FPQuestPtr quest);

		FPQWriter(size_t size, const char *method, bool oneway=false, FPMessage::FP_Pack_Type ptype=FPMessage::FP_PACK_MSGPACK);

		FPQWriter(size_t size, const std::string& method, bool oneway=false, FPMessage::FP_Pack_Type ptype=FPMessage::FP_PACK_MSGPACK);

		//only support pack a map struct
		FPQWriter(const char *method, bool oneway=false, FPMessage::FP_Pack_Type ptype=FPMessage::FP_PACK_MSGPACK);

		FPQWriter(const std::string& method, bool oneway=false, FPMessage::FP_Pack_Type ptype=FPMessage::FP_PACK_MSGPACK);

		//only support pack raw JSON
		FPQWriter(const std::string& method, const std::string& jsonBody, bool oneway=false, FPMessage::FP_Pack_Type ptype=FPMessage::FP_PACK_MSGPACK);
		FPQWriter(const char* method, const char* jsonBody, bool oneway=false, FPMessage::FP_Pack_Type ptype=FPMessage::FP_PACK_MSGPACK);

		~FPQWriter() { }

		operator FPQuestPtr();

		template<typename VALUE>
			FPQWriter& operator()(const char *key, const VALUE& v);

		template<typename VALUE>
			FPQWriter& operator()(const std::string& key, const VALUE& v);

		template<typename VALUE>
			FPQWriter& operator()(const VALUE& v);

		FPQuestPtr take();

		static FPQuestPtr emptyQuest(const char *method, bool oneway=false, FPMessage::FP_Pack_Type ptype=FPMessage::FP_PACK_MSGPACK);
		static FPQuestPtr emptyQuest(const std::string& method, bool oneway=false, FPMessage::FP_Pack_Type ptype=FPMessage::FP_PACK_MSGPACK);
};

#### 构造函数

	FPQWriter(size_t size, const char *method, bool oneway=false, FPMessage::FP_Pack_Type ptype=FPMessage::FP_PACK_MSGPACK);

	FPQWriter(size_t size, const std::string& method, bool oneway=false, FPMessage::FP_Pack_Type ptype=FPMessage::FP_PACK_MSGPACK);

	//only support pack a map struct
	FPQWriter(const char *method, bool oneway=false, FPMessage::FP_Pack_Type ptype=FPMessage::FP_PACK_MSGPACK);

	FPQWriter(const std::string& method, bool oneway=false, FPMessage::FP_Pack_Type ptype=FPMessage::FP_PACK_MSGPACK);

	//only support pack raw JSON
	FPQWriter(const std::string& method, const std::string& jsonBody, bool oneway=false, FPMessage::FP_Pack_Type ptype=FPMessage::FP_PACK_MSGPACK);
	FPQWriter(const char* method, const char* jsonBody, bool oneway=false, FPMessage::FP_Pack_Type ptype=FPMessage::FP_PACK_MSGPACK);

**参数说明**

* **`size_t size`**

	FPQuest 对象内字典，所含条目数量。

	**注意**

	未足数填充，或者超量填充，均会导致 FPQuest 对象数据的错乱。

* **`const char *method`** & **`const std::string& method`**

	FPQuest 请求对象，远程访问/调用的接口名/方法名。

* **`bool oneway`**

	是否为单向消息，还是双向消息。

	**注意**

	双向消息要求对方必须发送应答(FPAnswer)作为回应，单项消息不需要对方发送应答作为回应。

* **`FPMessage::FP_Pack_Type ptype`**

	FPQuest 对象内数据的编码类型。分为 MsgPack 和 Json 两类。  
	具体可参见 [FPMessage](FPMessage.md) 相关定义。

* **`const char *jsonBody`** & **`const std::string& jsonBody`**

	填充 FPQuest 对象数据的 Json 数据。

#### 成员函数

##### CloneQuest

	static FPQuestPtr CloneQuest(const char* method, const FPQuestPtr quest);
	static FPQuestPtr CloneQuest(const std::string& method, const FPQuestPtr quest);

用 `method` 指定的接口名/方法名，拷贝 `const FPQuestPtr quest` 指定的对象数据，生成新的请求对象。

**参数说明**

* **`const char *method`** & **`const std::string& method`**

	FPQuest 请求对象，远程访问/调用的接口名/方法名。

* **`const FPQuestPtr quest`**

	需要拷贝数据的 FPQuest 对象。

##### operator FPQuestPtr()

	operator FPQuestPtr();

将已填充完毕数据的 FPQWriter 对象转换为 FPQuestPtr 对象。

##### FPQWriter& operator()

	template<typename VALUE>
		FPQWriter& operator()(const char *key, const VALUE& v);

	template<typename VALUE>
		FPQWriter& operator()(const std::string& key, const VALUE& v);

	template<typename VALUE>
		FPQWriter& operator()(const VALUE& v);

写入请求数据的便捷方法。等同于调用父类 [FPWriter](#FPWriter) 的 [param](#param) 方法。

**注意**

FPQuest 为字典结构，不带 `key` 参数的重载接口，用于在字典中，向已经用父类 [FPWriter](#FPWriter) `paramArray(const char* k, size_t size)` 或 `paramArray(const std::string& k, size_t size)` 方法初始化完成，但未填充完成的数组，写入数据使用。

**参数说明**

* **`const char *key`** & **`const std::string& key`**

	向字典结构的 FPQuest 对象写入数据时的 key。

* **`const VALUE& v`**

	向 FPQuest 对象写入的数据。

##### take

	FPQuestPtr take();

生成 FPQuestPtr 对象。

##### emptyQuest

	static FPQuestPtr emptyQuest(const char *method, bool oneway=false, FPMessage::FP_Pack_Type ptype=FPMessage::FP_PACK_MSGPACK);
	static FPQuestPtr emptyQuest(const std::string& method, bool oneway=false, FPMessage::FP_Pack_Type ptype=FPMessage::FP_PACK_MSGPACK);

生成无须填充数据的 FPQuestPtr 对象。

**参数说明**

* **`const char *method`** & **`const std::string& method`**

	FPQuest 请求对象，远程访问/调用的接口名/方法名。

* **`bool oneway`**

	是否为单向消息，还是双向消息。

	**注意**

	双向消息要求对方必须发送应答(FPAnswer)作为回应，单项消息不需要对方发送应答作为回应。

* **`FPMessage::FP_Pack_Type ptype`**

	FPQuest 对象内数据的编码类型。分为 MsgPack 和 Json 两类。  
	具体可参见 [FPMessage](FPMessage.md) 相关定义。


### FPAWriter

FPAWriter 为 [FPWriter](#FPWriter) 的子类。提供生成 FPNN 协议应答对象（Answer）的能力。

#### 定义

	class FPAWriter : public FPWriter{
	public:
		static FPAnswerPtr CloneAnswer(const FPAnswerPtr answer, const FPQuestPtr quest);
		static FPAnswerPtr CloneAnswer(const std::string& payload, const FPQuestPtr quest);

		FPAWriter(size_t size, const FPQuestPtr quest);
		//only support pack a map
		FPAWriter(const FPQuestPtr quest);
		//only support pack raw JSON
		FPAWriter(const char* jsonBody, const FPQuestPtr quest);
		FPAWriter(const std::string& jsonBody, const FPQuestPtr quest);

		~FPAWriter() { }

		operator FPAnswerPtr();

		template<typename Value>
			FPAWriter& operator()(const char *key, const Value& v);
		template<typename Value>
			FPAWriter& operator()(const std::string& key, const Value& v);
		template<typename Value>
			FPAWriter& operator()(const Value& v);

		FPAnswerPtr take();

		static FPAnswerPtr errorAnswer(const FPQuestPtr quest, int32_t code = 0, const std::string& ex = "", const std::string& raiser = "");
		static FPAnswerPtr errorAnswer(const FPQuestPtr quest, int32_t code = 0, const char* ex = "", const char* raiser = "");
		static FPAnswerPtr emptyAnswer(const FPQuestPtr quest);

		FPAWriter(size_t size, uint16_t status, const FPQuestPtr quest);
	};

	#define FpnnErrorAnswer(quest, code, ex)
}

#### 构造函数

	FPAWriter(size_t size, const FPQuestPtr quest);
	//only support pack a map
	FPAWriter(const FPQuestPtr quest);
	//only support pack raw JSON
	FPAWriter(const char* jsonBody, const FPQuestPtr quest);
	FPAWriter(const std::string& jsonBody, const FPQuestPtr quest);
	FPAWriter(size_t size, uint16_t status, const FPQuestPtr quest);

**参数说明**

* **`size_t size`**

	FPAnswer 对象内字典，所含条目数量。

	**注意**

	未足数填充，或者超量填充，均会导致 FPAnswer 对象数据的错乱。

* **`const FPQuestPtr quest`**

	FPAnswer 需要回应的 FPQuest 对象。

* **`const char* jsonBody`** & **`const std::string& jsonBody`**

	填充 FPAnswer 对象数据的 JSON 数据。

* **`uint16_t status`**

	指定的 FPAnswer 回应数据的状态码。  
	一般情况下，该状态码无需特殊指定。


#### 成员函数

##### CloneAnswer

	static FPAnswerPtr CloneAnswer(const FPAnswerPtr answer, const FPQuestPtr quest);
	static FPAnswerPtr CloneAnswer(const std::string& payload, const FPQuestPtr quest);

拷贝 `const FPAnswerPtr answer` 指定的对象数据，生成回应 `const FPQuestPtr quest` 的请求对象；  
或用 `const std::string& payload` 指定的对象数据，生成回应 `const FPQuestPtr quest` 的请求对象。  

* **`const FPAnswerPtr answer`**

	需要拷贝数据的 FPAnswer 对象。

* **`const std::string& payload`**

	需要用于填充 FPAnswer 的 Json 数据。

* **`const FPQuestPtr quest`**

	需要回应的 FPQuest 对象。

##### operator FPAnswerPtr()

	operator FPAnswerPtr();

将已填充完毕数据的 FPAWriter 对象转换为 FPAnswerPtr 对象。


##### FPAWriter& operator()

	template<typename Value>
		FPAWriter& operator()(const char *key, const Value& v);
	template<typename Value>
		FPAWriter& operator()(const std::string& key, const Value& v);
	template<typename Value>
		FPAWriter& operator()(const Value& v);

写入应答数据的便捷方法。等同于调用父类 [FPWriter](#FPWriter) 的 [param](#param) 方法。

**注意**

FPAnswer 为字典结构，不带 `key` 参数的重载接口，用于在字典中，向已经用父类 [FPWriter](#FPWriter) `paramArray(const char* k, size_t size)` 或 `paramArray(const std::string& k, size_t size)` 方法初始化完成，但未填充完成的数组，写入数据使用。

**参数说明**

* **`const char *key`** & **`const std::string& key`**

	向字典结构的 FPAnswer 对象写入数据时的 key。

* **`const VALUE& v`**

	向 FPAnswer 对象写入的数据。

##### take

	FPAnswerPtr take();

生成 FPAnswerPtr 对象。

##### errorAnswer

	static FPAnswerPtr errorAnswer(const FPQuestPtr quest, int32_t code = 0, const std::string& ex = "", const std::string& raiser = "");
	static FPAnswerPtr errorAnswer(const FPQuestPtr quest, int32_t code = 0, const char* ex = "", const char* raiser = "");

生成异常应答。

**参数说明**

* **`const FPQuestPtr quest`**

	需要回应的 FPQuest 对象。

* **`int32_t code`**

	错误代码。对应 FPAnswer 对象的 `code` 字段。

* **`const std::string& ex`** & **`const char* ex`**

	错误描述。对应 FPAnswer 对象的 `ex` 字段。

* **`const std::string& raiser`** & **`const char* raiser`**

	引发错误的对象/模块。（**注意**：`raiser` 不再建议使用。后续版本会逐步取消该参数。）

##### emptyAnswer

	static FPAnswerPtr emptyAnswer(const FPQuestPtr quest);

生成无须填充数据的 FPAnswerPtr 对象。

**参数说明**

* **`const FPQuestPtr quest`**

	需要回应的 FPQuest 对象。

##### FpnnErrorAnswer

	#define FpnnErrorAnswer(quest, code, ex)

生成异常应答的便捷方法。与 [errorAnswer](#errorAnswer) 等价。


