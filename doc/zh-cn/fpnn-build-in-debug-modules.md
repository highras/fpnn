# FPNN 内置调试模块介绍

## 时间分析类

1. **SegmentTimeAnalyst**

	侵入式的多线程安全的时间消耗分析记录器。

	不仅记录任意操作总的时间消耗，还可以跟踪记录每一个流程**分段**的时间消耗，以及每一个循环的次数，总时间消耗和每个循环**片段**平均时间消耗。  
	当 SegmentTimeAnalyst 实例析构时，以上时间消耗数据均会以 `"Time cost"` 为 tag，用 [UXLOG](APIs/base/FPLog.md#UXLOG) 进行输出。

	详细文档请参见：[SegmentTimeAnalyst](APIs/base/TimeAnalyst.md#SegmentTimeAnalyst)。


1. **TimeCostAlarm**

	超时告警对象。

	当实例创建时，开始记录时间消耗。当实例析构时，将检测时间消耗是否超过构造时设定的时长。如果实际执行时长大于设定的时长，将使用 [LOG_WARN](APIs/base/FPLog.md#LOG_WARN) 在日志中输出实际执行时长。

	详细文档请参见：[TimeCostAlarm](APIs/base/TimeAnalyst.md#TimeCostAlarm)。


## 格式化输出类

1. **printMemory**

	以16进制和文本对照的形式，打印指定的内存数据到标准输出。

	详细文档请参见：[printMemory](APIs/base/PrintMemory.md)。


1. **visibleBinaryBuffer**

	输出指定内存块的可视化内容。

	详细文档请参见：[visibleBinaryBuffer](APIs/base/FormattedPrint.md#visibleBinaryBuffer)。


1. **printTable**

	格式化表格，以类似于 MySQL client 那样的格式，输出表格内容。

	详细文档请参见：[printTable](APIs/base/FormattedPrint.md#printTable)。

