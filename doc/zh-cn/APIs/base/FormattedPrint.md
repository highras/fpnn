## FormattedPrint

### 介绍

格式化控制台输出。主要用于测试和调试目的。

### 命名空间

	namespace fpnn;

### 全局函数

#### formatBytesQuantity

	std::string formatBytesQuantity(unsigned long long quantity, int outputRankCount = 0);

以 1024 为单位，格式化字节数显示格式。类似于：`117 M 755 K 277 B` 的格式。

**参数说明**

* **`unsigned long long quantity`**

	需要格式化的数值。

* **`int outputRankCount`**

	指定显示单位等级的数量。

	例：

	quantity = 123456789;

	outputRankCount == 0 or outputRankCount >= 3: 117 M 755 K 277 B
	outputRankCount == 2:                         117 M 755 KB
	outputRankCount == 1:                         117 MB

#### visibleBinaryBuffer

	std::string visibleBinaryBuffer(const void* memory, size_t size, const std::string& delim = " ");

输出指定内存块的可视化内容。

**参数说明**

* **`const void* memory`**

	指定内存的起始地址。

* **`size_t size`**

	需要输出的内存数据长度。

* **`const std::string& delim`**

	分隔符。默认输出8 个字节后，会输出一个分隔符。默认为空格。  
	**注意**如果不做后续处理，而直接输出，分隔符建议采用 "\n"。


#### printTable

	void printTable(const std::vector<std::string>& fields, const std::vector<std::vector<std::string>>& rows);

格式化表格，以类似于 MySQL client 那样的格式，输出表格内容。

**参数说明**

* **`const std::vector<std::string>& fields`**

	表格列名称。

* **`const std::vector<std::vector<std::string>>& rows`**

	表格数据。