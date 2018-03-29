# FPNN 内置调试模块介绍

## * 时间分析类

1. **TimeAnaylist**

位于 <fpnn-folder>/base/TimeAnalyst.h 文件中。

* class TimeCostAlarm

	单流程耗时分析。  
	析构时自动向日志输出从创建到析构经历的时间。单位：毫秒。

* class SegmentTimeAnalyst

	流程阶段耗时分析。  
	可以记录一个流程各个子阶段的时间消耗，以及流程总时间消耗。  
	析构时自动向日志输出流程总时间消耗，和流程各阶段的时间消耗。单位：毫秒。



## * 格式化输出类

1. **PrintMemory**

	打印内存数据

		#include "PrintMemory.h"
		using namespace fpnn;

		void printMemory(const void* memory, size_t size);




1. **printTable**

	以类似于 MySQL client 的方式，输出表格

		#include "FormattedPrint.h"
		using namespace fpnn;

		void printTable(const std::vector<std::string>& fields, const std::vector<std::vector<std::string>>& rows);

