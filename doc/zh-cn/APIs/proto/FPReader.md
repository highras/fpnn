## FPReader

### 介绍

**注意：请勿使用非文档化的 API。**  
非文档化的 API，或为内部使用，或因历史原因遗留，或为将来设计，后续版本均可能存在变动。

### 命名空间

	namespace fpnn;

### FPReader

FPReader 为 FPQReader 和 FPAReader 的基类。提供随机读取 FPNN 协议数据对象的能力。

#### 定义

	class FPReader{
	public:
		const msgpack::object& getObject(const char *k);
		const msgpack::object& getObject(const std::string& k);
		const msgpack::object& wantObject(const char *k);
		const msgpack::object& wantObject(const std::string& k);

		bool existKey(const char *k);
		bool existKey(const std::string& k);

		//the following function will not throw exception
		template<typename TYPE>
			TYPE get(const char *k, TYPE dft);

		template<typename TYPE>
			TYPE get(const std::string& k, TYPE dft);

		intmax_t getInt(const char* k, intmax_t dft = _intDef);
		intmax_t getInt(const std::string& k, intmax_t dft = _intDef);
		uintmax_t getUInt(const char* k, uintmax_t dft = _uintDef);
		uintmax_t getUInt(const std::string& k, uintmax_t dft = _uintDef);
		double getDouble(const char* k, double dft = _doubleDef);
		double getDouble(const std::string& k, double dft = _doubleDef);
		bool getBool(const char* k, bool dft = _boolDef);
		bool getBool(const std::string& k, bool dft = _boolDef);
		std::string getString(const char* k, std::string dft = _stringDef);
		std::string getString(const std::string& k, std::string dft = _stringDef);
		bool getFile(const char* k, FileSystemUtil::FileAttrs& attrs);
		bool getFile(const std::string& k, FileSystemUtil::FileAttrs& attrs);

		// The following function will throw exceptions
		template<typename TYPE>
			TYPE want(const char *k, TYPE dft);

		template<typename TYPE>
			TYPE want(const std::string& k, TYPE dft);

		intmax_t wantInt(const char* k, intmax_t dft = _intDef);
		intmax_t wantInt(const std::string& k, intmax_t dft = _intDef);
		uintmax_t wantUInt(const char* k, uintmax_t dft = _uintDef);
		uintmax_t wantUInt(const std::string& k, uintmax_t dft = _uintDef);
		double wantDouble(const char* k, double dft = _doubleDef);
		double wantDouble(const std::string& k, double dft = _doubleDef);
		bool wantBool(const char* k, bool dft = _boolDef);
		bool wantBool(const std::string& k, bool dft = _boolDef);
		std::string wantString(const char* k, std::string dft = _stringDef);
		std::string wantString(const std::string& k, std::string dft = _stringDef);
		bool wantFile(const char* k, FileSystemUtil::FileAttrs& attrs);
		bool wantFile(const std::string& k, FileSystemUtil::FileAttrs& attrs);

		template<typename TYPE>
			TYPE convert(TYPE& dst);

		msgpack::type::object_type getObjectType(const char* k);
		msgpack::type::object_type getObjectType(const std::string& k);
		//get the type of key, if key not exist, return false
		bool isInt(const char* k);
		bool isInt(const std::string& k);
		bool isBool(const char* k);
		bool isBool(const std::string& k);
		bool isDouble(const char* k);
		bool isDouble(const std::string& k);
		bool isString(const char* k);
		bool isString(const std::string& k);
		bool isBinary(const char* k);
		bool isBinary(const std::string& k);
		bool isArray(const char* k);
		bool isArray(const std::string& k);
		bool isMap(const char* k);
		bool isMap(const std::string& k);
		bool isExt(const char* k);
		bool isExt(const std::string& k);
		
		bool isNil(const char* k);
		bool isNil(const std::string& k);

		FPReader(const std::string& payload);
		FPReader(const char* buf, size_t len);
		FPReader(const msgpack::object& obj);
		virtual ~FPReader() {}

		std::string json();
	};

	typedef std::shared_ptr<FPReader> FPReaderPtr;

#### 构造函数

	FPReader(const std::string& payload);
	FPReader(const char* buf, size_t len);
	FPReader(const msgpack::object& obj);

**参数说明**

* **`const std::string& payload`**

	需要读取的，经过 MsgPack 编码的 FPNN 协议 body 数据。

* **`const char* buf`**

	需要读取的，经过 MsgPack 编码的 FPNN 协议 body 数据。

* **`size_t len`**

	需要读取的 FPNN 协议 body 数据的长度。

* **`const msgpack::object& obj`**

	需要读取的，经过 MsgPack 编码的 FPNN 协议 body 数据。

#### 成员函数

##### getObject

	const msgpack::object& getObject(const char *k);
	const msgpack::object& getObject(const std::string& k);

获取 FPMessage 对象数据字典中，`k` 指定键值对应的 MsgPack 编码的数据。  
如果对应键值不存在，将返回 MsgPack 空对象。

**参数说明**

* **`const char *k`** & **`const std::string& k`**

	需要获取的数据的键值。

**返回值**

	MsgPack 编码的二进制数据。

##### wantObject

	const msgpack::object& wantObject(const char *k);
	const msgpack::object& wantObject(const std::string& k);

获取 FPMessage 对象数据字典中，`k` 指定键值对应的 MsgPack 编码的数据。  
如果对应键值不存在，将抛出 [FPNN 异常][FPNNError]。

**参数说明**

* **`const char *k`** & **`const std::string& k`**

	需要获取的数据的键值。

**返回值**

	MsgPack 编码的二进制数据。

##### existKey

	bool existKey(const char *k);
	bool existKey(const std::string& k);

检查 FPMessage 对象数据字典中，`k` 指定键值对是否存在。

**参数说明**

* **`const char *k`** & **`const std::string& k`**

	需要检查的键值对。

##### get

	template<typename TYPE>
		TYPE get(const char *k, TYPE dft);

	template<typename TYPE>
		TYPE get(const std::string& k, TYPE dft);

按模板参数类型获取 FPMessage 对象数据字典中，`k` 指定键值的数据。  
如果对应键值不存在，将返回参数 `TYPE dft` 的值作为替代。

**参数说明**

* **`const char *k`** & **`const std::string& k`**

	需要获取的数据的键值。

* **`TYPE dft`**

	如果指定的键值不存在时，作为替代，返回的数据。

##### getInt

	intmax_t getInt(const char* k, intmax_t dft = _intDef);
	intmax_t getInt(const std::string& k, intmax_t dft = _intDef);

如果 FPMessage 对象数据字典中，`k` 指定键值存在，且可**被 MsgPack 转换**为整型，则返回对应的整型数据，否则将参数 `TYPE dft` 的值作为替代值返回。

**参数说明**

* **`const char *k`** & **`const std::string& k`**

	需要获取的数据的键值。

* **`intmax_t dft`**

	如果指定的键值不存在时，作为替代，返回的数据。

##### getUInt

	uintmax_t getUInt(const char* k, uintmax_t dft = _uintDef);
	uintmax_t getUInt(const std::string& k, uintmax_t dft = _uintDef);

如果 FPMessage 对象数据字典中，`k` 指定键值存在，且可**被 MsgPack 转换**为无符号整型，则返回对应的无符号整型数据，否则将参数 `TYPE dft` 的值作为替代值返回。

**参数说明**

* **`const char *k`** & **`const std::string& k`**

	需要获取的数据的键值。

* **`uintmax_t dft`**

	如果指定的键值不存在时，作为替代，返回的数据。

##### getDouble

	double getDouble(const char* k, double dft = _doubleDef);
	double getDouble(const std::string& k, double dft = _doubleDef);

如果 FPMessage 对象数据字典中，`k` 指定键值存在，且可**被 MsgPack 转换**为双精度浮点型，则返回对应的双精度浮点型数据，否则将参数 `TYPE dft` 的值作为替代值返回。

**参数说明**

* **`const char *k`** & **`const std::string& k`**

	需要获取的数据的键值。

* **`double dft`**

	如果指定的键值不存在时，作为替代，返回的数据。

##### getBool

	bool getBool(const char* k, bool dft = _boolDef);
	bool getBool(const std::string& k, bool dft = _boolDef);

如果 FPMessage 对象数据字典中，`k` 指定键值存在，且可**被 MsgPack 转换**为布尔型，则返回对应的布尔型数据，否则将参数 `TYPE dft` 的值作为替代值返回。

**参数说明**

* **`const char *k`** & **`const std::string& k`**

	需要获取的数据的键值。

* **`bool dft`**

	如果指定的键值不存在时，作为替代，返回的数据。

##### getString

	std::string getString(const char* k, std::string dft = _stringDef);
	std::string getString(const std::string& k, std::string dft = _stringDef);

如果 FPMessage 对象数据字典中，`k` 指定键值存在，且可**被 MsgPack 转换**为字符串类型，则返回对应的字符串类型数据，否则将参数 `TYPE dft` 的值作为替代值返回。

**参数说明**

* **`const char *k`** & **`const std::string& k`**

	需要获取的数据的键值。

* **`std::string dft`**

	如果指定的键值不存在时，作为替代，返回的数据。

##### getFile

	bool getFile(const char* k, FileSystemUtil::FileAttrs& attrs);
	bool getFile(const std::string& k, FileSystemUtil::FileAttrs& attrs);

如果 FPMessage 对象数据字典中，`k` 指定键值存在，且为字典类型，包含`name`、`sign`、`content`、`ext` 4个字符串类型字段，且还包含 `size`、`atime`、`mtime`、`ctime` 4 个整型字段，则 SDK 将提取 `k` 指定键值对应的字典，并尝试转换为 `FileSystemUtil::FileAttrs` 对象。

**参数说明**

* **`const char *k`** & **`const std::string& k`**

	需要获取的文件数据的键值。

* **`FileSystemUtil::FileAttrs& attrs`**

	提取 & 转换成功时，返回的 `FileSystemUtil::FileAttrs& attrs` 对象。

**返回值**

	| 返回值 | 含义 |
	|-------|-----|
	| true | 提取 & 转换成功 |
	| false | 提取 & 转换失败 |

##### want

	template<typename TYPE>
		TYPE want(const char *k, TYPE dft);

	template<typename TYPE>
		TYPE want(const std::string& k, TYPE dft);

按模板参数类型获取 FPMessage 对象数据字典中，`k` 指定键值的数据。  
如果对应键值不存在，抛出[FPNN 异常][FPNNError]。

**参数说明**

* **`const char *k`** & **`const std::string& k`**

	需要获取的数据的键值。

* **`TYPE dft`**

	供类型转换参考使用。

##### wantInt

	intmax_t wantInt(const char* k, intmax_t dft = _intDef);
	intmax_t wantInt(const std::string& k, intmax_t dft = _intDef);

如果 FPMessage 对象数据字典中，`k` 指定键值存在，且可**被 MsgPack 转换**为整型，则返回对应的整型数据，否则将抛出[FPNN 异常][FPNNError]。

**参数说明**

* **`const char *k`** & **`const std::string& k`**

	需要获取的数据的键值。

* **`intmax_t dft`**

	供类型转换参考使用。

##### wantUInt

	uintmax_t wantUInt(const char* k, uintmax_t dft = _uintDef);
	uintmax_t wantUInt(const std::string& k, uintmax_t dft = _uintDef);

如果 FPMessage 对象数据字典中，`k` 指定键值存在，且可**被 MsgPack 转换**为无符号整型，则返回对应的无符号整型数据，否则将抛出[FPNN 异常][FPNNError]。

**参数说明**

* **`const char *k`** & **`const std::string& k`**

	需要获取的数据的键值。

* **`uintmax_t dft`**

	供类型转换参考使用。

##### wantDouble

	double wantDouble(const char* k, double dft = _doubleDef);
	double wantDouble(const std::string& k, double dft = _doubleDef);

如果 FPMessage 对象数据字典中，`k` 指定键值存在，且可**被 MsgPack 转换**为双精度浮点型，则返回对应的双精度浮点型数据，否则将抛出[FPNN 异常][FPNNError]。

**参数说明**

* **`const char *k`** & **`const std::string& k`**

	需要获取的数据的键值。

* **`double dft`**

	供类型转换参考使用。

##### wantBool

	bool wantBool(const char* k, bool dft = _boolDef);
	bool wantBool(const std::string& k, bool dft = _boolDef);

如果 FPMessage 对象数据字典中，`k` 指定键值存在，且可**被 MsgPack 转换**为布尔型，则返回对应的布尔型数据，否则将抛出[FPNN 异常][FPNNError]。

**参数说明**

* **`const char *k`** & **`const std::string& k`**

	需要获取的数据的键值。

* **`bool dft`**

	供类型转换参考使用。

##### wantString

	std::string wantString(const char* k, std::string dft = _stringDef);
	std::string wantString(const std::string& k, std::string dft = _stringDef);

如果 FPMessage 对象数据字典中，`k` 指定键值存在，且可**被 MsgPack 转换**为字符串类型，则返回对应的字符串类型数据，否则将抛出[FPNN 异常][FPNNError]。

**参数说明**

* **`const char *k`** & **`const std::string& k`**

	需要获取的数据的键值。

* **`std::string dft`**

	供类型转换参考使用。

##### wantFile

	bool wantFile(const char* k, FileSystemUtil::FileAttrs& attrs);
	bool wantFile(const std::string& k, FileSystemUtil::FileAttrs& attrs);

如果 FPMessage 对象数据字典中，`k` 指定键值存在，且为字典类型，包含`name`、`sign`、`content`、`ext` 4个字符串类型字段，且还包含 `size`、`atime`、`mtime`、`ctime` 4 个整型字段，则 SDK 将提取 `k` 指定键值对应的字典，并尝试转换为 `FileSystemUtil::FileAttrs` 对象。否则将抛出[FPNN 异常][FPNNError]。

**参数说明**

* **`const char *k`** & **`const std::string& k`**

	需要获取的文件数据的键值。

* **`FileSystemUtil::FileAttrs& attrs`**

	提取 & 转换成功时，返回的 `FileSystemUtil::FileAttrs& attrs` 对象。

**返回值**

	| 返回值 | 含义 |
	|-------|-----|
	| true | 提取 & 转换成功 |
	| false | 提取 & 转换失败（实际不会发生，而是抛出[FPNN 异常][FPNNError]） |

##### convert

	template<typename TYPE>
		TYPE convert(TYPE& dst);

转换 MsgPack 数据类型。

##### getObjectType

	msgpack::type::object_type getObjectType(const char* k);
	msgpack::type::object_type getObjectType(const std::string& k);

获取指定键值的 msgPack 数据类型。

**参数说明**

* **`const char *k`** & **`const std::string& k`**

	需要获取 MsgPack 类型的数据的键值。

**返回值**

	MsgPack 数据类型。

##### isInt

	bool isInt(const char* k);
	bool isInt(const std::string& k);

判断键值对象是否是整型类型。

**参数说明**

* **`const char *k`** & **`const std::string& k`**

	需要判断整型类型的数据的键值。

##### isBool

	bool isBool(const char* k);
	bool isBool(const std::string& k);

判断键值对象是否是布尔类型。

**参数说明**

* **`const char *k`** & **`const std::string& k`**

	需要判断布尔类型的数据的键值。

##### isDouble

	bool isDouble(const char* k);
	bool isDouble(const std::string& k);

判断键值对象是否是浮点型类型。

**参数说明**

* **`const char *k`** & **`const std::string& k`**

	需要判断浮点型类型的数据的键值。

##### isString

	bool isString(const char* k);
	bool isString(const std::string& k);

判断键值对象是否是字符串类型。

**参数说明**

* **`const char *k`** & **`const std::string& k`**

	需要判断字符串类型的数据的键值。

##### isBinary

	bool isBinary(const char* k);
	bool isBinary(const std::string& k);

判断键值对象是否是二进制类型。

**参数说明**

* **`const char *k`** & **`const std::string& k`**

	需要判断二进制类型的数据的键值。

##### isArray

	bool isArray(const char* k);
	bool isArray(const std::string& k);

判断键值对象是否是数组类型。

**参数说明**

* **`const char *k`** & **`const std::string& k`**

	需要判断数组类型的数据的键值。

##### isMap

	bool isMap(const char* k);
	bool isMap(const std::string& k);

判断键值对象是否是字典类型。

**参数说明**

* **`const char *k`** & **`const std::string& k`**

	需要判断字典类型的数据的键值。

##### isExt

	bool isExt(const char* k);
	bool isExt(const std::string& k);

判断键值对象是否存在。

**参数说明**

* **`const char *k`** & **`const std::string& k`**

	需要判断是否存在的数据的键值。

##### isNil

	bool isNil(const char* k);
	bool isNil(const std::string& k);

判断键值对象是否是 null 类型。

**参数说明**

* **`const char *k`** & **`const std::string& k`**

	需要判断 null 类型的数据的键值。

##### json

	std::string json();

将 FPMessage 对象数据转换为 Json 格式。如果无法转换，则返回空字符串。



### FPQReader

FPQReader 为 [FPReader](#FPReader) 的子类。提供读取 [FPNN 协议请求对象（FPQuest）](FPQuest.md) 的能力。

#### 定义

	class FPQReader : public FPReader{
	public:
		FPQReader(const FPQuestPtr& quest);

		bool isHTTP();
		bool isTCP();
		bool isOneWay();
		bool isTwoWay();
		bool isQuest();
		uint32_t seqNum() const;
		const std::string& method() const;
	};

	typedef std::shared_ptr<FPQReader> FPQReaderPtr;

#### 构造函数

**参数说明**

* **`const FPQuestPtr& quest`**

	需要读取的 FPQuest 对象。

#### 成员函数

##### isHTTP

	bool isHTTP();

判断是否是 HTTP/HTTPS 数据。

##### isTCP

	bool isTCP();

判断是否是 FPNN 协议数据。

##### isOneWay

	bool isOneWay();

判断是否是单向请求。

##### isTwoWay

	bool isTwoWay();

判断是否是双向请求。

##### isQuest

	bool isQuest();

判断是否是请求数据。

##### seqNum

	uint32_t seqNum() const;

返回应答数据包的序列号（本地字节序）。

##### method

	const std::string& method() const;

返回请求的方法名/接口名。


### FPAReader

FPAReader 为 [FPReader](#FPReader) 的子类。提供读取 [FPNN 协议应答对象（FPAnswer）](FPAnswer.md) 的能力。

#### 定义

	class FPAReader : public FPReader{
	public:
		FPAReader(const FPAnswerPtr& answer);  

		uint32_t seqNum() const;
		uint16_t status() const;
	};

	typedef std::shared_ptr<FPAReader> FPAReaderPtr;

#### 构造函数

**参数说明**

* **`const FPAnswerPtr& answer`**

	需要读取的 FPAnswer 对象。

#### 成员函数

##### seqNum

	uint32_t seqNum() const;

返回应答数据包的序列号（本地字节序）。

##### status

	uint16_t status() const;

返回 FPAnswer 对象的状态码。


[FPNNError]: ../base/FpnnError.md#FpnnError