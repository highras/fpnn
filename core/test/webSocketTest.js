var questProcessor = new Object();

questProcessor.onConnected = function () { console.log("...connected..."); }
questProcessor.onClosed = function () { console.log("....closed...."); }
questProcessor.onError = function () { console.log("....error...."); }
questProcessor["duplex quest"] = function (quest) { console.log("receive server push"); console.log(quest); return {aaa:123}; }


var client = new FPNNClient('ws://35.167.185.139:13011/service/WebSocket', questProcessor);
client.sendQuest("httpDemo", "{}");

client.sendQuest("two way demo", {aaa:123}, function (a, b) { console.log(a);});

client.sendQuest("httpDemo", {}, function (a, b) { console.log(a);});

client.sendQuest("duplex demo", {}, function (a, b) { console.log(a);});
