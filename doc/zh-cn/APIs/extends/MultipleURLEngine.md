## MultipleURLEngine

### 介绍

多路 URL 并发访问引擎。

用于大量 URL 的 HTTP 1.0/1.1 密集访问，亦可用于生成 HTTP 压力源。

### 命名空间

	namespace fpnn;

### VisitStateCode

	class MultipleURLEngine
	{
	public:
		enum VisitStateCode
		{
			VISIT_OK = 0,
			VISIT_READY,
			VISIT_NOT_EXECUTED,
			VISIT_EXPIRED,
			VISIT_TERMINATED,
			VISIT_ADD_TO_ENGINE_FAILED,
			VISIT_UNKNOWN_ERROR
		};
	};

URL 访问状态枚举。

| 枚举值 | 含义 |
|-------|-----|
| VISIT_OK | 执行完成 |
| VISIT_READY | 就绪，待执行 |
| VISIT_NOT_EXECUTED | 未被执行 |
| VISIT_EXPIRED | 操作超时 |
| VISIT_TERMINATED | 操作被终止 |
| VISIT_ADD_TO_ENGINE_FAILED | 添加到引擎失败 |
| VISIT_UNKNOWN_ERROR | 未知错误 |

### CURLPtr

	typedef std::unique_ptr<CURL, void(*)(CURL*)> CURLPtr;

curl unique_ptr 指针。函数为 curl 对应的销毁函数。

### Result

	class MultipleURLEngine
	{
	public:
		struct Result
		{
			CURLPtr curlHandle;
			int64_t startTime;	//-- msec
			int responseCode;
			CURLcode curlCode;
			enum VisitStateCode visitState;
			char* errorInfo;
			std::shared_ptr<ChainBuffer> responseBuffer;

			Result();
			~Result();
		};
	};

HTTP 访问结果。

**成员说明**

* **`CURLPtr curlHandle`**

	curl 封装后的 CURL 对象。请参见 [CURLPtr](#CURLPtr)。

	当访问完成后，CURL 对象不会立即销毁，而会被包装入 curlHandle 中，以便后续访问继续使用。

	如后续不再使用，可以直接忽略该成员，curlHandle 内的 CURL 对象将被安全销毁。


* **`int64_t startTime`**

	开始访问的时间。UTC，单位：毫秒。

* **`int responseCode`**

	返回的 HTTP 状态码。

* **`CURLcode curlCode`**

	返回的 CURL 状态码。枚举值请参见 [CURLcode](https://curl.se/libcurl/c/libcurl-errors.html#CURLcode)。

* **`enum VisitStateCode visitState`**

	并发访问引擎返回的状态码，请参见 [VisitStateCode](#VisitStateCode)。

* **`char* errorInfo`**

	本次访问的错误信息（如果有）。

* **`std::shared_ptr<ChainBuffer> responseBuffer`**

	返回的数据。ChainBuffer 请参见 [ChainBuffer](../base/ChainBuffer.md)。

### ResultCallback

	class MultipleURLEngine
	{
	public:
		class ResultCallback
		{
		public:
			ResultCallback();
			virtual ~ResultCallback();

			virtual void onCompleted(Result &result) = 0;
			virtual void onExpired(CURLPtr curl_unique_ptr) = 0;
			virtual void onTerminated(CURLPtr curl_unique_ptr) = 0;
			virtual void onException(CURLPtr curl_unique_ptr, enum VisitStateCode errorCode, const char *errorInfo) = 0;
		};
		typedef std::shared_ptr<ResultCallback> ResultCallbackPtr;
	};

异步访问回调对象。

#### 成员函数

##### onCompleted

	virtual void onCompleted(Result &result);

当访问正常完成时，触发该接口。Result 为访问的结果和返回的数据，请参见 [Result](#Result)。

##### onExpired

	virtual void onExpired(CURLPtr curl_unique_ptr);

当访问超时时，触发该接口。CURLPtr 请参见 [CURLPtr](#CURLPtr)。

##### onTerminated

	virtual void onTerminated(CURLPtr curl_unique_ptr);

当访问被终止时，触发该接口。CURLPtr 请参见 [CURLPtr](#CURLPtr)。

##### onException

	virtual void onException(CURLPtr curl_unique_ptr, enum VisitStateCode errorCode, const char *errorInfo);

当访问发生异常时，触发该接口。

**参数说明**

* **`CURLPtr curl_unique_ptr`**

	本次访问使用的 curl 的 CURL 对象，请参见 [CURLPtr](#CURLPtr)。

* **`enum VisitStateCode errorCode`**

	并发访问引擎返回的状态码，请参见 [VisitStateCode](#VisitStateCode)。

* **`const char *errorInfo`**

	本次访问的错误信息（如果有）。

### StatusInfo

	class MultipleURLEngine
	{
	public:
		struct StatusInfo
		{
			struct ConfigInfo
			{
				int connsInPerThread;		//-- 每个线程最大任务并发数。
				int perfectThreadCount;		//-- 空闲时，保留线程限制
				int maxThreadCount;			//-- 最大线程数
				int maxConcurrentCount;		//-- 并发总上限
			} configInfo;
			
			struct CurrentInfo
			{
				int64_t threads;	//-- 当前线程数
				int64_t loads;		//-- 当前总任务数
				int64_t connections;	//-- 当前已建立连接的数目
				int64_t queuedTaskCount;	//-- 当前 queued 住的任务数量
			} currentInfo;
		};
	};

并发访问引擎状态结构。

成员说明请参见注释。

### MultipleURLEngine

	class MultipleURLEngine
	{
	public:
		static bool init(bool initSSL = true, bool initCurl = true);
		static void cleanup();

		/**
		 *  If maxThreadCount <= 0, maxThreadCount will be assigned 1000.
		 *  If callbackPool is nullptr, MultipleURLEngine will use the answerCallbackPool of ClientEngine.
		 */
		MultipleURLEngine(int nConnsInPerThread = 200, int initThreadCount = 1, int perfectThreadCount = 10,
							int maxThreadCount = 100, int maxConcurrentCount = 25000,
							int tempThreadLatencySeconds = 120,
							std::shared_ptr<TaskThreadPool> callbackPool = nullptr);
		~MultipleURLEngine();
		
		//==================================================================//
		//=  Single URL 
		//==================================================================//
		//---------------------------------------//
		//-          Sync Interfaces            -//
		//---------------------------------------//
		bool visit(const std::string& url, Result &result, int timeoutSeconds = 120,
		bool saveResponseData = true, const std::string& postBody = std::string(),
		const std::vector<std::string>& header = std::vector<std::string>());

		/* If return true, don't cleanup curl; if return false, please cleanup curl. */
		bool visit(CURL *curl, Result &result, int timeoutSeconds = 120,
			bool saveResponseData = true, const std::string& postBody = std::string(),
			const std::vector<std::string>& header = std::vector<std::string>());

		//---------------------------------------//
		//-  Async Interfaces: callback classes -//
		//---------------------------------------//
		bool visit(const std::string& url, ResultCallbackPtr callback, int timeoutSeconds = 120,
			bool saveResponseData = true, const std::string& postBody = std::string(),
			const std::vector<std::string>& header = std::vector<std::string>());

		/* If return true, don't cleanup curl; if return false, please cleanup curl. */
		inline bool visit(CURL *curl, ResultCallbackPtr callback, int timeoutSeconds = 120,
			bool saveResponseData = true, const std::string& postBody = std::string(),
			const std::vector<std::string>& header = std::vector<std::string>());

		//---------------------------------------//
		//-  Async Interfaces: Lambda callback  -//
		//---------------------------------------//
		bool visit(const std::string& url, std::function<void (Result &result)> callback,
			int timeoutSeconds = 120, bool saveResponseData = true, const std::string& postBody = std::string(),
			const std::vector<std::string>& header = std::vector<std::string>());

		/* If return true, don't cleanup curl; if return false, please cleanup curl. */
		inline bool visit(CURL *curl, std::function<void (Result &result)> callback,
			int timeoutSeconds = 120, bool saveResponseData = true, const std::string& postBody = std::string(),
			const std::vector<std::string>& header = std::vector<std::string>());

		//==================================================================//
		//=  Batch URLs 
		//==================================================================//
		//---------------------------------------//
		//-  Async Interfaces: callback classes -//
		//---------------------------------------//
		bool addToBatch(const std::string& url, ResultCallbackPtr callback, int timeoutSeconds = 120,
			bool saveResponseData = true, const std::string& postBody = std::string(),
			const std::vector<std::string>& header = std::vector<std::string>());

		/* If return true, don't cleanup curl; if return false, please cleanup curl. */
		inline bool addToBatch(CURL *curl, ResultCallbackPtr callback, int timeoutSeconds = 120,
			bool saveResponseData = true, const std::string& postBody = std::string(),
			const std::vector<std::string>& header = std::vector<std::string>());

		//---------------------------------------//
		//-  Async Interfaces: Lambda callback  -//
		//---------------------------------------//
		bool addToBatch(const std::string& url, std::function<void (Result &result)> callback,
			int timeoutSeconds = 120, bool saveResponseData = true, const std::string& postBody = std::string(),
			const std::vector<std::string>& header = std::vector<std::string>());

		/* If return true, don't cleanup curl; if return false, please cleanup curl. */
		inline bool addToBatch(CURL *curl, std::function<void (Result &result)> callback,
			int timeoutSeconds = 120, bool saveResponseData = true, const std::string& postBody = std::string(),
			const std::vector<std::string>& header = std::vector<std::string>());
		
		bool commitBatch();
		std::string status();

		void status(StatusInfo &statusInfo);
	};
	typedef std::shared_ptr<MultipleURLEngine> MultipleURLEnginePtr;

#### 初始化与构造函数

##### init

	static bool init(bool initSSL = true, bool initCurl = true);

全局初始化函数。

初始化并发访问引擎依赖的第三方库，比如 [OpenSSL](https://www.openssl.org/)，[libcurl](https://curl.se/)。

不论构造多少个并发访问引擎实例，该函数仅需调用一次。

##### cleanup

	static void cleanup();

全局清理函数。

清理并发访问引擎所依赖的第三方库，比如 [OpenSSL](https://www.openssl.org/)，[libcurl](https://curl.se/)。

不论构造多少个并发访问引擎实例，该函数仅需调用一次。

##### MultipleURLEngine

	MultipleURLEngine(int nConnsInPerThread = 200, int initThreadCount = 1, int perfectThreadCount = 10,
						int maxThreadCount = 100, int maxConcurrentCount = 25000,
						int tempThreadLatencySeconds = 120,
						std::shared_ptr<TaskThreadPool> callbackPool = nullptr);

并发访问引擎构造函数。

**参数说明**

* **`int nConnsInPerThread`**

	一个工作线程同时执行的任务数上限。

* **`int initThreadCount`**

	初始化并发访问引擎时，初始化的工作线程数。

* **`int perfectThreadCount`**

	并发访问引擎空闲时，保留的最大线程数。

* **`int maxThreadCount`**

	并发访问引擎最大工作线程数。

* **`int maxConcurrentCount`**

	并发访问引擎最大病发任务数量。

* **`int tempThreadLatencySeconds`**

	在并发引擎空闲时，超过 `perfectThreadCount` 数量限制的临时线程退出前，等待新任务的等待时间。单位：秒。

* **`std::shared_ptr<TaskThreadPool> callbackPool`**

	执行访问响应处理，和异步访问回调的线程池。

	如果为 nullptr，将采用 [ClientEngine](../core/ClientEngine.md) 的工作线程池(请求响应和 Callback 处理线程池)作为替代。

#### 成员函数

##### visit

	//---------------------------------------//
	//-          Sync Interfaces            -//
	//---------------------------------------//
	bool visit(const std::string& url, Result &result, int timeoutSeconds = 120,
		bool saveResponseData = true, const std::string& postBody = std::string(),
		const std::vector<std::string>& header = std::vector<std::string>());

	/* If return true, don't cleanup curl; if return false, please cleanup curl. */
	bool visit(CURL *curl, Result &result, int timeoutSeconds = 120,
		bool saveResponseData = true, const std::string& postBody = std::string(),
		const std::vector<std::string>& header = std::vector<std::string>());

	//---------------------------------------//
	//-  Async Interfaces: callback classes -//
	//---------------------------------------//
	bool visit(const std::string& url, ResultCallbackPtr callback, int timeoutSeconds = 120,
		bool saveResponseData = true, const std::string& postBody = std::string(),
		const std::vector<std::string>& header = std::vector<std::string>());

	/* If return true, don't cleanup curl; if return false, please cleanup curl. */
	inline bool visit(CURL *curl, ResultCallbackPtr callback, int timeoutSeconds = 120,
		bool saveResponseData = true, const std::string& postBody = std::string(),
		const std::vector<std::string>& header = std::vector<std::string>());

	//---------------------------------------//
	//-  Async Interfaces: Lambda callback  -//
	//---------------------------------------//
	bool visit(const std::string& url, std::function<void (Result &result)> callback,
		int timeoutSeconds = 120, bool saveResponseData = true, const std::string& postBody = std::string(),
		const std::vector<std::string>& header = std::vector<std::string>());

	/* If return true, don't cleanup curl; if return false, please cleanup curl. */
	inline bool visit(CURL *curl, std::function<void (Result &result)> callback,
		int timeoutSeconds = 120, bool saveResponseData = true, const std::string& postBody = std::string(),
		const std::vector<std::string>& header = std::vector<std::string>());

以 HTTP 的形式访问 URL，并获取返回信息。

**参数说明**

* **`const std::string& url`**

	需要访问的 URL。

* **`CURL *curl`**

	需要访问的 curl（URL 已设置的 curl）。

* **`Result &result`**

	访问的结果状态和数据，请参见 [Result](#Result)。

* **`int timeoutSeconds`**

	访问超时。单位：秒。

* **`bool saveResponseData`**

	是否保存返回的数据。

* **`const std::string& postBody`**

	POST 操作时，需要提交的数据。GET 操作传入空字符串即可。

* **`const std::vector<std::string>& header`**

	HTTP 请求的 Header。

* **`ResultCallbackPtr callback`**

	异步访问的结果回调函数。请参见 [ResultCallback](#ResultCallback)。

* **`std::function<void (Result &result)> callback`**

	异步访问的结果回调 lambda 函数。Result 请参见 [Result](#Result)。

**返回值**

+ true：

	+ 同步接口：访问完成。
	+ 异步接口：异步访问提交成功。

+ false：

	+ 同步接口：访问失败。
	+ 异步接口：异步访问提交失败。

**注意**

对于参数为 `CURL *curl` 的接口，如果 visit 函数返回 true，则无须释放 curl 结构，curl 结构将被并发访问引擎接管。如果 visit 接口返回 false，调用者需要负责释放 curl 结构。

##### addToBatch

	//---------------------------------------//
	//-  Async Interfaces: callback classes -//
	//---------------------------------------//
	bool addToBatch(const std::string& url, ResultCallbackPtr callback, int timeoutSeconds = 120,
		bool saveResponseData = true, const std::string& postBody = std::string(),
		const std::vector<std::string>& header = std::vector<std::string>());

	/* If return true, don't cleanup curl; if return false, please cleanup curl. */
	inline bool addToBatch(CURL *curl, ResultCallbackPtr callback, int timeoutSeconds = 120,
		bool saveResponseData = true, const std::string& postBody = std::string(),
		const std::vector<std::string>& header = std::vector<std::string>());

	//---------------------------------------//
	//-  Async Interfaces: Lambda callback  -//
	//---------------------------------------//
	bool addToBatch(const std::string& url, std::function<void (Result &result)> callback,
		int timeoutSeconds = 120, bool saveResponseData = true, const std::string& postBody = std::string(),
		const std::vector<std::string>& header = std::vector<std::string>());

	/* If return true, don't cleanup curl; if return false, please cleanup curl. */
	inline bool addToBatch(CURL *curl, std::function<void (Result &result)> callback,
		int timeoutSeconds = 120, bool saveResponseData = true, const std::string& postBody = std::string(),
		const std::vector<std::string>& header = std::vector<std::string>());

批量添加异步 HTTP 访问请求。

**注意**

+ 批量提交分为两步，在 addToBatch（批量添加异步 HTTP 访问请求）后，需要提交批量请求（commitBatch），请参见 [commitBatch](#commitBatch)。
+ 批量提交不能跨线程操作。

**参数说明**

* **`const std::string& url`**

	需要访问的 URL。

* **`CURL *curl`**

	需要访问的 curl（URL 已设置的 curl）。

* **`ResultCallbackPtr callback`**

	异步访问的结果回调函数。请参见 [ResultCallback](#ResultCallback)。

* **`std::function<void (Result &result)> callback`**

	异步访问的结果回调 lambda 函数。Result 请参见 [Result](#Result)。

* **`int timeoutSeconds`**

	访问超时。单位：秒。

* **`bool saveResponseData`**

	是否保存返回的数据。

* **`const std::string& postBody`**

	POST 操作时，需要提交的数据。GET 操作传入空字符串即可。

* **`const std::vector<std::string>& header`**

	HTTP 请求的 Header。

**返回值**

+ true：添加成功。
+ false：添加失败。

**注意**

对于参数为 `CURL *curl` 的接口，如果 addToBatch 函数返回 true，则无须释放 curl 结构，curl 结构将被并发访问引擎接管。如果 addToBatch 接口返回 false，调用者需要负责释放 curl 结构。

##### commitBatch

	bool commitBatch();

提交批量访问请求。

**注意**

+ 批量提交分为两步，在 [addToBatch](#addToBatch)（批量添加异步 HTTP 访问请求）后，才能调用该接口，提交批量请求。
+ 批量提交不能跨线程操作。

##### status

	std::string status();
	void status(StatusInfo &statusInfo);

获取并发访问引擎配置数据和当前状态。

第一个重载返回 JSON 格式的字符串数据，第二个重载返回 StatusInfo 结构。StatusInfo 请参见 [StatusInfo](#StatusInfo)。
