## StringUtil

### 介绍

字符串处理工具模块。

### 命名空间

	namespace fpnn::StringUtil;

### 全局函数

#### trim 系列

##### rtrim

	char * rtrim(char *s);

裁剪右侧。无数据移动，但**可能会修改原始数据**，以抛弃右侧空白值。

	std::string& rtrim(std::string& s);

裁剪右侧，**会修改原始数据**，抛弃右侧空白值。


##### ltrim

	char * ltrim(char *s);

裁剪左侧。无数据移动，返回新的起始指针。

	std::string& ltrim(std::string& s);

裁剪左侧，**会修改原始数据**，抛弃左侧空白值。

##### trim

	char * trim(char *s);

双端裁剪。无数据移动，但**可能会修改原始数据**，以抛弃右侧空白值，并返回新的起始指针。

	std::string& trim(std::string& s);

双端裁剪。**会修改原始数据**，抛弃双侧空白值。


##### softTrim

	void softTrim(const char* path, char* &start, char* &end);

双端裁剪，无数据移动，**不修改**原始数据。返回有效数据的起始和结束指针。**注意**，有效数据不包含 `end` 指向位置的字符。

#### replace

	bool replace(std::string& str, const std::string& from, const std::string& to);

将 `str` 中 `from` 第一次出现的地方替换为 `to`。

#### split

	//will discard empty field
	std::vector<std::string> &split(const std::string &s, const std::string& delim, std::vector<std::string> &elems);

	//-- Only for integer or long.
	template<typename T>
	std::vector<T> &split(const std::string &s, const std::string& delim, std::vector<T> &elems);

	//-- Only for integer or long.
	template<typename T>
	std::set<T> &split(const std::string &s, const std::string& delim, std::set<T> &elems);

	//-- Only for integer or long.
	template<typename T>
	std::unordered_set<T> &split(const std::string &s, const std::string& delim, std::unordered_set<T> &elems);

	std::set<std::string> &split(const std::string &s, const std::string& delim, std::set<std::string> &elems);
	std::unordered_set<std::string> &split(const std::string &s, const std::string& delim, std::unordered_set<std::string> &elems);

按照 `delim` 中出现的**字符**切分字符串 `s`，忽略空串后，放入容器对象 `elems` 中，并将 `elems` 返回。

**注意：**其中模版函数仅适用于**整型**类型。


#### join

	template<typename T>
	std::string join(const std::set<T> &v, const std::string& delim);

	template<typename T>
	std::string join(const std::vector<T> &v, const std::string& delim);

	std::string join(const std::set<std::string> &v, const std::string& delim);
	std::string join(const std::vector<std::string> &v, const std::string& delim);
	std::string join(const std::map<std::string, std::string> &v, const std::string& delim);

将容器 `v` 中的数据，用字符串 `delim` 拼接并返回。


#### escapseString

	std::string escapseString(const std::string& s);

内部用函数，用于拼接字符串数据到 Json 字符串中。

**注意**

一般情况下，请使用 [escape_string](escapeString.md#escape_string)。


### CharsChecker

字符匹配检查类。

	class CharsChecker
	{
	public:
		CharsChecker(const unsigned char *targets);
		CharsChecker(const unsigned char *targets, int len);
		CharsChecker(const std::string& targets);
		inline bool operator[] (unsigned char c);
	};

主要用于自定义字符集合的检查。类似于 `isspace()` 等函数。

### CharMarkMap

字符位掩码匹配检查。

	template<typename UNSIGNED_INTEGER_TYPE>
	class CharMarkMap
	{
	public:
		CharMarkMap();
		void init(const unsigned char *targets, UNSIGNED_INTEGER_TYPE mark);
		void init(const unsigned char *targets, int len, UNSIGNED_INTEGER_TYPE mark);

		void init(unsigned char c, UNSIGNED_INTEGER_TYPE mark);

		inline void init(const char *targets, UNSIGNED_INTEGER_TYPE mark);
		inline void init(const char *targets, int len, UNSIGNED_INTEGER_TYPE mark);
		inline void init(char c, UNSIGNED_INTEGER_TYPE mark);

		inline UNSIGNED_INTEGER_TYPE operator[] (unsigned char c);

		inline bool check(unsigned char c, UNSIGNED_INTEGER_TYPE mark);

		inline bool check(char c, UNSIGNED_INTEGER_TYPE mark);
	};

主要用于自定义的字符集合及其成员字符关联掩码的匹配和查询。