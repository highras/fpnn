## FPJson

### 介绍

FPJson 为一个极其简便的 C++ Json 编解码工具。

设计目标为**任何操作**，仅需一行代码。  
无论是解析、序列化、存取、设置修改、添加，以及打包任意复杂度的 STL 容器，均只需一行代码。

**注意**

目前仅支持 UTF-8 编码，包含支持 UTF-8 编码的 UTF-16 代理对。不支持 BOM 头。

本文档为 API 文档，具体使用实例请参见 [swxJson](https://github.com/swxlion/swxJson)。

### 命名空间

	namespace fpnn;

### 关键定义

	class Json;
	typedef std::shared_ptr<Json> JsonPtr;

	std::ostream& operator << (std::ostream& os, const Json& node);
	std::ostream& operator << (std::ostream& os, const JsonPtr node);

	class Json
	{
	public:
		enum ElementType
		{
			JSON_Object,
			JSON_Array,
			JSON_String,
			JSON_Integer,
			JSON_UInteger,
			JSON_Real,
			JSON_Boolean,
			JSON_Null,
			JSON_Uninit
		};

	public:
		Json();
		~Json();

		static JsonPtr parse(const char* data);
		std::string str();

		... ... ...
	};

### 构造函数

	Json();

创建一个未初始化根节点。可以通过赋值、修改节点类型、添加数据等操作，进行初始化。

### 解析

	static JsonPtr parse(const char* data);

将一个以 '\0' 结尾的 UTF-8 字符串，解析为 FPJson 对象。  
如果解析错误，将会抛出 [FPNN 异常][FPNNError]。

### 序列化

	std::string str();

成员函数 `str()` 将 FPJson 对象序列化为字符串，并返回。

通过全局重载函数：

	std::ostream& operator << (std::ostream& os, const Json& node);
	std::ostream& operator << (std::ostream& os, const JsonPtr node);

可将 FPJson 直接序列化到 STL 标准输出流中。

示例：
	
	//-- jsonObject is instance of Json or JsonPtr.
	std::cout<<jsonObject<<std::endl;

### 修改节点类型

	void setNull();
	void setBool(bool value);
	void setInt(intmax_t value);
	void setUInt(uintmax_t value);
	void setReal(double value);
	void setString(const char* value);
	void setString(const std::string& value);
	void setArray();		//-- empty array
	void setDict();			//-- empty dict

将**当前** FPJson 节点设置为 Json 基本类型节点，并赋值。

**注意**

如果当前节点是数组节点，或者对象/字典节点，任何修改节点类型的操作，**均会清除**当前节点的所有子节点。

#### 高级操作：修改为容器类型节点并同时赋值

* 将**当前节点**设置为**数组节点**，并添加任意数量成员

		template<typename... Args>
		void setArray(Args... args);

* 将**当前节点**设置为**数组节点**，然后复制 STL 容器对象中的数据

		template<class... Args>
		void setArray(const std::tuple<Args...>& tup);

		template < class T, size_t N >
		void setArray(const std::array<T, N>& arr);

		template < class T, class Alloc = std::allocator<T> >
		void setArray(const std::deque<T, Alloc>& deq);


		template <template <typename, typename> class Container, typename T, class Alloc = std::allocator<T>>
		void setArray(const Container<T, Alloc>& container);

		template <class Alloc = std::allocator<bool>>
		void setArray(const std::vector<bool, Alloc>& container);

		template < class T, class Compare = std::less<T>, class Alloc = std::allocator<T> >
		void setArray(const std::set<T, Compare, Alloc>& collections);

		template < class Key, class Hash = std::hash<Key>, class Pred = std::equal_to<Key>, class Alloc = std::allocator<Key> >
		void setArray(const std::unordered_set<Key, Hash, Pred, Alloc>& collections);


* 将**当前节点**设置为**字典节点**，然后复制 STL 容器对象中的数据

		template <class T, class Compare = std::less<std::string>, class Alloc = std::allocator<std::pair<const std::string, T> > >
		void setDict(const std::map<std::string, T, Compare, Alloc>& theMap);

		template <class T, class Hash = std::hash<std::string>, class Pred = std::equal_to<std::string>, class Alloc = std::allocator<std::pair<const std::string, T> > >
		void setDict(const std::unordered_map<std::string, T, Hash, Pred, Alloc>& theMap);


### 赋值

	Json& operator = (bool value)               { setBool(value);   return *this; }
	Json& operator = (double value)             { setReal(value);   return *this; }
	Json& operator = (long double value)        { setReal(value);   return *this; }
	Json& operator = (const char* value)        { setString(value); return *this; }
	Json& operator = (const std::string& value) { setString(value); return *this; }

	Json& operator = (short value)              { setInt(value);  return *this; }
	Json& operator = (unsigned short value)     { setUInt(value); return *this; }
	Json& operator = (int value)                { setInt(value);  return *this; }
	Json& operator = (unsigned int value)       { setUInt(value); return *this; }
	Json& operator = (long value)               { setInt(value);  return *this; }
	Json& operator = (unsigned long value)      { setUInt(value); return *this; }
	Json& operator = (long long value)          { setInt(value);  return *this; }
	Json& operator = (unsigned long long value) { setUInt(value); return *this; }

对于整型、浮点型、字符串类型的数据，可以对当前 FPJson 节点直接赋值。赋值将修改对应的节点类型，并清除所有的子节点（如果存在）。

#### 高级操作：直接赋与 STL 容器及所含数据

	template<class... Args>
	Json& operator = (const std::tuple<Args...>& tup);

	template < class T, size_t N >
	Json& operator = (const std::array<T, N>& arr);

	template < class T, class Alloc = std::allocator<T> >
	Json& operator = (const std::deque<T, Alloc>& deq);

	template <template <typename, typename> class Container, typename T, class Alloc = std::allocator<T>>
	Json& operator = (const Container<T, Alloc>& container);

	template < class T, class Compare = std::less<T>, class Alloc = std::allocator<T> >
	Json& operator = (const std::set<T, Compare, Alloc>& collections);

	template < class Key, class Hash = std::hash<Key>, class Pred = std::equal_to<Key>, class Alloc = std::allocator<Key> >
	Json& operator = (const std::unordered_set<Key, Hash, Pred, Alloc>& collections);

	template <class T, class Compare = std::less<std::string>, class Alloc = std::allocator<std::pair<const std::string, T> > >
	Json& operator = (const std::map<std::string, T, Compare, Alloc>& theMap);

	template <class T, class Hash = std::hash<std::string>, class Pred = std::equal_to<std::string>, class Alloc = std::allocator<std::pair<const std::string, T> > >
	Json& operator = (const std::unordered_map<std::string, T, Hash, Pred, Alloc>& theMap);

### 添加数据

添加数据分成 push 操作、add 操作、merge 操作和杂类。

数组节点使用 push 系列接口和 merge 系列接口添加数据，对象节点/字典节点使用 add 系列接口和 merge 系列接口添加数据。

通俗理解：

Push/add 每次添加一个**新**的子节点，不论数据有多少，都向该子节点中添加；  
merge 向当前节点中添加数据，不论数据多少，都向当前节点中添加。

#### 向数组中添加数据

##### 向**当前**的数组节点中添加数据

	bool push(bool value);
	bool push(double value);
	bool push(long double value);
	bool push(const char* value);
	bool push(const std::string& value);

	bool push(short value);
	bool push(unsigned short value);
	bool push(int value);
	bool push(unsigned int value);
	bool push(long value);
	bool push(unsigned long value);
	bool push(long long value);
	bool push(unsigned long long value);

	bool pushNull();
	bool pushBool(bool value);
	bool pushReal(double value);
	bool pushInt(intmax_t value);
	bool pushUInt(uintmax_t value);
	bool pushString(const char* value);
	bool pushString(const std::string& value);
	JsonPtr pushArray();		//-- point added array node.
	JsonPtr pushDict();			//-- point added dict node.

**返回值**

* **bool**

	如果当前节点不是数组类型，将返回 false，否则返回 true。

* **JsonPtr**

	如果当前节点不是数组类型，将返回 nullptr，否则返回新添加的数组节点指针，或者对象节点指针/字典节点指针。

###### 高级操作

* 向当前节点添加任意数量数据

		template<typename... Args>
		bool fill(Args... args);

* 向当前数组节点中加入一个**新**的**子节点**，然后复制 STL 容器对象中的数据到该子节点中

		template<class... Args>
		bool push(const std::tuple<Args...>& tup);

		template < class T, size_t N >
		bool push(const std::array<T, N>& arr);

		template < class T, class Alloc = std::allocator<T> >
		bool push(const std::deque<T, Alloc>& deq);

		template <template <typename, typename> class Container, typename T, class Alloc = std::allocator<T>>
		bool push(const Container<T, Alloc>& container);

		template < class T, class Compare = std::less<T>, class Alloc = std::allocator<T> >
		bool push(const std::set<T, Compare, Alloc>& collections);

		template < class Key, class Hash = std::hash<Key>, class Pred = std::equal_to<Key>, class Alloc = std::allocator<Key> >
		bool push(const std::unordered_set<Key, Hash, Pred, Alloc>& collections);

		template <class T, class Compare = std::less<std::string>, class Alloc = std::allocator<std::pair<const std::string, T> > >
		bool push(const std::map<std::string, T, Compare, Alloc>& theMap);

		template <class T, class Hash = std::hash<std::string>, class Pred = std::equal_to<std::string>, class Alloc = std::allocator<std::pair<const std::string, T> > >
		bool push(const std::unordered_map<std::string, T, Hash, Pred, Alloc>& theMap);

	**注意**

	如果当前节点不能转换为数组类型，则函数将抛出 [FPNN 异常][FPNNError]。

* 将 STL 容器对象中的数据添加到当前节点中

		template<class... Args>
		bool merge(const std::tuple<Args...>& tup);

		template < class T, size_t N >
		bool merge(const std::array<T, N>& arr);

		template < class T, class Alloc = std::allocator<T> >
		bool merge(const std::deque<T, Alloc>& deq);

		template <template <typename, typename> class Container, typename T, class Alloc = std::allocator<T>>
		bool merge(const Container<T, Alloc>& container);

		template <class Alloc = std::allocator<bool>>
		bool merge(const std::vector<bool, Alloc>& container);

		template < class T, class Compare = std::less<T>, class Alloc = std::allocator<T> >
		bool merge(const std::set<T, Compare, Alloc>& collections);

		template < class Key, class Hash = std::hash<Key>, class Pred = std::equal_to<Key>, class Alloc = std::allocator<Key> >
		bool merge(const std::unordered_set<Key, Hash, Pred, Alloc>& collections);

	**注意**

	如果当前节点不能转换为数组类型，则函数将抛出 [FPNN 异常][FPNNError]。

##### 向指定子级节点，添加数据

	void push(const std::string& path, bool value, const std::string& delim = "./");
	void push(const std::string& path, double value, const std::string& delim = "./");
	void push(const std::string& path, long double value, const std::string& delim = "./");
	void push(const std::string& path, const char* value, const std::string& delim = "./");
	void push(const std::string& path, const std::string& value, const std::string& delim = "./");

	void push(const std::string& path, short value, const std::string& delim = "./");
	void push(const std::string& path, unsigned short value, const std::string& delim = "./");
	void push(const std::string& path, int value, const std::string& delim = "./");
	void push(const std::string& path, unsigned int value, const std::string& delim = "./");
	void push(const std::string& path, long value, const std::string& delim = "./");
	void push(const std::string& path, unsigned long value, const std::string& delim = "./");
	void push(const std::string& path, long long value, const std::string& delim = "./");
	void push(const std::string& path, unsigned long long value, const std::string& delim = "./");

	void pushNull(const std::string& path, const std::string& delim = "./");
	void pushBool(const std::string& path, bool value, const std::string& delim = "./");
	void pushReal(const std::string& path, double value, const std::string& delim = "./");
	void pushInt(const std::string& path, intmax_t value, const std::string& delim = "./");
	void pushUInt(const std::string& path, uintmax_t value, const std::string& delim = "./");
	void pushString(const std::string& path, const char* value, const std::string& delim = "./");
	void pushString(const std::string& path, const std::string& value, const std::string& delim = "./");
	JsonPtr pushArray(const std::string& path, const std::string& delim = "./");			//-- point pushed array node.
	JsonPtr pushDict(const std::string& path, const std::string& delim = "./");				//-- point pushed dict node.

**注意**

* 如果从当前节点到指定的子级节点的路径上，中间层级节点，或最终子节点不存在，函数将会自动创建。其中中间级节点均为对象/字典类型。
* 如果从当前节点到指定的子级节点的路径上，中间层级节点不为对象/字典类型，函数将抛出 [FPNN 异常][FPNNError]。

**参数说明**

* **`const std::string& path`**

	从当前节点到最终子级节点的路径。

* **`const std::string& delim`**

	从当前节点到最终子级节点的路径的分隔符集合。默认 "." 和 "/" 均被视为路径分隔符。

**返回值**

* **JsonPtr**

	如果当前节点不是数组类型，将返回 nullptr，否则返回新添加的数组节点指针，或者对象节点指针/字典节点指针。

###### 高级操作

*	向指定子级节点添加任意数量数据

		template<typename... Args>
		void fillTo(const std::string& path, Args... args);

	**注意**

	如果指定子级节点不能转换为数组类型，则函数将抛出 [FPNN 异常][FPNNError]。

* 向指定子级数组节点中加入一个**新**的**子节点**，然后复制 STL 容器对象中的数据到该子节点中

		template<class... Args>
		bool push(const std::string& path, const std::tuple<Args...>& tup, const std::string& delim = "./");

		template < class T, size_t N >
		bool push(const std::string& path, const std::array<T, N>& arr, const std::string& delim = "./");

		template < class T, class Alloc = std::allocator<T> >
		bool push(const std::string& path, const std::deque<T, Alloc>& deq, const std::string& delim = "./");

		template <template <typename, typename> class Container, typename T, class Alloc = std::allocator<T>>
		bool push(const std::string& path, const Container<T, Alloc>& container, const std::string& delim = "./");

		template < class T, class Compare = std::less<T>, class Alloc = std::allocator<T> >
		bool push(const std::string& path, const std::set<T, Compare, Alloc>& collections, const std::string& delim = "./");

		template < class Key, class Hash = std::hash<Key>, class Pred = std::equal_to<Key>, class Alloc = std::allocator<Key> >
		bool push(const std::string& path, const std::unordered_set<Key, Hash, Pred, Alloc>& collections, const std::string& delim = "./");

		template <class T, class Compare = std::less<std::string>, class Alloc = std::allocator<std::pair<const std::string, T> > >
		bool push(const std::string& path, const std::map<std::string, T, Compare, Alloc>& theMap, const std::string& delim = "./");

		template <class T, class Hash = std::hash<std::string>, class Pred = std::equal_to<std::string>, class Alloc = std::allocator<std::pair<const std::string, T> > >
		bool push(const std::string& path, const std::unordered_map<std::string, T, Hash, Pred, Alloc>& theMap, const std::string& delim = "./");


	**注意**

	如果指定子级节点不能转换为数组类型，则函数将抛出 [FPNN 异常][FPNNError]。

* 将 STL 容器对象中的数据添加到指定子级数组节点中

		template<class... Args>
		void merge(const std::string& path, const std::tuple<Args...>& tup, const std::string& delim = "./");

		template < class T, size_t N >
		void merge(const std::string& path, const std::array<T, N>& arr, const std::string& delim = "./");

		template < class T, class Alloc = std::allocator<T> >
		void merge(const std::string& path, const std::deque<T, Alloc>& deq, const std::string& delim = "./");

		template <template <typename, typename> class Container, typename T, class Alloc = std::allocator<T>>
		void merge(const std::string& path, const Container<T, Alloc>& container, const std::string& delim = "./");

		template < class T, class Compare = std::less<T>, class Alloc = std::allocator<T> >
		void merge(const std::string& path, const std::set<T, Compare, Alloc>& collections, const std::string& delim = "./");

		template < class Key, class Hash = std::hash<Key>, class Pred = std::equal_to<Key>, class Alloc = std::allocator<Key> >
		void merge(const std::string& path, const std::unordered_set<Key, Hash, Pred, Alloc>& collections, const std::string& delim = "./");


	**注意**

	如果当前节点不能转换为数组类型，则函数将抛出 [FPNN 异常][FPNNError]。

#### 向对象节点字典节点中添加数据


	void add(const std::string& path, bool value, const std::string& delim = "./");
	void add(const std::string& path, double value, const std::string& delim = "./");
	void add(const std::string& path, long double value, const std::string& delim = "./");
	void add(const std::string& path, const char* value, const std::string& delim = "./");
	void add(const std::string& path, const std::string& value, const std::string& delim = "./");

	void add(const std::string& path, short value, const std::string& delim = "./");
	void add(const std::string& path, unsigned short value, const std::string& delim = "./");
	void add(const std::string& path, int value, const std::string& delim = "./");
	void add(const std::string& path, unsigned int value, const std::string& delim = "./");
	void add(const std::string& path, long value, const std::string& delim = "./");
	void add(const std::string& path, unsigned long value, const std::string& delim = "./");
	void add(const std::string& path, long long value, const std::string& delim = "./");
	void add(const std::string& path, unsigned long long value, const std::string& delim = "./");

	void addBool(const std::string& path, bool value, const std::string& delim = "./");
	void addReal(const std::string& path, double value, const std::string& delim = "./");
	void addInt(const std::string& path, intmax_t value, const std::string& delim = "./");
	void addUInt(const std::string& path, uintmax_t value, const std::string& delim = "./");
	void addString(const std::string& path, const char* value, const std::string& delim = "./");
	void addString(const std::string& path, const std::string& value, const std::string& delim = "./");

	void addNull(const std::string& path, const std::string& delim = "./");
	JsonPtr addArray(const std::string& path, const std::string& delim = "./");
	JsonPtr addDict(const std::string& path, const std::string& delim = "./");

**注意**

* 如果从当前节点到指定的子级节点的路径上，中间层级节点，或最终子节点不存在，函数将会自动创建。其中中间级节点均为对象/字典类型。
* 如果从当前节点到指定的子级节点的路径上，中间层级节点不为对象/字典类型，函数将抛出 [FPNN 异常][FPNNError]。
* 如果最终子节点已经存在，函数将抛出 [FPNN 异常][FPNNError]。

**参数说明**

* **`const std::string& path`**

	从当前节点到最终子级节点的路径。

* **`const std::string& delim`**

	从当前节点到最终子级节点的路径的分隔符集合。默认 "." 和 "/" 均被视为路径分隔符。

**返回值**

* **JsonPtr**

	如果当前节点不是数组类型，将返回 nullptr，否则返回新添加的数组节点指针，或者对象节点指针/字典节点指针。

###### 高级操作

* 将 STL 容器对象中的数据添加到当前节点中

		template <class T, class Compare = std::less<std::string>, class Alloc = std::allocator<std::pair<const std::string, T> > >
		bool merge(const std::map<std::string, T, Compare, Alloc>& theMap);

		template <class T, class Hash = std::hash<std::string>, class Pred = std::equal_to<std::string>, class Alloc = std::allocator<std::pair<const std::string, T> > >
		bool merge(const std::unordered_map<std::string, T, Hash, Pred, Alloc>& theMap);

	**注意**

	如果当前节点不能转换为对象/字典类型，则函数将抛出 [FPNN 异常][FPNNError]。


* 向指定子级对象/字典节点中加入一个**新**的**子节点**，然后复制 STL 容器对象中的数据到该子节点中


		template<class... Args>
		bool add(const std::string& path, const std::tuple<Args...>& tup, const std::string& delim = "./");

		template < class T, size_t N >
		bool add(const std::string& path, const std::array<T, N>& arr, const std::string& delim = "./");

		template < class T, class Alloc = std::allocator<T> >
		bool add(const std::string& path, const std::deque<T, Alloc>& deq, const std::string& delim = "./");

		template <template <typename, typename> class Container, typename T, class Alloc = std::allocator<T>>
		bool add(const std::string& path, const Container<T, Alloc>& container, const std::string& delim = "./");

		template < class T, class Compare = std::less<T>, class Alloc = std::allocator<T> >
		bool add(const std::string& path, const std::set<T, Compare, Alloc>& collections, const std::string& delim = "./");

		template < class Key, class Hash = std::hash<Key>, class Pred = std::equal_to<Key>, class Alloc = std::allocator<Key> >
		bool add(const std::string& path, const std::unordered_set<Key, Hash, Pred, Alloc>& collections, const std::string& delim = "./");

		template <class T, class Compare = std::less<std::string>, class Alloc = std::allocator<std::pair<const std::string, T> > >
		bool add(const std::string& path, const std::map<std::string, T, Compare, Alloc>& theMap, const std::string& delim = "./");

		template <class T, class Hash = std::hash<std::string>, class Pred = std::equal_to<std::string>, class Alloc = std::allocator<std::pair<const std::string, T> > >
		bool add(const std::string& path, const std::unordered_map<std::string, T, Hash, Pred, Alloc>& theMap, const std::string& delim = "./");


	**注意**

	如果当前节点不能转换为对象/字典类型，则函数将抛出 [FPNN 异常][FPNNError]。

* 将 STL 容器对象中的数据添加到指定子级对象/字典节点中


		template <class T, class Compare = std::less<std::string>, class Alloc = std::allocator<std::pair<const std::string, T> > >
		void merge(const std::string& path, const std::map<std::string, T, Compare, Alloc>& theMap, const std::string& delim = "./");

		template <class T, class Hash = std::hash<std::string>, class Pred = std::equal_to<std::string>, class Alloc = std::allocator<std::pair<const std::string, T> > >
		void merge(const std::string& path, const std::unordered_map<std::string, T, Hash, Pred, Alloc>& theMap, const std::string& delim = "./");

	**注意**

	如果当前节点不能转换为对象/字典类型，则函数将抛出 [FPNN 异常][FPNNError]。


### 删除数据

#### 删除数组元素

删除索引值为 index 的元素。

	bool remove(int index);

**返回值说明**

如果当前节点不是数组类型，将返回 false，其他情况返回 true。如果超出数组长度，也将返回 true。


#### 删除字典成员


	bool remove(const std::string& path, const std::string& delim = "./");

**参数说明**

* **`const std::string& path`**

	从当前节点到最终子级节点的路径。

* **`const std::string& delim`**

	从当前节点到最终子级节点的路径的分隔符集合。默认 "." 和 "/" 均被视为路径分隔符。

**返回值**

* 如果从当前节点到指定的子级节点的路径上，中间层级节点，或最终子节点不存在，函数将返回 false。
* 如果从当前节点到指定的子级节点的路径上，中间层级节点不为对象/字典类型，函数将返回 false。

### 判断节点数据类型

获取或者判断节点类型。

	inline bool isNull() const;
	inline enum ElementType type() const;

	//-- isNull(): return true only the target exist and which is null.
	bool isNull(const std::string& path, const std::string& delim = "./") noexcept;
	enum ElementType type(const std::string& path, const std::string& delim = "./");

**参数说明**

* **`const std::string& path`**

	从当前节点到最终子级节点的路径。

* **`const std::string& delim`**

	从当前节点到最终子级节点的路径的分隔符集合。默认 "." 和 "/" 均被视为路径分隔符。


### 判断节点是否存在

	bool exist(const std::string& path, const std::string& delim = "./") noexcept;

**参数说明**

* **`const std::string& path`**

	从当前节点到最终子级节点的路径。

* **`const std::string& delim`**

	从当前节点到最终子级节点的路径的分隔符集合。默认 "." 和 "/" 均被视为路径分隔符。


### 获取节点

* 如果当前节点为数组类型，获取当前节点所有子节点：

		const std::list<JsonPtr> * const getList() const;

* 如果当前节点为对象/字典类型，获取当前节点所有子节点：

		const std::map<std::string, JsonPtr> * const getDict() const;

* 如果指定子节点为数组类型，获取该节点所有子节点：

		const std::list<JsonPtr> * const getList(const std::string& path, const std::string& delim = "./");
		const std::list<JsonPtr> * const getList(const char* path, const std::string& delim = "./");

* 如果指定节点为对象/字典类型，获取该节点所有子节点：

		const std::map<std::string, JsonPtr> * const getDict(const std::string& path, const std::string& delim = "./");
		const std::map<std::string, JsonPtr> * const getDict(const char* path, const std::string& delim = "./");

* 获取指定节点：

		JsonPtr getNode(const std::string& path, const std::string& delim = "./");
		JsonPtr getNode(const char* path, const std::string& delim = "./");

	对于目标节点是对象节点/字典节点成员：

		Json& operator [] (const char* path);
		Json& operator [] (const std::string& path);

	如果当前节点是数组类型，获取指定下标节点：

		Json& operator [] (int index);

	**注意**

	下标运算符重载版本，如果 path 指定路径上，任意中间节点不是对象/字典类型，函数将抛出 [FPNN 异常][FPNNError]。

**参数说明**

* **`const std::string& path`**

	从当前节点到最终子级节点的路径。

* **`const std::string& delim`**

	从当前节点到最终子级节点的路径的分隔符集合。默认 "." 和 "/" 均被视为路径分隔符。

### 获取数据

**注意**

获取数据分为 get 系列，和 want 系列。  
get 系列，如果目标节点不存在，或者类型不匹配，则返回替代值。  
want 系列，如果目标节点不存在，或者类型不匹配，则抛出 [FPNN 异常][FPNNError]。

#### 基础类型

**get 系列**

* **操作当前节点**

		bool getBool(bool dft = false) const;
		intmax_t getInt(intmax_t dft = 0) const;
		uintmax_t getUInt(uintmax_t dft = 0) const;
		double getReal(double dft = 0.0) const;
		std::string getString(const std::string& dft = std::string()) const;

	**注意**

	第二个参数 `dft` 即为替代值。

* **操作指定节点**

		bool getBool(const std::string& path, bool dft = false, const std::string& delim = "./");
		intmax_t getInt(const std::string& path, intmax_t dft = 0, const std::string& delim = "./");
		uintmax_t getUInt(const std::string& path, uintmax_t dft = 0, const std::string& delim = "./");
		double getReal(const std::string& path, double dft = 0.0, const std::string& delim = "./");
		std::string getStringAt(const std::string& path, const std::string& dft = std::string(), const std::string& delim = "./");

		bool getBool(const char* path, bool dft = false, const std::string& delim = "./");
		intmax_t getInt(const char* path, intmax_t dft = 0, const std::string& delim = "./");
		uintmax_t getUInt(const char* path, uintmax_t dft = 0, const std::string& delim = "./");
		double getReal(const char* path, double dft = 0.0, const std::string& delim = "./");
		std::string getStringAt(const char* path, const std::string& dft = std::string(), const std::string& delim = "./");

	**参数说明**

	* **`const std::string& path`**

		从当前节点到最终子级节点的路径。

	* **`dft`**

		第二个参数 `dft` 即为替代值。

	* **`const std::string& delim`**

		从当前节点到最终子级节点的路径的分隔符集合。默认 "." 和 "/" 均被视为路径分隔符。

**want 系列**

* **操作当前节点**

		bool wantBool() const;
		intmax_t wantInt() const;
		uintmax_t wantUInt() const;
		double wantReal() const;
		std::string wantString() const;

* **操作指定节点**

		bool wantBool(const std::string& path, const std::string& delim = "./");
		intmax_t wantInt(const std::string& path, const std::string& delim = "./");
		uintmax_t wantUInt(const std::string& path, const std::string& delim = "./");
		double wantReal(const std::string& path, const std::string& delim = "./");
		std::string wantString(const std::string& path, const std::string& delim = "./");
		std::string wantStringAt(const std::string& path, const std::string& delim = "./");

		bool wantBool(const char* path, const std::string& delim = "./")
		intmax_t wantInt(const char* path, const std::string& delim = "./")
		uintmax_t wantUInt(const char* path, const std::string& delim = "./")
		double wantReal(const char* path, const std::string& delim = "./")
		std::string wantString(const char* path, const std::string& delim = "./")
		std::string wantStringAt(const char* path, const std::string& delim = "./");

	**参数说明**

	* **`const std::string& path`**

		从当前节点到最终子级节点的路径。

	* **`const std::string& delim`**

		从当前节点到最终子级节点的路径的分隔符集合。默认 "." 和 "/" 均被视为路径分隔符。



#### 类型转换

**注意**

类型转换属于 want 系列操作，如果目标节点不存在，或者类型不匹配，则抛出 [FPNN 异常][FPNNError]。

	operator bool()        const;
	operator float()       const;
	operator double()      const;
	operator long double() const;
	operator std::string() const;

	operator char()               const;
	operator unsigned char()      const;
	operator short()              const;
	operator unsigned short()     const;
	operator int()                const;
	operator unsigned int()       const;
	operator long()               const;
	operator unsigned long()      const;
	operator long long()          const;
	operator unsigned long long() const;


#### 容器类型

**get 系列**

* **操作当前节点**

		template<class... Args>
		std::tuple<Args...> getTuple(std::tuple<Args...>& tup, bool compatibleMode = false);

		template < class T, size_t N >
		std::array<T, N> getArray(const std::array<T, N>& dft = std::array<T, N>(), bool compatibleMode = false);

		template <class T, class Container = std::deque<T> >
		std::queue<T, Container> getQueue(const std::queue<T, Container>& dft = std::queue<T, Container>());

		template < class T, class Alloc = std::allocator<T> >
		std::deque<T, Alloc> getDeque(const std::deque<T, Alloc>& dft = std::deque<T, Alloc>());

		template < class T, class Alloc = std::allocator<T> >
		std::list<T, Alloc> getList(const std::list<T, Alloc>& dft = std::list<T, Alloc>());

		template < class T, class Alloc = std::allocator<T> >
		std::vector<T, Alloc> getVector(const std::vector<T, Alloc>& dft = std::vector<T, Alloc>());

		template < class T, class Compare = std::less<T>, class Alloc = std::allocator<T> >
		std::set<T, Compare, Alloc> getSet(const std::set<T, Compare, Alloc>& dft = std::set<T, Compare, Alloc>());

		template < class Key, class Hash = std::hash<Key>, class Pred = std::equal_to<Key>, class Alloc = std::allocator<Key> >
		std::unordered_set<Key, Hash, Pred, Alloc> getUnorderedSet(const std::unordered_set<Key, Hash, Pred, Alloc>& dft = std::unordered_set<Key, Hash, Pred, Alloc>());

		template <class T, class Compare = std::less<std::string>, class Alloc = std::allocator<std::pair<const std::string, T> > >
		std::map<std::string, T, Compare, Alloc> getDict(const std::map<std::string, T, Compare, Alloc>& dft = std::map<std::string, T, Compare, Alloc>());

		template <class T, class Hash = std::hash<std::string>, class Pred = std::equal_to<std::string>, class Alloc = std::allocator<std::pair<const std::string, T> > >
		std::unordered_map<std::string, T, Hash, Pred, Alloc> getUnorderedDict(const std::unordered_map<std::string, T, Hash, Pred, Alloc>& dft = std::unordered_map<std::string, T, Hash, Pred, Alloc>());


	**参数说明**

	* **`bool compatibleMode`**

		对于获取 tuple 或 array 类型数据，是否启用兼容模式。  
		如果关闭兼容模式，则要求 tuple 长度和实际数据长度必须完全相同。  
		如果开启兼容模式，且数据实际长度大于 tuple 长度，则多余的数据被舍弃。如果小于，则未填充数据取决于 std::tuple 和 std::array 的初始化行为。

		默认关闭兼容模式。

	* **`dft`**

		第二个参数 `dft` 即为替代值。

* **操作指定节点**

		template<class... Args>
		std::tuple<Args...> getTuple(const std::string& path, std::tuple<Args...>& tup, bool compatibleMode = false, const std::string& delim = "./");

		template < class T, size_t N >
		std::array<T, N> getArray(const std::string& path, const std::array<T, N>& dft = std::array<T, N>(), bool compatibleMode = false, const std::string& delim = "./");

		template <class T, class Container = std::deque<T> >
		std::queue<T, Container> getQueue(const std::string& path, const std::queue<T, Container>& dft = std::queue<T, Container>(), const std::string& delim = "./");

		template < class T, class Alloc = std::allocator<T> >
		std::deque<T, Alloc> getDeque(const std::string& path, const std::deque<T, Alloc>& dft = std::deque<T, Alloc>(), const std::string& delim = "./");

		template < class T, class Alloc = std::allocator<T> >
		std::list<T, Alloc> getList(const std::string& path, const std::list<T, Alloc>& dft = std::list<T, Alloc>(), const std::string& delim = "./");

		template < class T, class Alloc = std::allocator<T> >
		std::vector<T, Alloc> getVector(const std::string& path, const std::vector<T, Alloc>& dft = std::vector<T, Alloc>(), const std::string& delim = "./");

		template < class T, class Compare = std::less<T>, class Alloc = std::allocator<T> >
		std::set<T, Compare, Alloc> getSet(const std::string& path, const std::set<T, Compare, Alloc>& dft = std::set<T, Compare, Alloc>(), const std::string& delim = "./");

		template < class Key, class Hash = std::hash<Key>, class Pred = std::equal_to<Key>, class Alloc = std::allocator<Key> >
		std::unordered_set<Key, Hash, Pred, Alloc> getUnorderedSet(const std::string& path, const std::unordered_set<Key, Hash, Pred, Alloc>& dft = std::unordered_set<Key, Hash, Pred, Alloc>(), const std::string& delim = "./");

		template <class T, class Compare = std::less<std::string>, class Alloc = std::allocator<std::pair<const std::string, T> > >
		std::map<std::string, T, Compare, Alloc> getDict(const std::string& path, const std::map<std::string, T, Compare, Alloc>& dft = std::map<std::string, T, Compare, Alloc>(), const std::string& delim = "./");

		template <class T, class Hash = std::hash<std::string>, class Pred = std::equal_to<std::string>, class Alloc = std::allocator<std::pair<const std::string, T> > >
		std::unordered_map<std::string, T, Hash, Pred, Alloc> getUnorderedDict(const std::string& path, const std::unordered_map<std::string, T, Hash, Pred, Alloc>& dft = std::unordered_map<std::string, T, Hash, Pred, Alloc>(), const std::string& delim = "./");

	**参数说明**

	* **`bool compatibleMode`**

		对于获取 tuple 或 array 类型数据，是否启用兼容模式。  
		如果关闭兼容模式，则要求 tuple 长度和实际数据长度必须完全相同。  
		如果开启兼容模式，且数据实际长度大于 tuple 长度，则多余的数据被舍弃。如果小于，则未填充数据取决于 std::tuple 和 std::array 的初始化行为。

		默认关闭兼容模式。

	* **`dft`**

		第二个参数 `dft` 即为替代值。

	* **`const std::string& path`**

		从当前节点到最终子级节点的路径。

	* **`const std::string& delim`**

		从当前节点到最终子级节点的路径的分隔符集合。默认 "." 和 "/" 均被视为路径分隔符。


**want 系列**

* **操作当前节点**

		template<class... Args>
		std::tuple<Args...> wantTuple(std::tuple<Args...>& tup, bool compatibleMode = false);

		template < class T, size_t N >
		std::array<T, N> wantArray(bool compatibleMode = false);

		template <class T, class Container = std::deque<T> >
		std::queue<T, Container> wantQueue();

		template < class T, class Alloc = std::allocator<T> >
		std::deque<T, Alloc> wantDeque();

		template < class T, class Alloc = std::allocator<T> >
		std::list<T, Alloc> wantList();

		template < class T, class Alloc = std::allocator<T> >
		std::vector<T, Alloc> wantVector();

		template < class T, class Compare = std::less<T>, class Alloc = std::allocator<T> >
		std::set<T, Compare, Alloc> wantSet();

		template < class Key, class Hash = std::hash<Key>, class Pred = std::equal_to<Key>, class Alloc = std::allocator<Key> >
		std::unordered_set<Key, Hash, Pred, Alloc> wantUnorderedSet();


		template <class T, class Compare = std::less<std::string>, class Alloc = std::allocator<std::pair<const std::string, T> > >
		std::map<std::string, T, Compare, Alloc> wantDict();

		template <class T, class Hash = std::hash<std::string>, class Pred = std::equal_to<std::string>, class Alloc = std::allocator<std::pair<const std::string, T> > >
		std::unordered_map<std::string, T, Hash, Pred, Alloc> wantUnorderedDict();

	**参数说明**

	* **`bool compatibleMode`**

		对于获取 tuple 或 array 类型数据，是否启用兼容模式。  
		如果关闭兼容模式，则要求 tuple 长度和实际数据长度必须完全相同。  
		如果开启兼容模式，且数据实际长度大于 tuple 长度，则多余的数据被舍弃。如果小于，则未填充数据取决于 std::tuple 和 std::array 的初始化行为。

		默认关闭兼容模式。

* **操作指定节点**

		template<class... Args>
		std::tuple<Args...> wantTuple(const std::string& path, std::tuple<Args...>& tup, bool compatibleMode = false, const std::string& delim = "./");

		template < class T, size_t N >
		std::array<T, N> wantArray(const std::string& path, bool compatibleMode = false, const std::string& delim = "./");

		template <class T, class Container = std::deque<T> >
		std::queue<T, Container> wantQueue(const std::string& path, const std::string& delim = "./");

		template < class T, class Alloc = std::allocator<T> >
		std::deque<T, Alloc> wantDeque(const std::string& path, const std::string& delim = "./");

		template < class T, class Alloc = std::allocator<T> >
		std::list<T, Alloc> wantList(const std::string& path, const std::string& delim = "./");

		template < class T, class Alloc = std::allocator<T> >
		std::vector<T, Alloc> wantVector(const std::string& path, const std::string& delim = "./");

		template < class T, class Compare = std::less<T>, class Alloc = std::allocator<T> >
		std::set<T, Compare, Alloc> wantSet(const std::string& path, const std::string& delim = "./");

		template < class Key, class Hash = std::hash<Key>, class Pred = std::equal_to<Key>, class Alloc = std::allocator<Key> >
		std::unordered_set<Key, Hash, Pred, Alloc> wantUnorderedSet(const std::string& path, const std::string& delim = "./");

		template <class T, class Compare = std::less<std::string>, class Alloc = std::allocator<std::pair<const std::string, T> > >
		std::map<std::string, T, Compare, Alloc> wantDict(const std::string& path, const std::string& delim = "./");

		template <class T, class Hash = std::hash<std::string>, class Pred = std::equal_to<std::string>, class Alloc = std::allocator<std::pair<const std::string, T> > >
		std::unordered_map<std::string, T, Hash, Pred, Alloc> wantUnorderedDict(const std::string& path, const std::string& delim = "./");


	**参数说明**

	* **`bool compatibleMode`**

		对于获取 tuple 或 array 类型数据，是否启用兼容模式。  
		如果关闭兼容模式，则要求 tuple 长度和实际数据长度必须完全相同。  
		如果开启兼容模式，且数据实际长度大于 tuple 长度，则多余的数据被舍弃。如果小于，则未填充数据取决于 std::tuple 和 std::array 的初始化行为。

		默认关闭兼容模式。

	* **`const std::string& path`**

		从当前节点到最终子级节点的路径。

	* **`const std::string& delim`**

		从当前节点到最终子级节点的路径的分隔符集合。默认 "." 和 "/" 均被视为路径分隔符。



[FPNNError]: FpnnError.md
