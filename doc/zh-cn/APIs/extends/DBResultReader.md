## DBResultReader

### 介绍

[DBProxy](https://github.com/highras/dbproxy) 查询返回结果读取器。

### 命名空间

	namespace fpnn;

### 关键定义

	class DBResultReader
	{
	public:
		DBResultReader(const fpnn::FPAnswerPtr& answer);
		~DBResultReader();

		//---------------[ for exception ]-----------------//
		void exception();
		inline bool isException();

		inline int code();
		inline std::string ex();

		//---------------[ for write options ]-----------------//
		inline int affectedRows();
		inline int insertId();

		//---------------[ for invalied & failed Ids ]-----------------//
		inline const std::vector<int64_t>& invalidIds();
		inline const std::vector<int64_t>& failedIds(int);
		inline const std::vector<std::string>& failedIds(const std::string&);

		//---------------[ for select & desc options ]-----------------//
		inline const std::vector<std::string>& fields();
		inline int fieldIndex(const std::string &fieldName);

		inline int rowsCount();
		inline const std::vector<std::string>& row(int index);
		inline const std::vector<std::vector<std::string> >& rows();

		inline const std::string& cell(int rowIndex, int fieldIndex);
		inline const std::string& cell(int rowIndex, const std::string& fieldName);
		
		inline int64_t intCell(int rowIndex, int fieldIndex);
		inline int64_t intCell(int rowIndex, const std::string& fieldName);
		
		inline double realCell(int rowIndex, int fieldIndex);
		inline double realCell(int rowIndex, const std::string& fieldName);
	};

### 构造函数

	DBResultReader(const fpnn::FPAnswerPtr& answer);

**参数说明**

* **`const fpnn::FPAnswerPtr& answer`**

	[DBProxy](https://github.com/highras/dbproxy) 查询返回的结果。

### 成员函数

#### exception

	void exception();

检测返回是否是异常返回。如果是，则转换为 [FPNNError](../base/FpnnError.md#FpnnError)，并抛出。

#### isException

	inline bool isException();

检测是否是异常返回。

#### code

	inline int code();

如果是异常返回，返回错误代码。

#### ex

	inline std::string ex();

如果是异常返回，返回错误描述。

#### affectedRows

	inline int affectedRows();

Insert、replace、delete、update 等操作影响的条目数量。

#### insertId

	inline int insertId();

插入条目的 Id。

#### invalidIds

	inline const std::vector<int64_t>& invalidIds();

无效的 hintId。

#### failedIds

	inline const std::vector<int64_t>& failedIds(int);
	inline const std::vector<std::string>& failedIds(const std::string&);

执行失败的分片的 hintId 列表。

**注意**

+ 第一个形式针对请求的 hintId 是整型类型，第二个形式，针对的请求的 hintId 是字符串类型。
+ 函数参数用于区别函数签名，避免编译器混淆。参数值无实际用途。

#### fields

	inline const std::vector<std::string>& fields();

对于查询，返回的结果集的字段列表。

#### fieldIndex

	inline int fieldIndex(const std::string &fieldName);

获取指定字段在返回的结果集中的索引。

#### rowsCount

	inline int rowsCount();

返回的结果集的行数。

#### row

	inline const std::vector<std::string>& row(int index);

获取返回结果集中的指定行的数据。

#### rows

	inline const std::vector<std::vector<std::string> >& rows();

获取返回的结果集。

#### cell

	inline const std::string& cell(int rowIndex, int fieldIndex);
	inline const std::string& cell(int rowIndex, const std::string& fieldName);

获取返回的结果集中，指定行处，指定字段/指定字段索引位置的字符串值。
	
#### intCell

	inline int64_t intCell(int rowIndex, int fieldIndex);
	inline int64_t intCell(int rowIndex, const std::string& fieldName);

获取返回的结果集中，指定行处，指定字段/指定字段索引位置的整型数值。
	
#### realCell

	inline double realCell(int rowIndex, int fieldIndex);
	inline double realCell(int rowIndex, const std::string& fieldName);

获取返回的结果集中，指定行处，指定字段/指定字段索引位置的双精度浮点型数值。
