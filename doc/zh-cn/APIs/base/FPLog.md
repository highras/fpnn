## FPLog

### 介绍

分布式日志服务 LogServer 的日志客户端。

文件配置可参见 [conf.template](../../../conf.template) 相关条目。代码配置可参见代码文件 [FPLog.h](../../../../base/FPLog.h)，介绍从略。

### 命名空间

	namespace fpnn;

### 宏定义

#### LOG_INFO

	#define LOG_INFO( fmt, args...)

以 `INFO` 级别记录日志。如果当前日志缓存级别高于 `INFO` 级别，则该日志将被忽略。输出日志的 tag 字段为空。具体使用方式类似于 `printf` 函数。

#### LOG_ERROR

	#define LOG_ERROR( fmt, args...)

以 `ERROR` 级别记录日志。如果当前日志缓存级别高于 `ERROR` 级别，则该日志将被忽略。输出日志的 tag 字段为空。具体使用方式类似于 `printf` 函数。

#### LOG_WARN

	#define LOG_WARN( fmt, args...)

以 `WARN` 级别记录日志。如果当前日志缓存级别高于 `WARN` 级别，则该日志将被忽略。输出日志的 tag 字段为空。具体使用方式类似于 `printf` 函数。

#### LOG_DEBUG

	#define LOG_DEBUG( fmt, args...)

以 `DEBUG` 级别记录日志。如果当前日志缓存级别高于 `DEBUG` 级别，则该日志将被忽略。输出日志的 tag 字段为空。具体使用方式类似于 `printf` 函数。

#### LOG_FATAL

	#define LOG_FATAL( fmt, args...)

以 `FATAL` 级别记录日志。如果当前日志缓存级别高于 `FATAL` 级别，则该日志将被忽略。输出日志的 tag 字段为空。具体使用方式类似于 `printf` 函数。

#### XLOG_INFO

	#define XLOG_INFO( tag, fmt, args...)

以 `INFO` 级别记录日志。如果当前日志缓存级别高于 `INFO` 级别，则该日志将被忽略。具体使用方式类似于 `printf` 函数。

#### XLOG_ERROR

	#define XLOG_ERROR( tag, fmt, args...)

以 `ERROR` 级别记录日志。如果当前日志缓存级别高于 `ERROR` 级别，则该日志将被忽略。具体使用方式类似于 `printf` 函数。

#### XLOG_WARN

	#define XLOG_WARN( tag, fmt, args...)

以 `WARN` 级别记录日志。如果当前日志缓存级别高于 `WARN` 级别，则该日志将被忽略。具体使用方式类似于 `printf` 函数。

#### XLOG_DEBUG

	#define XLOG_DEBUG( tag, fmt, args...)

以 `DEBUG` 级别记录日志。如果当前日志缓存级别高于 `DEBUG` 级别，则该日志将被忽略。具体使用方式类似于 `printf` 函数。

#### XLOG_FATAL

	#define XLOG_FATAL( tag, fmt, args...)

以 `FATAL` 级别记录日志。如果当前日志缓存级别高于 `FATAL` 级别，则该日志将被忽略。具体使用方式类似于 `printf` 函数。

#### UXLOG

	#define UXLOG( tag, fmt, args...)
	
以 `INFO` 级别记录日志，该日志不受当前日志缓存级别的影响，将被确定记录。使用方式类似于 `printf` 函数。


### FPLog

	class FPLog {
	public:
	    FPLog();
	    ~FPLog();
	    static FPLogBasePtr init(std::ostream& out, const std::string& route, const std::string& level = "ERROR", const std::string& serverName = "unknown", int32_t pid = 0, size_t maxQueueSize = FPLOG_DEFAULT_MAX_QUEUE_SIZE);
	    static FPLogBasePtr init(const std::string& endpoint, const std::string& route, const std::string& level = "ERROR", const std::string& serverName = "unknown", int32_t pid = 0, size_t maxQueueSize = FPLOG_DEFAULT_MAX_QUEUE_SIZE);
	    static void setLevel(const std::string& logLevel);
	    static std::string getLevel();
	    static FPLogBasePtr instance();
	};

#### 成员函数

##### init

	static FPLogBasePtr init(std::ostream& out, const std::string& route, const std::string& level = "ERROR",
		const std::string& serverName = "unknown", int32_t pid = 0, size_t maxQueueSize = FPLOG_DEFAULT_MAX_QUEUE_SIZE);

	static FPLogBasePtr init(const std::string& endpoint, const std::string& route, const std::string& level = "ERROR",
		const std::string& serverName = "unknown", int32_t pid = 0, size_t maxQueueSize = FPLOG_DEFAULT_MAX_QUEUE_SIZE);

初始化/修改日志初始设置。

**参数说明**

* **`std::ostream& out`**

	日志输出目的地。比如：`std::cout` 或 `std::cerr`。

* **`const std::string& endpoint`**

	日志输出目的地。支持 `std::cout`、`std::cerr`，以及 TCP 和 UNIX 协议。

* **`const std::string& route`**

	日志路由标记。

* **`const std::string& level`**

	日志级别。

	可选值："FATAL"、"ERROR"、"WARN"、"INFO"、"DEBUG"。

* **`const std::string& serverName`**

	服务名称。

* **`int32_t pid`**

	保留参数，暂未使用。

* **`size_t maxQueueSize`**

	日志缓存队列大小。

##### setLevel

	static void setLevel(const std::string& logLevel);

设置日志等级。

低于设置的日志等级的日志，将会被忽略。只有级别等于，或者高于设置的日志级别的日志，才会被日志系统记录和输出。

参数值与相对等级：

| 参数值 | 相对等级 |
|-------|---------|
| "FATAL" | 最高 |
| "ERROR" | 次高 |
| "WARN" | 中等 |
| "INFO" | 次低 |
| "DEBUG" | 最低 |

##### getLevel

	static std::string getLevel();

获取当前日志等级。

##### instance

	static FPLogBasePtr instance();

获取 FPLog 实例。