# FPNN WebSocket SDK

[English](README.md)


## 一、介绍

请使用 FPNN v2.0.1 级之后版本。FPNN 从 v2.0.1 开始支持 WebSocket。

**特性：**

* FPNN WebSocket 支持 duplex (server push)

* FPNN WebSocket 使用标准 FPNN 二进制头部，及 JSON 格式的 Payload

* FPNN WebSocket 不支持加密链接  
  没有合适的 JS 版加解密库，因此暂无法支持 FPNN 加密链接。

* FPNN WebSocket 不支持 wss 协议  
  因为 OpenSSL & TLS 自身的并发及性能问题，FPNN 不支持 OpenSSL & TLS。  
  因此，无法支持依赖于 OpenSSL & TLS 的 wws 协议。

**注意：**

* FPNN 服务配置
    * 请勿启动 FPNN.server.security.forceEncrypt.userMethods 配置项，否则 WebSocket 无法连接。

    * 请打开 HTTP 支持，否则 WebSocket 无法连接  
      `FPNN.server.http.supported = true`  
      对于 WebSocket，配置项  
      `FPNN.server.http.closeAfterAnswered`  
      将被忽略。

* 请确认浏览器版本是否支持 WebSocket。

* FPNN 目前忽略控制帧中的数据，且回应的 pong 帧的 payload 为空，不带数据。

## 二、API

1. **相关结构**

    1. **QuestProcessor**

        Object 类型，用于事件处理器。包括链接事件和 server push。

        * 如果处理链接事件，QuestProcessor 根据业务需要，需提供以下方法：

            | 方法            | 对应 WebSocket 事件 | 说明     |
            | ------------- | --------------- | ------ |
            | onConnected() | onopen(event)   | 链接已经建立 |
            | onError()     | onerror(event)  | 发生错误   |
            | onClosed()    | onclose(event)  | 链接已经关闭 |

        * 如果处理 duplex (server push) 事件，需提供以接口方法名为名称的属性，且该属性类型为 function (quest) { ... }。

            假设需要提供名为 demo 的接口，如下示例：
        
                var questProcessor = new Object();
                questProcessor.demo = function (quest) { ...}
                //-- 或者
                questProcessor["demo"] = function (quest) { ...}

            quest 为 JSON Object。

            如果是 oneway/notify 请求，直接返回 null/undefined 即可。

            如果是 twoway/quest 请求，请返回 JSON Object，或者 json 字符串。

    2. **Error Answer**

        标准的错误的应答，至少包含以下三个部分：

        | 属性     | 类型     | 含义      |
        | ------ | ------ | ------- |
        | code   | int    | 错误码     |
        | ex     | string | 异常说明    |
        | raiser | string | 抛出异常的地方 |

        其中 code 的取值，请参考 **SDK 代码**中 FPNNClient.ErrorCode 对象的定义。

    3. **Quest Callback**

        Quest 的 Callback 有两种类型，Function 和 Object。

            //-- Function Type
            function questCallback(answer, errorCode);
       
            //-- Object Type
            var callback = new Object();
            callback.onAnswer = function (answer) { ... };
            callback.onException = function (answer, errorCode) { ... };

        SDK 会自动识别相应类型。

        如果没有特殊需求，比如数据聚合和保存其他变量，建议使用函数类型。

        | 参数       | 说明                                       |
        | --------- | ---------------------------------------- |
        | answer    | JSON Object 类型对象。如果 errorCode 不为 0，则为标准 Error Answer 类型，或者 undefined。 |
        | errorCode | 整型。如果没有错误，则为 0。                          |

2. **使用**

    请包含 fpnn-ws.js。

3. **创建客户端实例**

        var client = new FPNNClient(endpoint, questProcessor, autoConnect, defaultTimeout);
    
    | 参数             | 必要性  | 说明                                       | 默认值       |
    | -------------- | ---- | ---------------------------------------- | --------- |
    | endpoint       | 必选   | 服务器地址                                    | N/A       |
    | questProcessor | 可选   | 事件处理器。处理链接事件和 server push 事件。 请参见“1. 相关结构” 部分 “a. QuestProcessor” 节。| undefined |
    | autoConnect    | 可选   | 是否自动链接                                   | true      |
    | defaultTimeout | 可选   | 客户端发送数据默认超时时间。单位为秒              | 5 秒 |
    另外，如果不小心写成
    
        var client = FPNNClient(endpoint, questProcessor, autoConnect, defaultTimeout);
    
    亦可，SDK 内部处理了该异常情况。但强烈建议以第一种格式使用。

4. **连接服务器**

        client.connect();
      
    当创建客户端实例时，`autoConnect` 为 true，则连接服务器为可选操作。此时，如未连接，发送数据时，客户端实例将自动连接。

5. **发送数据**

    1. **发送 oneway 数据**

            client.sendNotify(method, jsonObject);
      
        | 参数         | 必要性  | 说明                                   |
        | ---------- | ---- | ------------------------------------ |
        | method     | 必选   | 方法名，字符串形式                            |
        | jsonObject | 必选   | 参数包，即 payload。可以是字符串形式，或者 Object 对象。 |
      
        **注意**：oneway 数据没有回应。

    2. **发送 twoway 数据**

            client.sendQuest(method, jsonObject, callback, timeout);
      
        | 参数         | 必要性  | 说明                                       | 默认值       |
        | ---------- | ---- | ---------------------------------------- | --------- |
        | method     | 必选   | 方法名，字符串形式                                |           |
        | jsonObject | 必选   | 参数包，即 payload。可以是字符串形式，或者 Object 对象。     |           |
        | callback   | 可选   | answer 的处理函数。请参见“1. 相关结构” 部分 “c. Quest Callback” 节。 | undefined |
        | timeout    | 可选   | 请求超时。创建客户端实例时，由 defaultTimeout 参数决定。单位：秒 | 5 秒       |

6. **关闭链接**

        client.close();
      
    该调用不是强制的。
      
    但如果希望及时关闭链接，还是需要明确调用的。

7. **处理连接事件**

    目前支持链接建立事件，连接错误事件，连接关闭事件。

    具体请参见“1. 相关结构” 部分 “a. QuestProcessor” 节，链接事件部分。

8. **duplex (server push)**

    具体请参见“1. 相关结构” 部分 “a. QuestProcessor” 节，duplex (server push) 事件部分。


## 三、例子

假设以 FPNN 框架自带的 serverTest 为目标服务：

```javascript
//-- 链接事件 & server push
var questProcessor = new Object();
questProcessor.onConnected = function () { console.log("...connected..."); }
questProcessor.onClosed = function () { console.log("....closed...."); }
questProcessor.onError = function () { console.log("....error...."); }
questProcessor["duplex quest"] = function (quest) { console.log("receive server push"); console.log(quest); return {aaa:123}; }
 
//-- Object Callback 示范
var callbackObject = new Object();
callbackObject.onAnswer = function (answer) { console.log(answer); };
callbackObject.onException = function (answer, errorCode) { console.log("quest exception"); };
 
//-- 创建客户端
var client = new FPNNClient('ws://35.167.185.139:13011/service/WebSocket', questProcessor);
 
//-- 发送数据
client.sendQuest("httpDemo", "{}");
client.sendQuest("two way demo", {aaa:123}, function (a, b) { console.log(a);});
client.sendQuest("httpDemo", {}, function (a, b) { console.log(a);});
client.sendQuest("duplex demo", {}, function (a, b) { console.log(a);});
 
client.sendQuest("duplex demo", {}, callbackObject);
```