# FPNN WebSocket SDK

[简体中文](README.zh-cn.md)

## Introduction

Please use the version after FPNN v2.0.1. FPNN supports WebSocket from version v2.0.1.

**Features**

**Please Refer to [简体中文](README.zh-cn.md)**




## 例子

Assume using the serverTest (FPNN standard testing server) as the target server:

```javascript
//-- Connection events & server push
var questProcessor = new Object();
questProcessor.onConnected = function () { console.log("...connected..."); }
questProcessor.onClosed = function () { console.log("....closed...."); }
questProcessor.onError = function () { console.log("....error...."); }
questProcessor["duplex quest"] = function (quest) { console.log("receive server push"); console.log(quest); return {aaa:123}; }
 
//-- Demo for Object Callback
var callbackObject = new Object();
callbackObject.onAnswer = function (answer) { console.log(answer); };
callbackObject.onException = function (answer, errorCode) { console.log("quest exception"); };
 
//-- Create client instance
var client = new FPNNClient('ws://35.167.185.139:13011/service/WebSocket', questProcessor);
 
//-- send data
client.sendQuest("httpDemo", "{}");
client.sendQuest("two way demo", {aaa:123}, function (a, b) { console.log(a);});
client.sendQuest("httpDemo", {}, function (a, b) { console.log(a);});
client.sendQuest("duplex demo", {}, function (a, b) { console.log(a);});
 
client.sendQuest("duplex demo", {}, callbackObject);
```