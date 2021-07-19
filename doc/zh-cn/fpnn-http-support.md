# FPNN HTTP/HTTPS & webSocket (ws/wss) 支持

## 1. HTTP 支持概况

FPNN 框架 [TCP 服务组件](APIs/core/TCPEpollServer.md)支持 HTTP/HTTPS 1.1 常用功能；  
FPNN 框架扩展模块 [MultipleURLEngine](APIs/extends/MultipleURLEngine.md) 支持 HTTP/HTTPS 1.1 全部功能。

1. FPNN 框架 [TCP 服务组件](APIs/core/TCPEpollServer.md)目前仅支持HTTP POST 与 GET 请求。

	POST Body 支持 Content-Length 和 chunk 两种表述。

1. HTTP 支持默认处于关闭状态，需要显式启用。启用 HTTP 支持只须在配置文件中增加 `FPNN.server.http.supported ＝ true` 条目即可。
1. FPNN 默认支持 HTTP 和 TCP 包在同一个链接中混杂传递( FPNN 持有该能力，但**强烈不建议使用**。一般情况下禁止 TCP 请求和 HTTP 请求在同一链接中混合发送)。
1. 如果配置文件中没有配置 `FPNN.server.http.closeAfterAnswered` 条目，默认**不会**在发送完 HTTP 应答后关闭当前链接，而会保持当前链接一段时间。链接保持时间取决于配置项 `FPNN.server.idle.timeout`。默认 60 秒。
1. 如果配置 `FPNN.server.http.closeAfterAnswered = true`， 会在收到第一个 HTTP 请求后，停止在同一链接中接收新的请求（不论是HTTP还是TCP）。并在answer发送后，关闭链接。
1. 目前忽略 HTTP 1.1 标准中 Keep-Alive 操作，是否关闭链接取决于配置项 `FPNN.server.http.closeAfterAnswered` 和 `FPNN.server.idle.timeout`。
1. 目前不支持HTTP 1.1 标准中连续发送 HTTP 请求时，顺序回应的要求。回应的先后顺序取决于请求的完成先后。


## 2. HTTPS 支持概况

FPNN 对 HTTPS 的支持与 HTTP 相同。

启用 HTTP 支持后，配置相关的 SSL/TLS 参数，即可启用 HTTPS 功能。


## 3. 相关配置

1. **FPNN.server.http.supported**

	是否允许接收和回应 HTTP 请求。

1. **FPNN.server.http.closeAfterAnswered**

	当允许接收 HTTP 请求时，是否在收到任一 HTTP 请求后，停止接收请求，并在 HTTP 应答发送后，关闭链接。  
	如果该配置项为 true，则意味着进入传统 HTTP 1.0 一问一答模式。且一个链接只能发送并处理一次 HTTP 请求（同一链接中的TCP请求不受限制，但禁止在这种情况下，混合发送TCP和HTTP请求）。  
	当该配置项为 false 时，允许在一个链接中同时发送多个 HTTP 和 TCP 请求，且 HTTP 请求和 TCP 请求可以混合发送。

1. **FPNN.server.idle.timeout**

	链接的空置时间。空置超过该时间，链接将会被 FPNN 框架关闭，并进行资源回收。  
	空置状态取决于是否有完整包的传递，而不由是否有字节数据传递决定。

1. **FPNN.server.tcp.ipv4.ssl.listening.ip**

	TCP/IPv4 over SSL/TLS 监听地址

1. **FPNN.server.tcp.ipv4.ssl.listening.port**

	TCP/IPv4 over SSL/TLS 监听端口

1. **FPNN.server.tcp.ipv6.ssl.listening.ip**

	TCP/IPv6 over SSL/TLS 监听地址

1. **FPNN.server.tcp.ipv6.ssl.listening.port**

	TCP/IPv6 over SSL/TLS 监听端口

1. **FPNN.server.security.ssl.certificate**

	SSL/TLS 证书

1. **FPNN.server.security.ssl.privateKey**

	SSL/TLS 私钥


## 4. 违背禁止情况可能出现的意外

1. FPNN 框架 [TCP 服务模块](APIs/core/TCPEpollServer.md)不具备组装 HTTP 请求包，和解释 HTTP 应答包的功能，因此 FPNN 框架 [TCP 服务模块](APIs/core/TCPEpollServer.md)只能接收 HTTP 请求包和发送 HTTP 应答包，而无法发送 HTTP 请求包，和接收 HTTP 应答包。
	要发送 HTTP 请求包，并接收 HTTP 应答包，请使用 FPNN 框架扩展模块 [MultipleURLEngine](APIs/extends/MultipleURLEngine.md)。

1. 当 `FPNN.server.http.supported` 和 `FPNN.server.http.closeAfterAnswered` 均为 true，且混合发送 TCP 和 HTTP 请求时，可能会产生如下意外：

	如果在发送HTTP请求之前，发送了TCP请求；且TCP请求的应答在HTTP请求接收完后才完成发送，则将在TCP请求的应答发送完成后，关闭链接。导致HTTP的应答无法发送。

	期望时间顺序为：  
	收到TCP请求 ---> 收到HTTP请求 --->发送TCP应答 ---> 发送HTTP应答 ---> 关闭链接

	实际时间顺序为：  
	收到TCP请求 ---> 收到HTTP请求 --->TCP应答发送完成 ---> 关闭链接－－－－ HTTP 应答未发送

	修改：  
	因为正常模式下，不会在同一链接中混合发送TCP和HTTP请求。且在同一链接中混合发送TCP请求和HTTP请求属于禁止行为。只有特殊目的，才会允许。  
	因此该意外不属于bug，而属于意料之内的情况。因此不会进行修正。


## 5. FPNN HTTP 支持原理

HTTP 和 TCP 的大致处理流程  
![HTTP 和 TCP 的处理流程图](fpnn-http-tcp-flow.png)


## 6. WebSocket (ws/wss) 支持

WebSocket 需要打开 HTTP 支持：

	FPNN.server.http.supported = true

`FPNN.server.http.closeAfterAnswered` 参数不影响 WebSocket 行为。

WebSocket over SSL/TLS (wss) 需要配置 ssl 参数。

WebSocket 支持 server push。

**注意**

WebSocket 控制帧中如果包含 payload，payload 将会被忽略。
