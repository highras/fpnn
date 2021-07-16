## ignoreSignals

### 介绍

简易信号忽略处理。

### 命名空间

	namespace fpnn;

### 全局函数

#### ignoreSignals

	inline void ignoreSignals();

忽略 `SIGHUP`、`SIGXFSZ`、`SIGPIPE`、`SIGTERM`、`SIGUSR1`、`SIGUSR2` 6个信号引发的中断。