# FPNN 内置工具

## 一、运维/管理工具

位置：<fpnn-folder>/core/tools/

* **cmd**

	FPNN 通用命令行客户端。可以发送任何 fpnn 命令，调用任何 fpnn 接口。

		Usgae: ./cmd ip port method body(json) isTwoWay isMsgPack [timeoutInSecond]

	+ 接口参数须以 json 形式表示。
	+ 该客户端不支持加密链接。
	+ isTwoWay 与 isMsgPack 以 0 和 1 表示 false 和 true。


* **eccKeyMaker**

	FPNN 加密链接的秘钥对生成器。  

		Usage: ./eccKeyMaker <ecc-curve> key-pair-name

	+ ecc-curve 为：  
		secp192r1、secp224r1、secp256r1、secp256k1 四者之一。

	运行后会在当前目录下产生五个二进制文件：

	+ 原始私钥：<name>-private.key
	+ 原始公钥：<name>-public.key
	+ 压缩的公钥：<name>-compressed-public.key
	+ PEM 格式的公钥：<name>-public.pem
	+ DER 格式的公钥：<name>-public.der


* **fss**

	FPNN Secure Shell，FPNN 加密交互式命令行终端。  
	支持加密链接和非加密链接。

		Usage: ./fss ip port
		Usage: ./fss ip port ecc-curve server-public-key-file [encrypt-mode-opt] [encrypt-strength-opt]

	+ ecc-curve：  
		secp192r1、secp224r1、secp256r1、secp256k1 四者之一。
	+ encrypt-mode-opt：stream 或 package
	+ encrypt-strength-opt：128bits 或 256bits



## 二、测试工具

位置：<fpnn-folder>/core/test/

* **asyncStressClient**

	异步压力测试客户端。  
	可作为业务服务器的压力测试模版使用。  
	一个实例可模拟指定数目的链接，和总的输出 QPS。

		Usage: ./asyncStressClient ip port connections qps [client_work_thread]
		Usage: ./asyncStressClient ip port connections qps client_work_thread [encryptConfigFile]

	+ encryptConfigFile 模版请参见 clientEncrypt.conf
	+ 默认测试目标服务器请参见 serverTest


* **clientAsyncOnewayTest**

	Oneway 消息功能／压力测试客户端。

		Usage: ./clientAsyncOnewayTest ip port

	+ 默认测试目标服务器请参见 serverTest


* **clientAsyncTest**

	单链接洪水压力测试工具。(建议使用 netAsyncTest)

		Usage: ./clientAsyncTest ip port [client_work_thread]

	+ 默认测试目标服务器请参见 serverTest


* **clientTest**

	功能&压力&稳定性测试工具。

		Usage: ./clientTest ip port threadNum sendCount

	+ 默认测试目标服务器请参见 serverTest


* **cmd**

	FPNN 通用命令行客户端。可以发送任何 fpnn 命令，调用任何 fpnn 接口。

		Usgae: ./cmd ip port method body(json) isTwoWay isMsgPack [timeoutInSecond]

	+ 接口参数须以 json 形式表示。
	+ 该客户端不支持加密链接。
	+ isTwoWay 与 isMsgPack 以 0 和 1 表示 false 和 true。


* **concurrentConnectionTesting**

	并发连接测试工具。

		Usage: ./concurrentConnectionTesting ip port threadNum sendCount [encryptConfigFile]

	+ encryptConfigFile 模版请参见 clientEncrypt.conf
	+ 默认测试目标服务器请参见 serverTest


* **duplexClientTest**

	duple(双向双工)功能&压力测试工具。

		Usage: ./duplexClientTest ip port threadNum sendCount

	+ 默认测试目标服务器请参见 serverTest


* **massiveClientTest**

	最大链接数压测工具。

		Usage: ./massiveClientTest ip port threadNum clientCount [interval_times_in_milliseconds] [timeout] [worker_threads]

	+ 默认测试目标服务器请参见 serverTest


* **netAsyncTest**

	单链接洪水压力测试工具。

		Usage: ./netAsyncTest ip port

	+ 默认测试目标服务器请参见 serverTest


* **periodClientTest**

	周期性测试工具。

		Usage: ./periodClientTest ip port quest_period(seconds)

	+ 默认测试目标服务器请参见 serverTest


* **serverTest**

	目标测试服务器。

		Usage: ./serverTest config [print-period-in-seconds]


* **shortConnectionTesting**

	短链接压力测试工具。

		Usage: ./shortConnectionTesting ip port threadNum sendCount [encryptConfigFile]

	+ encryptConfigFile 模版请参见 clientEncrypt.conf
	+ 默认测试目标服务器请参见 serverTest


* **singleClientConcurrentTest**

	单一客户端多线程并发稳定性测试工具。  
	(配合框架修改后，可测试是否有并发建立的多余链接。)

		Usage: ./singleClientConcurrentTest ip port [encryptConfigFile]

	+ encryptConfigFile 模版请参见 clientEncrypt.conf
	+ 默认测试目标服务器请参见 serverTest


* **test.php**

	PHP over HTTP 测试工具。


* **testCloneAnswer**

	Clone Answer 功能测试服务器。

		Usage: ./testCloneAnswer config

* **testCloneQuest**

	Clone Quest 功能测试服务器。

		Usage: ./testCloneQuest config


* **timeoutTest**

	超时相关测试工具。

		Usage: ./timeoutTest ip port delay_seconds engine_quest_timeout [client_quest_timeout]

	+ 默认测试目标服务器请参见 serverTest
