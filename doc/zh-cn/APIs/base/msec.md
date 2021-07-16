## msec

### 介绍

快速时间获取模块。

### 命名空间

无命名空间约束。

### 全局函数

#### slack_mono_msec

	int64_t slack_mono_msec();

获取当前机器启动后已运行的时间。毫秒级（快速，成本低，但精度可能有误差）。

#### slack_real_msec

	int64_t slack_real_msec();

获取 UTC 毫秒级时间戳（快速，成本低，但精度可能有误差）。

#### slack_mono_sec

	int64_t slack_mono_sec();

获取当前机器启动后已运行的时间。秒级（快速，成本低，但精度可能有误差）。


#### slack_real_sec

	int64_t slack_real_sec();

获取 UTC 秒级时间戳（快速，成本低，但精度可能有误差）。

#### exact_mono_msec

	int64_t exact_mono_msec();

获取当前机器启动后已运行的时间。毫秒级（相对较慢，成本高，但精度准确）。


#### exact_real_msec

	int64_t exact_real_msec();

获取 UTC 毫秒级时间戳（相对较慢，成本高，但精度准确）。


#### exact_mono_usec

	int64_t exact_mono_usec();

获取当前机器启动后已运行的时间。微秒级（相对较慢，成本高，但精度准确）。

#### exact_real_usec

	int64_t exact_real_usec();

获取 UTC 微秒级时间戳（相对较慢，成本高，但精度准确）。

#### exact_mono_nsec

	int64_t exact_mono_nsec();

获取当前机器启动后已运行的时间。纳秒级（相对较慢，成本高，但精度准确）。

#### exact_real_nsec

	int64_t exact_real_nsec();

获取 UTC 纳秒级时间戳（相对较慢，成本高，但精度准确）。


