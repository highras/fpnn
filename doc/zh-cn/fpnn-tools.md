# FPNN 内置工具

[TOC]

## 一、通用工具

位置：[/core/tools/](../../core/tools/)

### cmd

FPNN 通用命令行客户端。可以发送任何 fpnn 命令，调用任何 fpnn 接口。

	Usage: ./cmd ip port method body(json) [-ssl] [-json] [-oneway] [-t timeout]
	Usage: ./cmd ip port method body(json) [-ecc-pem ecc-pem-file] [-json] [-oneway] [-t timeout]
	Usage: ./cmd ip port method body(json) [-ecc-der ecc-der-file] [-json] [-oneway] [-t timeout]
	Usage: ./cmd ip port method body(json) [-ecc-curve ecc-curve-name -ecc-raw-key ecc-raw-public-key-file] [-json] [-oneway] [-t timeout]
	Usage: ./cmd ip port method body(json) [-udp] [-json] [-oneway] [-discardable] [-t timeout]

+ 接口参数须以 json 形式表示。

### cmdinfos

查询使用 FPNN 框架开发的服务的框架信息/状态及业务信息/状态（如果业务支持）。

	Usage: ./cmdinfos [ip] port [-udp]

+ ip 参数默认是 localhost。

### cmdtune

调整使用 FPNN 框架开发的服务的运行参数及业务参数（如果业务支持）。

	Usage: ./cmdtune [ip] port key value [-udp]

+ 如果目标服务配置了 FPNN ECC 加密，请使用 **cmd** 工具在加密链接下调整。非加密链接的 **cmdtune** 将被禁止链接。
+ ip 参数默认是 localhost。

### cmdstatus

检测使用 FPNN 框架开发的服务的运行状态。

	Usage: ./cmdstatus [ip] port [-udp]

+ ip 参数默认是 localhost。

### eccKeyMaker

FPNN 加密链接的秘钥对生成器。  

	Usage: ./eccKeyMaker <ecc-curve> key-pair-name

+ ecc-curve：secp192r1、secp224r1、secp256r1、secp256k1 四者之一。
+ key-pair-name：密钥文件名称前缀。

运行后会在当前目录下产生五个二进制文件：

+ 原始私钥：\<key-pair-nam>-private.key
+ 原始公钥：\<key-pair-nam>-public.key
+ 压缩的公钥：\<key-pair-nam>-compressed-public.key
+ PEM 格式的公钥：\<key-pair-nam>-public.pem
+ DER 格式的公钥：\<key-pair-nam>-public.der


### fss

FPNN Secure Shell，FPNN 加密交互式命令行终端。

	Usage: ./fss ip port
	Usage: ./fss ip port -udp
	Usage: ./fss ip port -ssl
	Usage: ./fss ip port -pem pem-file [encrypt-mode-opt] [encrypt-strength-opt]
	Usage: ./fss ip port -der der-file [encrypt-mode-opt] [encrypt-strength-opt]
	Usage: ./fss ip port ecc-curve raw-public-key-file [encrypt-mode-opt] [encrypt-strength-opt]

+ ecc-curve：secp192r1、secp224r1、secp256r1、secp256k1 四者之一。
+ encrypt-mode-opt：stream 或 package
+ encrypt-strength-opt：128bits 或 256bits



## 二、测试工具

位置：[/core/test/](../../core/test/)

### 1. 通用测试工具

位置：[/core/test/UniversalFunctionalTests/](../../core/test/UniversalFunctionalTests/)


#### clientAsyncOnewayTest

Oneway 消息功能／压力测试客户端。

	Usage: ./clientAsyncOnewayTest ip port [-udp]

+ 默认测试目标服务器请参见 [serverTest](#serverTest)。


#### duplexClientTest

duplex(双向双工)功能&压力测试工具。

	Usage: ./duplexClientTest ip port threadNum sendCount [-udp]

+ 默认测试目标服务器请参见 [serverTest](#serverTest)。


#### periodClientTest

周期性测试工具。

	Usage: ./periodClientTest ip port quest_period(seconds) [-udp] [-keepAlive]

+ 默认测试目标服务器请参见 [serverTest](#serverTest)。


#### serverTest

目标测试服务器。

	Usage: ./serverTest config [print-period-in-seconds] [-udp]

+ print-period-in-seconds 参数默认为 3 秒。


#### singleClientConcurrentTest

单一客户端多线程并发稳定性测试工具。

	Usage: ./singleClientConcurrentTest ip port [-ssl]
	Usage: ./singleClientConcurrentTest ip port [-ecc-pem ecc-pem-file [-package|-stream] [-128bits|-256bits]]
	Usage: ./singleClientConcurrentTest ip port -udp

+ 默认测试目标服务器请参见 [serverTest](#serverTest)。


#### timeoutTest

超时相关测试工具。

	Usage: ./timeoutTest ip port delay_seconds [-e engine_quest_timeout] [-c client_quest_timeout] [-q single_quest_timeout] [-udp]

+ 默认测试目标服务器请参见 [serverTest](#serverTest)。


### 2. TCP 专用测试工具

位置：[/core/test/tcp-test/](../../core/test/tcp-test/)

#### asyncCloseConnectionAfterSentViolentTest

	Usage: ./asyncCloseConnectionAfterSentViolentTest ip port [--connections 1000] [--duplex] [--factor 500]

+ 默认测试目标服务器请参见 [serverTest](#serverTest)。


#### asyncStressClient (TCP)

TCP 异步压力测试客户端。  
可作为业务服务器的压力测试模版使用。  
单个实例可模拟指定数目的链接，和总的输出 QPS。  
一般使用多个实例共同测试。

	Usage: ./asyncStressClient ip port connections totalQPS [-ssl] [-thread client-work-thread-count]
	Usage: ./asyncStressClient ip port connections totalQPS [-ecc-pem ecc-pem-file [-package|-stream] [-128bits|-256bits]] [-thread client-work-thread-count]

+ 默认测试目标服务器请参见 [serverTest](#serverTest)。


#### clientAsyncTest

单链接洪水压力测试工具。(建议使用 netAsyncTest)

	Usage: ./clientAsyncTest ip port [client_work_thread]

+ 默认测试目标服务器请参见 [serverTest](#serverTest)。


#### clientTest

功能&压力&稳定性测试工具。

	Usage: ./clientTest ip port threadNum sendCount

+ 默认测试目标服务器请参见 [serverTest](#serverTest)。


#### massiveAsyncClientTest

最大链接数异步压测工具。  
一般使用多个实例共同测试。

	Usage: ./massiveAsyncClientTest <ip> <port> <threadNum> <clientCount> <perClientQPS> [timeout] [worker_threads]

+ 默认测试目标服务器请参见 [serverTest](#serverTest)。


#### massiveClientTest

最大链接数压测工具。  
一般使用多个实例共同测试。

	Usage: ./massiveClientTest <ip> <port> <threadNum> <clientCount> <perClientQPS> [timeout] [worker_threads]

+ 默认测试目标服务器请参见 [serverTest](#serverTest)。


#### netAsyncTest

单链接洪水压力测试工具。

	Usage: ./netAsyncTest ip port

+ 默认测试目标服务器请参见 [serverTest](#serverTest)。


#### shortConnectionTesting

短链接压力测试工具。

	Usage: ./shortConnectionTesting ip port threadNum sendCount [encryptConfigFile]

+ encryptConfigFile 模版请参见 [clientEncrypt.conf](../../core/test/tcp-test/clientEncrypt.conf)。
+ 默认测试目标服务器请参见 [serverTest](#serverTest)。


#### testCloneAnswer

Clone Answer 功能测试服务器。

	Usage: ./testCloneAnswer config

#### testCloneQuest

Clone Quest 功能测试服务器。

	Usage: ./testCloneQuest config

#### test_get.php

PHP over HTTP/GET 测试工具。

#### test_post.php

PHP over HTTP/POST 测试工具。


### 3. UDP 专用测试工具

位置：[/core/test/udp-test/](../../core/test/udp-test/)

#### asyncStressClient (UDP)

UDP 异步压力测试客户端。  
可作为业务服务器的压力测试模版使用。  
单个实例可模拟指定数目的链接，和总的输出 QPS。  
一般使用多个实例共同测试。

	Usage: ./asyncStressClient ip port connections qps [config-file]

+ 默认测试目标服务器请参见 [serverTest](#serverTest)。


### 4. TCP & UDP 对比测试工具

位置：[/core/test/TCP.vs.UDP/](../../core/test/TCP.vs.UDP/)

#### UDPBaseServer

TCP & UDP 单链接对比测试服务。

	Usage: ./UDPBaseServer config

+ 需要成对使用
+ 支持洲际测试

### 5. 分布式自动化测试工具

**注意**：分布式自动化测试工具需要在**DATS** (Distributed Automated Testing Suite)下使用。

位置：[/core/test/AutoDistributedTest/](../../core/test/AutoDistributedTest/)

#### TCPStressTest

TCP 分布式**异步**压力自动化测试模块。  
可作为业务服务器的分布式压力测试模版使用。

+ 默认测试目标服务器请参见 [serverTest](#serverTest)。
+ TCPStressController 可自动上传和分发 StressActor (FPNNStandardTCPStressActor)

TCPStressController 使用参数：

| 参数 | 类型 | 说明 | 默认值 |
|-----|------|-----|-------|
| contorlCenterEndpoint | 必须 | DATS 控制中心地址(host:port) | N/A |
| testEndpoint | 必须 | 目标服务器地址(host:port) | N/A |
| actor | 可选 | StressActor 二进制文件路径。如果该参数不为空，且 DATS 控制中心没有 StressActor 的缓存，则 controller 会自动上传和分发 Actor。具体上传和分发条件，请参考源代码。 | N/A |
| output | 可选 | 如果该项参数不为空，测试结束后，会将测试数据保存到该参数指定的文件(CSV 格式)。 | N/A |
| minCPUs | 可选 | 执行 StressActor 需要的最少 CPU 核心数量。 | 4 |
| stopPerCPULoad | 可选 | 标服务器 CPU 核心平均 load 上限。当连续 9 个统计周期，CPU 平均 load 达到该数值后，测试自动转入停止阶段。 | 1.5 |
| stopTimeCostMsec | 可选 | 当连续 9 个统计周期，平均请求响应时间超过该值后，测试自动转入停止阶段。单位：毫秒 | 600 |
| stressTimeout | 可选 | 压力测试请求的超时时间。单位：秒 | 60 |
| answerPoolThread | 可选 | StressActor 处理请求回调的线程池大小。 | Deployer/Actor 执行机的机器核数 |
| perStressConnections | 可选 | 单个任务模拟的链接数量。 | 100 |
| perStressQPS | 可选 | 单个任务模拟的 QPS 数量。 | 50000 |
| ssl | 可选 | 启用 SSL/TLS 链接。**无值参数**。与 FPNN ECC 加密参数互斥。 | <无值参数> |
| eccPublicKey | 可选 | FPNN ECC 加密公钥，PEM 格式。与 ssl 参数互斥。 | N/A |
| eccEncryptMode | 可选 | FPNN ECC 加密模式。可选值为：stream、package。 | package |
| eccReinforce | 可选 | FPNN ECC 加密强度。false: 使用 128 位密钥；true: 使用 256 位密钥。 | false |

#### UDPStressTest

UDP 分布式**异步**压力自动化测试模块。  
可作为业务服务器的分布式压力测试模版使用。

+ 默认测试目标服务器请参见 [serverTest](#serverTest)。
+ UDPStressController 可自动上传和分发 StressActor (FPNNStandardUDPStressActor)

UDPStressController 使用参数：

| 参数 | 类型 | 说明 | 默认值 |
|-----|------|-----|-------|
| contorlCenterEndpoint | 必须 | DATS 控制中心地址(host:port) | N/A |
| testEndpoint | 必须 | 目标服务器地址(host:port) | N/A |
| actor | 可选 | StressActor 二进制文件路径。如果该参数不为空，且 DATS 控制中心没有 StressActor 的缓存，则 controller 会自动上传和分发 Actor。具体上传和分发条件，请参考源代码。 | N/A |
| output | 可选 | 如果该项参数不为空，测试结束后，会将测试数据保存到该参数指定的文件(CSV 格式)。 | N/A |
| minCPUs | 可选 | 执行 StressActor 需要的最少 CPU 核心数量。 | 4 |
| stopPerCPULoad | 可选 | 标服务器 CPU 核心平均 load 上限。当连续 9 个统计周期，CPU 平均 load 达到该数值后，测试自动转入停止阶段。 | 1.5 |
| stopTimeCostMsec | 可选 | 当连续 9 个统计周期，平均请求响应时间超过该值后，测试自动转入停止阶段。单位：毫秒 | 600 |
| stressTimeout | 可选 | 压力测试请求的超时时间。单位：秒 | 60 |
| answerPoolThread | 可选 | StressActor 处理请求回调的线程池大小。 | Deployer/Actor 执行机的机器核数 |
| perStressConnections | 可选 | 单个任务模拟的链接数量。 | 200 |
| perStressQPS | 可选 | 单个任务模拟的 QPS 数量。 | 20000 |
| mtu | 可选 | 指定使用的 MTU 大小。 | 576 |
| eccPublicKey | 可选 | FPNN ECC 加密公钥，PEM 格式。 | N/A |
| eccEncryptMode | 可选 | FPNN ECC 加密模式。可选值为：stream、package。 | package |
| eccReinforce | 可选 | FPNN ECC 加密强度。false: 使用 128 位密钥；true: 使用 256 位密钥。 | false |


#### TCPMassiveConnectionsTest

TCP 分布式海量长链接**同步**压力自动化测试模块。  
可作为业务服务器的分布式海量长链接测试模版使用。

+ 默认测试目标服务器请参见 [serverTest](#serverTest)。
+ TCPMassConnController 可自动上传和分发 MassConnActor (FPNNTCPMassiveConnectionsActor)

TCPMassConnController 使用参数：

| 参数 | 类型 | 说明 | 默认值 |
|-----|------|-----|-------|
| contorlCenterEndpoint | 必须 | DATS 控制中心地址(host:port) | N/A |
| testEndpoint | 必须 | 目标服务器地址(host:port) | N/A |
| actor | 可选 | MassConnActor 二进制文件路径。如果该参数不为空，且 DATS 控制中心没有 MassConnActor 的缓存，则 controller 会自动上传和分发 Actor。具体上传和分发条件，请参考源代码。 | N/A |
| output | 可选 | 如果该项参数不为空，测试结束后，会将测试数据保存到该参数指定的文件(CSV 格式)。 | N/A |
| minCPUs | 可选 | 执行 MassConnActor 需要的最少 CPU 核心数量。 | 4 |
| stopPerCPULoad | 可选 | 标服务器 CPU 核心平均 load 上限。当连续 9 个统计周期，CPU 平均 load 达到该数值后，测试自动转入停止阶段。 | 1.5 |
| stopTimeCostMsec | 可选 | 当连续 9 个统计周期，平均请求响应时间超过该值后，测试自动转入停止阶段。单位：毫秒 | 600 |
| stressTimeout | 可选 | 压力测试请求的超时时间。单位：秒 | 300 |
| answerPoolThread | 可选 | MassConnActor 处理请求回调的线程池大小。 | Deployer/Actor 执行机的机器核数 |
| massThreadCount | 可选 | MassConnActor 单次任务使用的线程数量。 | 1000 |
| clientCount | 可选 | MassConnActor 单次任务模拟的链接数量。 | 20000 |
| perClientQPS | 可选 | 单个客户端的 QPS 数量。 | 0.02 |
| maxClientsPerMachine | 可选 | 每个 Deployer/Actor 执行机可模拟的最大链接数量。 | 60000 |
| stopDelayWhenPeakOccur | 可选 | 如果在指定的时间内，没有再次达到链接数量的峰值，测试自动转入停止阶段。单位：分钟 | 5 |
| connPeakTolerance | 可选 | 连接峰值的容差。 | 2000 |
| extraWaitTime | 可选 | 当没有更多的 Deployer/Actor 执行机可供调度的情况下，测试进入停止阶段前，持续监控统计的时间。单位：分钟 | 0 |
| enableAutoBoost | 可选 | 是否启动自动增加模式。当无该参数时，MassConnActor 运行模式为固定压力模式。当有该参数时，MassConnActor 运行为自动增压模式。本参数为标记，无值。 | N/A |
| autoBoostFirstPeriod | 可选 | 单次压力任务开始后，第一次增压前的间隔时间。单位：分钟 | 10 |
| autoBoostPeriod | 可选 | 增压周期。 | 10 |
| autoBoostDecMsec | 可选 | 自动增压模式下，每次发送间隔，在每个增压周期内，如果大于1秒，减少1秒。如果小余1秒，则减少指定数量的毫秒数。单位：毫秒 | 5 |
| autoBoostMinSleepMsec | 可选 | 自动增压模式下，每次发送的最短间隔。如果初始间隔小于该值，则回归为固定压力模式，并忽略该值。单位：毫秒 | 5 |

#### UDPMassiveConnectionsTest

分布式海量长链接**同步**压力自动化测试模块。  
可作为业务服务器的分布式海量长链接测试模版使用。

+ 默认测试目标服务器请参见 [serverTest](#serverTest)。
+ UDPMassConnController 可自动上传和分发 MassConnActor (FPNNUDPMassiveConnectionsActor)

UDPMassConnController 使用参数：

| 参数 | 类型 | 说明 | 默认值 |
|-----|------|-----|-------|
| contorlCenterEndpoint | 必须 | DATS 控制中心地址(host:port) | N/A |
| testEndpoint | 必须 | 目标服务器地址(host:port) | N/A |
| actor | 可选 | MassConnActor 二进制文件路径。如果该参数不为空，且 DATS 控制中心没有 MassConnActor 的缓存，则 controller 会自动上传和分发 Actor。具体上传和分发条件，请参考源代码。 | N/A |
| output | 可选 | 如果该项参数不为空，测试结束后，会将测试数据保存到该参数指定的文件(CSV 格式)。 | N/A |
| minCPUs | 可选 | 执行 MassConnActor 需要的最少 CPU 核心数量。 | 4 |
| stopPerCPULoad | 可选 | 标服务器 CPU 核心平均 load 上限。当连续 9 个统计周期，CPU 平均 load 达到该数值后，测试自动转入停止阶段。 | 1.5 |
| stopTimeCostMsec | 可选 | 当连续 9 个统计周期，平均请求响应时间超过该值后，测试自动转入停止阶段。单位：毫秒 | 600 |
| stressTimeout | 可选 | 压力测试请求的超时时间。单位：秒 | 300 |
| answerPoolThread | 可选 | MassConnActor 处理请求回调的线程池大小。 | Deployer/Actor 执行机的机器核数 |
| massThreadCount | 可选 | MassConnActor 单次任务使用的线程数量。 | 1000 |
| clientCount | 可选 | MassConnActor 单次任务模拟的链接数量。 | 20000 |
| perClientQPS | 可选 | 单个客户端的 QPS 数量。 | 0.02 |
| maxClientsPerMachine | 可选 | 每个 Deployer/Actor 执行机可模拟的最大链接数量。 | 60000 |
| stopDelayWhenPeakOccur | 可选 | 如果在指定的时间内，没有再次达到链接数量的峰值，测试自动转入停止阶段。单位：分钟 | 5 |
| connPeakTolerance | 可选 | 连接峰值的容差。 | 2000 |
| extraWaitTime | 可选 | 当没有更多的 Deployer/Actor 执行机可供调度的情况下，测试进入停止阶段前，持续监控统计的时间。单位：分钟 | 0 |
| enableAutoBoost | 可选 | 是否启动自动增加模式。当无该参数时，MassConnActor 运行模式为固定压力模式。当有该参数时，MassConnActor 运行为自动增压模式。本参数为标记，无值。 | N/A |
| autoBoostFirstPeriod | 可选 | 单次压力任务开始后，第一次增压前的间隔时间。单位：分钟 | 10 |
| autoBoostPeriod | 可选 | 增压周期。 | 10 |
| autoBoostDecMsec | 可选 | 自动增压模式下，每次发送间隔，在每个增压周期内，如果大于1秒，减少1秒。如果小余1秒，则减少指定数量的毫秒数。单位：毫秒 | 5 |
| autoBoostMinSleepMsec | 可选 | 自动增压模式下，每次发送的最短间隔。如果初始间隔小于该值，则回归为固定压力模式，并忽略该值。单位：毫秒 | 5 |