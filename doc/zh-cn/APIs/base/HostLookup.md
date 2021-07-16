## HostLookup

### 介绍

域名查询处理模块。

**注意**

域名解析缓存功能已经取消。

### 命名空间

	namespace fpnn;

### 关键定义

	class HostLookup 
	{
	public:
		static std::string get(const std::string& host);
		static std::string add(const std::string& host);
	};

#### get

	static std::string get(const std::string& host);

域名查询。如果缓存功能启用，则优先获取缓存信息。如果获取不到，再做域名查询。

**注意**

域名解析缓存功能已经取消，目前 get 函数完全等价于 add 函数。

#### add

	static std::string add(const std::string& host);

域名查询。如果缓存功能启用，则将解析后的域名信息加入缓存。

**注意**

域名解析缓存功能已经取消，目前 add 函数完全等价于 get 函数。