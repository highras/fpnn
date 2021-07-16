## Setting

### 介绍

配置设置与获取模块。

**注意：配置文件规范**

+ 配置文件为字典风格，类似于 ini 文件，以 '=' 分割键值。
+ 键名不允许重复，否则会发生覆盖。
+ 键名以 '.' 分割，表示配置所属模块或者分段。
+ 配置文件不存在类似于 ini 文件的分段或者段落。
+ 行首 '#' 开始，表示该行为注释。
+ 如果键值用空格 ' '，半角逗号 ','，制表符 '\t'，或者回车、换行分割，则可用 `getStringList()` 接口，直接获取字符串形式的列表。
+ 对于整型键值：
	+ 键值支持不区分大小写的 "KMGTPEZY" 后缀。比如 2M，3k
	+ 键值如果以 'i' 或者 'I' 结尾，表示 "KMGTPEZY" 后缀的倍率为 1024， 否则为 1000。
+ 对于布尔型键值：
	+ 支持 'true' 'yes' 'false' 'no' 't' 'f' 'y' 'n' 的小写表述
	+ 支持整型表述

### 命名空间

	namespace fpnn;

### 关键定义

	class Setting
	{
	public:
		typedef std::unordered_map<std::string, std::string> MapType;
	public:

		static std::string getString(const std::string& name, const std::string& dft = std::string(), const MapType& map = _map);
		static intmax_t getInt(const std::string& name, intmax_t dft = 0, const MapType& map = _map);
		static bool getBool(const std::string& name, bool dft = false, const MapType& map = _map);
		static double getReal(const std::string& name, double dft = 0.0, const MapType& map = _map);
		static std::vector<std::string> getStringList(const std::string& name, const MapType& map = _map);

		static void set(const std::string& name, const std::string& value);
		static bool insert(const std::string& name, const std::string& value);
		static bool update(const std::string& name, const std::string& value);

		static bool load(const std::string& file);
		static MapType loadMap(const std::string& file);
		static std::string getFileMD5(const std::string& file);

		static bool setted(const std::string& name, const MapType& map = _map);

		static void printInfo();
		static std::string getConfigFile();

		static std::string getString(const std::vector<std::string>& priority, const std::string& dft = std::string(), const MapType& map = _map);
		static intmax_t getInt(const std::vector<std::string>& priority, intmax_t dft = 0, const MapType& map = _map);
		static bool getBool(const std::vector<std::string>& priority, bool dft = false, const MapType& map = _map);
		static double getReal(const std::vector<std::string>& priority, double dft = 0.0, const MapType& map = _map);
	};

### 成员函数

#### getString

	static std::string getString(const std::string& name, const std::string& dft = std::string(), const MapType& map = _map);
	static std::string getString(const std::vector<std::string>& priority, const std::string& dft = std::string(), const MapType& map = _map);

以字符串形式，获取指定键名的值。如果指定的键名不存在，则返回替代值。

**参数说明**

* **`const std::string& name`**

	指定的键名。

* **`const std::vector<std::string>& priority`**

	指定的键名组。  
	如果有多个键名可以采用，但有优先次序，则优先返回键名组中，按次序先匹配上的键名对应的键值。

* **`const std::string& dft`**

	键名无匹配时，返回的默认值。

* **`const MapType& map`**

	配置字典。默认是 Setting 内部字典。


#### getInt

	static intmax_t getInt(const std::string& name, intmax_t dft = 0, const MapType& map = _map);
	static intmax_t getInt(const std::vector<std::string>& priority, intmax_t dft = 0, const MapType& map = _map);

以整型形式，获取指定键名的值。如果指定的键名不存在，或者无法转换成整型，则返回替代值。

**注意**

+ 键值支持不区分大小写的 "KMGTPEZY" 后缀。比如 2M，3k
+ 键值如果以 'i' 或者 'I' 结尾，表示 "KMGTPEZY" 后缀的倍率为 1024， 否则为 1000。

**参数说明**

* **`const std::string& name`**

	指定的键名。

* **`const std::vector<std::string>& priority`**

	指定的键名组。  
	如果有多个键名可以采用，但有优先次序，则优先返回键名组中，按次序先匹配上的键名对应的键值。

* **`intmax_t dft`**

	键名无匹配时，返回的默认值。

* **`const MapType& map`**

	配置字典。默认是 Setting 内部字典。

#### getBool

	static bool getBool(const std::string& name, bool dft = false, const MapType& map = _map);
	static bool getBool(const std::vector<std::string>& priority, bool dft = false, const MapType& map = _map);

以布尔值形式，获取指定键名的值。如果指定的键名不存在，或者无法转换成布尔类型，则返回替代值。

**注意**

+ 支持 'true' 'yes' 'false' 'no' 't' 'f' 'y' 'n' 的小写表述
+ 支持整型表述

**参数说明**

* **`const std::string& name`**

	指定的键名。

* **`const std::vector<std::string>& priority`**

	指定的键名组。  
	如果有多个键名可以采用，但有优先次序，则优先返回键名组中，按次序先匹配上的键名对应的键值。

* **`bool dft`**

	键名无匹配时，返回的默认值。

* **`const MapType& map`**

	配置字典。默认是 Setting 内部字典。

#### getReal

	static double getReal(const std::string& name, double dft = 0.0, const MapType& map = _map);
	static double getReal(const std::vector<std::string>& priority, double dft = 0.0, const MapType& map = _map);

以双精度浮点型形式，获取指定键名的值。如果指定的键名不存在，或者无法转换成浮点类型，则返回替代值。

**参数说明**

* **`const std::string& name`**

	指定的键名。

* **`const std::vector<std::string>& priority`**

	指定的键名组。  
	如果有多个键名可以采用，但有优先次序，则优先返回键名组中，按次序先匹配上的键名对应的键值。

* **`double dft`**

	键名无匹配时，返回的默认值。

* **`const MapType& map`**

	配置字典。默认是 Setting 内部字典。

#### getStringList

	static std::vector<std::string> getStringList(const std::string& name, const MapType& map = _map);

以字符串列表的形式，获取指定键名的值。如果指定的键名不存在，则返回空列表。

**注意**

键值须用 空格 ' '，半角逗号 ','，制表符 '\t'，或者回车、换行分割。

**参数说明**

* **`const std::string& name`**

	指定的键名。

* **`const MapType& map`**

	配置字典。默认是 Setting 内部字典。


#### set

	static void set(const std::string& name, const std::string& value);

设置键值对。如果键已存在，则覆盖。

#### insert

	static bool insert(const std::string& name, const std::string& value);

设置键值对。如果键已存在，则返回 false。不会覆盖。

#### update

	static bool update(const std::string& name, const std::string& value);

更新键值对。如果键不存在，则返回 false。

#### load

	static bool load(const std::string& file);

加载配置文件。

#### loadMap

	static MapType loadMap(const std::string& file);

加载配置文件。

#### getFileMD5

	static std::string getFileMD5(const std::string& file);

获取文件 md5 值。

#### setted

	static bool setted(const std::string& name, const MapType& map = _map);

检测键值是否已设置。

#### printInfo

	static void printInfo();

在标准输出中，打印配置字典。（仅内部调试用）

#### getConfigFile

	static std::string getConfigFile();

获取配置文件名称。
