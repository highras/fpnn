## HttpClient

### 介绍

简单的 HTTP 客户端实现。

### 命名空间

	namespace fpnn::HttpClient;

### 全局函数

#### Post

	int Post(const std::string& url, const std::string& data, const std::vector<std::string>& header, std::string& resp, int timeout = HTTP_CLIENT_DEFAULT_TIMEOUT);

发送 Post 请求。

**参数说明**

* **`const std::string& url`**

	访问的地址。

* **`const std::string& data`**

	Post 数据。

* **`const std::vector<std::string>& header`**

	HTTP header 内容。

* **`std::string& resp`**

	返回的数据。

* **`int timeout = HTTP_CLIENT_DEFAULT_TIMEOUT`**

	请求超时。HTTP_CLIENT_DEFAULT_TIMEOUT 为 5 秒。

**返回值**

+ -1: libcurl 初始化失败。
+ -2: libcurl 执行失败。
+ 0: 执行成功，且 HTTP 返回 200。
+ 其他数值：HTTP 返回代码。

#### Get

	int Get(const std::string& url, const std::vector<std::string>& header, std::string& resp, int timeout = HTTP_CLIENT_DEFAULT_TIMEOUT);

发送 Get 请求。

**参数说明**

* **`const std::string& url`**

	访问的地址。

* **`const std::string& data`**

	Post 数据。

* **`const std::vector<std::string>& header`**

	HTTP header 内容。

* **`std::string& resp`**

	返回的数据。

* **`int timeout = HTTP_CLIENT_DEFAULT_TIMEOUT`**

	请求超时。HTTP_CLIENT_DEFAULT_TIMEOUT 为 5 秒。

**返回值**

+ -1: libcurl 初始化失败。
+ -2: libcurl 执行失败。
+ 0: 执行成功，且 HTTP 返回 200。
+ 其他数值：HTTP 返回代码。

#### uriEncode

	std::string uriEncode(const std::string& uri);

URI 编码。

#### uriDecode

	std::string uriDecode(const std::string& uri);

URI 解码。

#### uriDecode2

	std::string uriDecode2(const std::string& uri);

URI 解码。