## TimeUtil

### 介绍

时间格式化工具函数集合。

### 命名空间

	namespace fpnn::TimeUtil;

### 全局函数

#### getTimeRFC1123

	std::string getTimeRFC1123();

按 RFC1123 的格式，格式化当前 UTC 时间。

#### getDateTime

	std::string getDateTime();
	std::string getDateTime(int64_t t);

按 `yyyy-MM-dd HH:mm:ss` 格式，格式化当前时间，或 `t` 指定的时间。

#### getDateStr

	std::string getDateStr(char sep = '-');
	std::string getDateStr(int64_t t, char sep = '-');

按 `yyyy-MM-dd` 格式，格式化当前时间，或 `t` 指定的时间。连接字符由 `sep` 参数指定。

#### getDateHourStr

	std::string getDateHourStr(char sep = '-');
	std::string getDateHourStr(int64_t t, char sep = '-');

按 `yyyy-MM-dd-HH` 格式，格式化当前时间，或 `t` 指定的时间。连接字符由 `sep` 参数指定。

#### getTimeStr

	std::string getTimeStr(char sep = '-');
	std::string getTimeStr(int64_t t, char sep = '-');

按 `yyyy-MM-dd-HH-mm-ss` 格式，格式化当前时间，或 `t` 指定的时间。连接字符由 `sep` 参数指定。

#### getDateTimeMS

	std::string getDateTimeMS();
	std::string getDateTimeMS(int64_t t);

按 `yyyy-MM-dd HH:mm:ss,SSS` 格式，格式化当前时间，或 `t` 指定的时间。