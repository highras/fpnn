# FPNN 框架可动态调节参数列表

| key | 含义 | 取值范围 | 备注 |
|-----|-----|---------|-----|
| server.security.ip.whiteList.enable | 启用或者禁用白名单 | true、false  | TCP 和 UDP 服务器相互独立，不存在白名单联动 |
| server.security.ip.whiteList.addIP | 向白名单添加 IP 地址 | IP v4 & IP v6 地址 | TCP 和 UDP 服务器相互独立，不存在白名单联动 |
| server.security.ip.whiteList.removeIP | 从白名单中移除 IP 地址 | IP v4 & IP v6 地址 | TCP 和 UDP 服务器相互独立，不存在白名单联动 |
| server.core.size | 进程崩溃时，core 大小限制 | 以字节为单位的整数 |  |
| server.no.file | 进程可用文件描述符数量修改 | 整数值 |  |
| server.log.level | 进程日志级别 | DEBUG、INFO、WARN、ERROR、FATAL |  |
| server.quest.log | 是否开启服务端请求日志 | true、false | 默认关闭 |
| server.answer.log | 是否开启服务端应答日志 | true、false | 默认关闭 |
| server.slow.log | 是否开启服务端慢日志 | 正整数。单位：毫秒。0 为关闭。 | 默认关闭 |
| client.quest.log | 是否开启客户端请求日志 | true、false | 默认关闭 |
| client.answer.log | 是否开启客户端应答日志 | true、false | 默认关闭 |
| client.slow.log | 是否开启客户端慢日志 | --- | 预留接口，暂无效果 |
| server.http.supported | 是否开启 HTTP 支持 | true、false | 默认关闭 |
| server.stat | 是否开启接口状态统计 | true、false | 服务启动后，默认开启 |
| server.status.logStatusInfos | 是否定时将服务状态写入日志 | true、false | 默认关闭 |
| server.status.logStatusInterval | 服务状态写入日志的时间间隔。单位：秒 | 正整数 | 60 秒 |
