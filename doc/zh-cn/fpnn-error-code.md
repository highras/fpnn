# FPNN 错误代码

| 数值 | 枚举字面量 | 备注 |
|-----|----------|------|
| 0 | FPNN_EC_OK | 正常，无错误。 |
|  |  |  |
| 10001 | FPNN_EC_PROTO_UNKNOWN_ERROR | 未知的编解码错误 |
| 10002 | FPNN_EC_PROTO_NOT_SUPPORTED | 不支持的协议/格式/编码 |
| 10003 | FPNN_EC_PROTO_INVALID_PACKAGE | 无效的数据包 |
| 10004 | FPNN_EC_PROTO_JSON_CONVERT | JSON 转换失败 |
| 10005 | FPNN_EC_PROTO_STRING_KEY | key 不是字符串类型 |
| 10006 | FPNN_EC_PROTO_MAP_VALUE | 数据包根对象不是字典类型 |
| 10007 | FPNN_EC_PROTO_METHOD_TYPE | 数据包协议类型相关错误 |
| 10008 | FPNN_EC_PROTO_PROTO_TYPE | 不支持的协议/编码类型 |
| 10009 | FPNN_EC_PROTO_KEY_NOT_FOUND | key 不存在 |
| 10010 | FPNN_EC_PROTO_TYPE_CONVERT | 参数类型转换失败 |
| 10011 | FPNN_EC_PROTO_FILE_SIGN | 无效的文件签名 |
| 10012 | FPNN_EC_PROTO_FILE_NOT_EXIST | 文件不存在 |
|  |  |  |
| 20001 | FPNN_EC_CORE_UNKNOWN_ERROR | 未知的错误 |
| 20002 | FPNN_EC_CORE_CONNECTION_CLOSED | 链接已关闭 |
| 20003 | FPNN_EC_CORE_TIMEOUT | 操作超时 |
| 20004 | FPNN_EC_CORE_UNKNOWN_METHOD | 未知的方法/所请求的接口不存在 |
| 20005 | FPNN_EC_CORE_ENCODING | 编码错误 |
| 20006 | FPNN_EC_CORE_DECODING | 解码错误 |
| 20007 | FPNN_EC_CORE_SEND_ERROR | 发送错误 |
| 20008 | FPNN_EC_CORE_RECV_ERROR | 接收错误 |
| 20009 | FPNN_EC_CORE_INVALID_PACKAGE | 无效的数据包 |
| 20010 | FPNN_EC_CORE_HTTP_ERROR | HTTP 错误 |
| 20011 | FPNN_EC_CORE_WORK_QUEUE_FULL | 工作队列满 |
| 20012 | FPNN_EC_CORE_INVALID_CONNECTION | 无效的链接 |
| 20013 | FPNN_EC_CORE_FORBIDDEN | 禁止的操作 |
| 20014 | FPNN_EC_CORE_SERVER_STOPPING | 服务器正在停止服务 |
| 20015 | FPNN_EC_CORE_CANCELLED | 操作已被取消 |
|  |  |  |
| 30001 | FPNN_EC_ZIP_COMPRESS | 压缩异常 |
| 30002 | FPNN_EC_ZIP_DECOMPRESS | 解压异常 |

** 注意 **

* 错误代码的具体定义可以参见 [FpnnError.h](../../base/FpnnError.h)