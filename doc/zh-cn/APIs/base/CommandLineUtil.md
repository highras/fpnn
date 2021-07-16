## CommandLineUtil

### 介绍

命令行辅助工具库。

### 命名空间

	namespace fpnn;

### CommandLineParser

命令行解析工具。  
支持以 `-` 或 `--` 为前导的参数(标记或键值对)和无前导标志的参数混合，支持变长参数。

**注意：**

* 不建议无前导标志的参数跟随在有前导标志的标记(没有值的参数)后，否则会被作为该标记的值，而非无前导标志的参数。

#### 关键定义

	class CommandLineParser
	{
	public:
		static void init(int argc, const char* const * argv, int beginIndex = 1);

		static std::string getString(const std::string& sign, const std::string& dft = std::string());
		static intmax_t getInt(const std::string& sign, intmax_t dft = 0);
		static bool getBool(const std::string& sign, bool dft = false);
		static double getReal(const std::string& sign, double dft = 0.0);
		static bool exist(const std::string& sign);

		static std::vector<std::string> getRestParams();
	};

### 成员函数

#### init

	static void init(int argc, const char* const * argv, int beginIndex = 1);

初始化函数。

**参数说明**

+ `int argc`

	同 main() 函数的 argc 参数。直接传递即可。

+ `const char* const * argv`

	同 main() 函数的 argv 参数。直接传递即可。

+ `int beginIndex`

	需要处理的参数的起始位置。该参数可以指定跳过预定个数的特定参数。

	例：

		example_exe argv1, argv2, argv3, ...

	如果 argv1、argv2 有固定用途，则指定 `beginIndex = 3` 则会从 argv3 开始处理，而忽略 argv1 和 argv2。  
	此时调用 `getRestParams()` 获取的无前导参数列表，也不包含 argv1 和 argv2。

#### getString

	static std::string getString(const std::string& sign, const std::string& dft = std::string());

以字符串形式，获取 `sign` 参数指定的标记的值。如果指定标记不存在，则使用 `dft` 参数作为替代。

#### getInt

	static intmax_t getInt(const std::string& sign, intmax_t dft = 0);

以整型形式，获取 `sign` 参数指定的标记的值。如果指定标记不存在，则使用 `dft` 参数作为替代。

#### getBool

	static bool getBool(const std::string& sign, bool dft = false);

以布尔值形式，获取 `sign` 参数指定的标记的值。如果指定标记不存在，则使用 `dft` 参数作为替代。

#### getReal

	static double getReal(const std::string& sign, double dft = 0.0);

以双精度浮点型形式，获取 `sign` 参数指定的标记的值。如果指定标记不存在，则使用 `dft` 参数作为替代。

#### exist

	static bool exist(const std::string& sign);

检查由 `sign` 参数指定的标记是否存在。

#### getRestParams

	static std::vector<std::string> getRestParams();

获取无前导标志的参数列表。
