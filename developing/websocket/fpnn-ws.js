/*
	questProcessor prototype:
		questProcessor.onConnected();
		questProcessor.onClosed();
		questProcessor.onError();

		questProcessor.<method>(quest);	//-- quest is a json object, include all quest parameters.
										//-- if oneway, just return null; if twoway, must return object.


	questCallback prototype:
		function questCallback(answer, errorCode);		//-- answer is a json object, include all answer parameters.
														//-- If errorCode is Zero, answer is normal answer,
														//-- else, answer maybe undefined, or exception answer.
		or 
		questCallback.onAnswer(answer);
		questCallback.onException(answer, errorCode); //-- answer maybe undefined.

	Error Json Object:
	{
		code: number,
		ex: string,
		raiser: string
	}

	Error code: (Please ref: infra-fpnn/base/FpnnError.h)
	请参考 FPNNClient.ErrorCode 对象声明。
*/


var FPNNClient = function (endpoint, questProcessor, autoConnect, defaultTimeout) {
	'use strict';

	if (!(this instanceof FPNNClient)) {
		return new FPNNClient(endpoint, questProcessor, autoConnect, defaultTimeout);
	}

	this.endpoint = endpoint;
	this.processor = questProcessor;
	this.sendQueue = new Array();

	if (autoConnect === undefined)
		this.autoConnect = true;
	else
		this.autoConnect = (autoConnect == true);

	if (defaultTimeout === undefined || defaultTimeout <= 0)
		this.defaultTimeout = 5;
	else
		this.defaultTimeout = defaultTimeout;
};

FPNNClient.SDKInfo = {
	version: '0.0.1'
};

FPNNClient.ErrorCode = {
	FPNN_EC_PROTO_UNKNOWN_ERROR: 10001,		// 未知错误（协议解析错误）
 	FPNN_EC_PROTO_NOT_SUPPORTED: 10002,		// 不支持的协议
 	FPNN_EC_PROTO_INVALID_PACKAGE: 10003,	// 无效的数据包
 	FPNN_EC_PROTO_JSON_CONVERT: 10004,		// JSON转换错误
 	FPNN_EC_PROTO_STRING_KEY: 10005,		// 数据包错误
 	FPNN_EC_PROTO_MAP_VALUE: 10006,			// 数据包错误
 	FPNN_EC_PROTO_METHOD_TYPE: 10007,		// 请求错误
 	FPNN_EC_PROTO_PROTO_TYPE: 10008,		// 协议类型错误
 	FPNN_EC_PROTO_KEY_NOT_FOUND: 10009,		// 数据包错误
 	FPNN_EC_PROTO_TYPE_CONVERT: 10010,		// 数据包转换错误
 	 	 	 
 	FPNN_EC_CORE_UNKNOWN_ERROR: 20001,		// 未知错误（业务流程异常中断）
 	FPNN_EC_CORE_CONNECTION_CLOSED: 20002,	// 链接已关闭
 	FPNN_EC_CORE_TIMEOUT: 20003,			// 请求超时
 	FPNN_EC_CORE_UNKNOWN_METHOD: 20004,		// 错误的请求
 	FPNN_EC_CORE_ENCODING: 20005,			// 编码错误
 	FPNN_EC_CORE_DECODING: 20006,			// 解码错误
 	FPNN_EC_CORE_SEND_ERROR: 20007,			// 发送错误
 	FPNN_EC_CORE_RECV_ERROR: 20008,			// 接收错误
 	FPNN_EC_CORE_INVALID_PACKAGE: 20009,	// 无效的数据包
 	FPNN_EC_CORE_HTTP_ERROR: 20010,			// HTTP错误
 	FPNN_EC_CORE_WORK_QUEUE_FULL: 20011,	// 任务队列满
 	FPNN_EC_CORE_INVALID_CONNECTION: 20012,	// 无效的链接
 	FPNN_EC_CORE_FORBIDDEN: 20013,			// 禁止操作
 	FPNN_EC_CORE_SERVER_STOPPING: 20014		// 服务器即将停止
};

FPNNClient.prototype.connect = function () {
	try {
		this.connection = new WebSocket(this.endpoint);
	} catch (e) {
		if (this.sendQueue.length > 0) {
			var errorCode = FPNNClient.ErrorCode.FPNN_EC_CORE_INVALID_CONNECTION;
			FPNNClient._clearUnsentQuests(this.sendQueue, errorCode);
			this.sendQueue = new Array();
		}
		this.connection = undefined;
		return;
	}

	this.connection.fpnn_seqNum = 1;
	this.connection.binaryType = "arraybuffer";
	this.connection.fpnn_callbacks = new Object();
	this.connection.fpnn_sendQueue = this.sendQueue;
	this.sendQueue = new Array();

	if (this.processor instanceof Object) {
		this.connection.fpnn_processor = this.processor;
	}

	//-----------------[ For callbacks ]----------------//
	this.connection.fpnn_runCallback = function (key, errorCode, answerJson) {
		var cbPackage = this.fpnn_callbacks[key];
		if (cbPackage === undefined)
			return;
		
		var callback = cbPackage.callback;
		FPNNClient._runCallback(callback, errorCode, answerJson);
		delete this.fpnn_callbacks[key];
	};

	this.connection.fpnn_callCallback = function (seqNum, status, answerJson) {
		var errorCode = 0;
		if (status != 0 && status != 200)
			errorCode = answerJson.code;

		var key = FPNNClient._makeCallbackKey(seqNum);
		this.fpnn_runCallback(key, errorCode, answerJson);
	};

	this.connection.fpnn_clearCallbacks = function () {
		var arr = Object.keys(this.fpnn_callbacks);
		var errorCode = FPNNClient.ErrorCode.FPNN_EC_CORE_CONNECTION_CLOSED;

		for (var i = 0; i < arr.length; i++) {
			var key = arr[i];
			if (key.substr(0, 5) == 'fpnn_') {
				this.fpnn_runCallback(key, errorCode, undefined);
			}
		}
	};

	//-----------------[ For unsent quest & callback ]----------------//
	this.connection.fpnn_clearUnsentQuests = function () {
		var errorCode = FPNNClient.ErrorCode.FPNN_EC_CORE_INVALID_CONNECTION;
		FPNNClient._clearUnsentQuests(this.fpnn_sendQueue, errorCode);
		this.fpnn_sendQueue = new Array();
	};

	this.connection.fpnn_sendQuest = function (method, jsonObject, expire, callback, twoway) {
		var data;
		if (twoway === false) {
			if (callback) {
				var errorCode = FPNNClient.ErrorCode.FPNN_EC_CORE_UNKNOWN_ERROR;
				FPNNClient._runCallback(callback, errorCode, undefined);
				return;
			}
			data = FPNNClient._buildOnewayQuestPackage(method, jsonObject);
		} else {

			var newSeqNum = this.fpnn_seqNum;
			this.fpnn_seqNum += 1;

			if (this.fpnn_seqNum == Number.MAX_VALUE)
				this.fpnn_seqNum = 1;

			data = FPNNClient._buildTwowayQuestPackage(newSeqNum, method, jsonObject);

			if (callback !== undefined) {
				var cbPackage = new Object();
				cbPackage.callback = callback;
				cbPackage.expire = expire;
				
				var key = FPNNClient._makeCallbackKey(newSeqNum);
				this.fpnn_callbacks[key] = cbPackage;
			}
		}

		try {
			this.send(data);
		} catch (e) {
			var errorCode = FPNNClient.ErrorCode.FPNN_EC_CORE_CONNECTION_CLOSED;
			FPNNClient._runCallback(callback, errorCode, undefined);
			return;
		}
	}

	//-----------------[ For callbacks ]----------------//
	this.connection.onopen = function () {
		if (this.fpnn_processor && this.fpnn_processor.onConnected instanceof Function) {
			this.fpnn_processor.onConnected();
		}
		while (this.fpnn_sendQueue.length) {
			var elem = this.fpnn_sendQueue.pop();
			this.fpnn_sendQuest(elem.method, elem.jsonObject, elem.expire, elem.callback, elem.twoway);
		}
	};

	this.connection.onerror = function () {
		if (this.fpnn_processor && this.fpnn_processor.onClosed instanceof Function) {
			this.fpnn_processor.onError();
		}
	};

	this.connection.onclose = function () {
		this.fpnn_clearUnsentQuests();
		this.fpnn_clearCallbacks();

		if (this.fpnn_timerId !== undefined)
			clearInterval(this.fpnn_timerId);

		if (this.fpnn_processor && this.fpnn_processor.onClosed instanceof Function) {
			this.fpnn_processor.onClosed();
		}
	};

	this.connection.onmessage = function (msg) {
		if(msg.data instanceof ArrayBuffer) {
			var data = FPNNClient._unpack(msg.data);
			if (!data) {
				this.close();
				return;
			}

			if (data.status !== undefined) {	//-- Answer
				this.fpnn_callCallback(data.seqNum, data.status, data.payload);
			} else {	//-- Quest
				var answer = undefined;
				var exception = false;
				if (this.fpnn_processor[data.method] && this.fpnn_processor[data.method] instanceof Function) {
					answer = this.fpnn_processor[data.method](data.payload);
				} else if (data.seqNum !== undefined) {
					answer = new Object();
					answer.code = FPNNClient.ErrorCode.FPNN_EC_CORE_UNKNOWN_METHOD;
					answer.ex = 'FPNN_EC_CORE_UNKNOWN_METHOD';
					answer.raiser = 'FPNN WebSocket SDK';
					exception = true;
				}

				if (data.seqNum !== undefined && answer) {
					var data = FPNNClient._buildAnswerPackage(data.seqNum, answer, exception);

				try {
						this.send(data);
					} catch (e) {
						return;
					}
				}
			}
		}
		else
			this.close();
	};

	var self = this;
	this.connection.fpnn_timerId = setInterval(function() {
			self._checkExpire();
		}, 1000);
};

FPNNClient.prototype._checkExpire = function () {
	var errorCode = FPNNClient.ErrorCode.FPNN_EC_CORE_TIMEOUT;

	if (this.connection !== undefined) {
		//----------[ callback map ]-----------//
		var arr = Object.keys(this.connection.fpnn_callbacks);

		for (var i = 0; i < arr.length; i++) {
			var key = arr[i];
			if (key.substr(0, 5) == 'fpnn_') {
				var expire = this.connection.fpnn_callbacks[key].expire;

				if (expire <= Date.now())
					this.connection.fpnn_runCallback(key, errorCode, undefined);
			}
		}

		//----------[ wait sending quest ]-----------//
		for (var i = this.connection.fpnn_sendQueue.length - 1; i >= 0; i--) {
			var elem = this.connection.fpnn_sendQueue[i];
			if (elem.expire <= Date.now()) {
				FPNNClient._runCallback(elem.callback, errorCode, undefined);
				this.connection.fpnn_sendQueue.splice(i, 1);
			}
		}
	}

	//----------[ wait sending quest ]-----------//
	for (var i = this.sendQueue.length - 1; i >= 0; i--) {
		var elem = this.sendQueue[i];
		if (elem.expire <= Date.now()) {
			FPNNClient._runCallback(elem.callback, errorCode, undefined);
			this.sendQueue.splice(i, 1);
		}
	}
};

FPNNClient.prototype.sendNotify = function (method, jsonObject) {
	var requireConnect = false;
	var expire = this.defaultTimeout * 1000 + Date.now();

	if (this.connection === undefined || this.connection.readyState == WebSocket.CLOSED) {
		if (this.autoConnect)
			requireConnect = true;
		else
			return;
	}
	else if (this.connection.readyState == WebSocket.CONNECTING) {
		var elem = new Object();
		elem.method = method;
		elem.jsonObject = jsonObject;
		elem.callback = undefined;
		elem.twoway = false;
		elem.expire = expire;

		this.connection.fpnn_sendQueue.push(elem);
		return;
	}
	else if (this.connection.readyState == WebSocket.CLOSING) {
		return;
	}
	else {	//-- this.connection.readyState == WebSocket.OPEN
		this.connection.fpnn_sendQuest(method, jsonObject, expire, undefined, false);
		return;
	}

	if (requireConnect) {
		var elem = new Object();
		elem.method = method;
		elem.jsonObject = jsonObject;
		elem.callback = undefined;
		elem.twoway = false;
		elem.expire = expire;

		this.sendQueue.push(elem);
		this.connect();
		return;
	}
};

FPNNClient.prototype.sendQuest = function (method, jsonObject, callback, timeout) {
	var requireConnect = false;
	var errorCode = 0;

	if (timeout === undefined || timeout <= 0)
		timeout = this.defaultTimeout;

	var expire = timeout * 1000 + Date.now();

	if (this.connection === undefined || this.connection.readyState == WebSocket.CLOSED) {
		if (this.autoConnect)
			requireConnect = true;
		else
			errorCode = FPNNClient.ErrorCode.FPNN_EC_CORE_INVALID_CONNECTION;
	}
	else if (this.connection.readyState == WebSocket.CONNECTING) {
		var elem = new Object();
		elem.method = method;
		elem.jsonObject = jsonObject;
		elem.callback = callback;
		elem.twoway = true;
		elem.expire = expire;

		this.connection.fpnn_sendQueue.push(elem);
		return;
	}
	else if (this.connection.readyState == WebSocket.CLOSING) {
		errorCode = FPNNClient.ErrorCode.FPNN_EC_CORE_CONNECTION_CLOSED;
	}
	else {	//-- this.connection.readyState == WebSocket.OPEN
		this.connection.fpnn_sendQuest(method, jsonObject, expire, callback, true);
		return;
	}

	if (requireConnect) {
		var elem = new Object();
		elem.method = method;
		elem.jsonObject = jsonObject;
		elem.callback = callback;
		elem.twoway = true;
		elem.expire = expire;

		this.sendQueue.push(elem);
		this.connect();
		return;
	}

	if (errorCode > 0) {
		FPNNClient._runCallback(callback, errorCode, undefined);
	}
};

FPNNClient.prototype.close = function () {
	if (this.connection === undefined || this.connection.readyState == WebSocket.CLOSED)
		return;
	else
		this.connection.close();
};

FPNNClient._makeCallbackKey = function (seqNum) {
	return 'fpnn_' + String(seqNum);
};

FPNNClient._runCallback = function (callback, errorCode, answerJson) {
	if (callback instanceof Function) {
		callback(answerJson, errorCode);
	}
	else if (callback instanceof Object) {
		if (errorCode == 0 && callback.onAnswer) {
			callback.onAnswer(answerJson);
		}
		else if (callback.onException) {
			callback.onException(answerJson, errorCode);
		}
	}
};

FPNNClient._clearUnsentQuests = function (questInfoArray, errorCode) {
	if (errorCode === undefined) {
		errorCode = FPNNClient.ErrorCode.FPNN_EC_CORE_INVALID_CONNECTION;
	}
	if (questInfoArray instanceof Array) {
		questInfoArray.forEach(function (elem){
			FPNNClient._runCallback(elem.callback, errorCode, undefined);
		});
	}
};

FPNNClient._buildPackageHeader = function (byteArray) {
	byteArray[0] = 0x46;
	byteArray[1] = 0x50;
	byteArray[2] = 0x4e;
	byteArray[3] = 0x4e;

	byteArray[4] = 0x1;
	byteArray[5] = 0x40;
};

FPNNClient._buildTwowayQuestPackage = function (newSeqNum, method, jsonData) {
	if (jsonData === undefined)
		jsonData = "{}";

	if (typeof(jsonData) != "string")
		jsonData = JSON.stringify(jsonData);

	var buf = new ArrayBuffer(16 + method.length + jsonData.length);
	var byteArray = new Uint8Array(buf);

	FPNNClient._buildPackageHeader(byteArray);

	byteArray[6] = 1;
	byteArray[7] = method.length;

	var dataView = new DataView(buf);
	dataView.setUint32(8, jsonData.length, true);
	dataView.setUint32(12, newSeqNum, true);

	for (var i = 0; i < method.length; i++) {
		byteArray[16 + i] = method.charCodeAt(i);
	}

	for (var i = 0; i < jsonData.length; i++) {
		byteArray[16 + method.length + i] = jsonData.charCodeAt(i);
	}

	return buf;
};

FPNNClient._buildOnewayQuestPackage = function (method, jsonData) {
	if (jsonData === undefined)
		jsonData = "{}";

	if (typeof(jsonData) != "string")
		jsonData = JSON.stringify(jsonData);

	var buf = new ArrayBuffer(12 + method.length + jsonData.length);
	var byteArray = new Uint8Array(buf);

	FPNNClient._buildPackageHeader(byteArray);

	byteArray[6] = 0;
	byteArray[7] = method.length;

	var dataView = new DataView(buf);
	dataView.setUint32(8, jsonData.length, true);

	for (var i = 0; i < method.length; i++) {
		byteArray[12 + i] = method.charCodeAt(i);
	}

	for (var i = 0; i < jsonData.length; i++) {
		byteArray[12 + method.length + i] = jsonData.charCodeAt(i);
	}

	return buf;
};

FPNNClient._buildAnswerPackage = function (questSeqNum, jsonData, exception) {
	if (jsonData === undefined)
		jsonData = "{}";

	if (typeof(jsonData) != "string")
		jsonData = JSON.stringify(jsonData);

	var buf = new ArrayBuffer(16 + jsonData.length);
	var byteArray = new Uint8Array(buf);

	FPNNClient._buildPackageHeader(byteArray);
	byteArray[6] = 2;
	byteArray[7] = exception ? 1 : 0;

	var dataView = new DataView(buf);
	dataView.setUint32(8, jsonData.length, true);
	dataView.setUint32(12, questSeqNum, true);

	for (var i = 0; i < jsonData.length; i++) {
		byteArray[16 + i] = jsonData.charCodeAt(i);
	}

	return buf;
};

FPNNClient._unpackQuest = function (arrayBuffer, byteArray) {
	var dataView = new DataView(arrayBuffer);
	var payloadLen = dataView.getUint32(8, true);

	if (byteArray[6] == 1) {
		if (byteArray.length != (16 + byteArray[7] + payloadLen))
			return null;

		var seqNum = dataView.getUint32(12, true);
		var methodBuffer = new Uint8Array(arrayBuffer, 16, byteArray[7]);
		var method = String.fromCharCode.apply(null, methodBuffer);
		//var method = String.fromCharCode.apply(null, new Uint8Array(arrayBuffer, 16));
		//method = method.slice(0, byteArray[7]);
		var payload = String.fromCharCode.apply(null, new Uint8Array(arrayBuffer, 16 + byteArray[7]));

		return {
			seqNum: seqNum,
			method: method,
			payload: JSON.parse(payload)
		};

	} else {

		if (byteArray.length != (12 + byteArray[7] + payloadLen))
			return null;

		var methodBuffer = new Uint8Array(arrayBuffer, 12, byteArray[7]);
		var method = String.fromCharCode.apply(null, methodBuffer);
		//var method = String.fromCharCode.apply(null, new Uint8Array(arrayBuffer, 12));
		//method = method.slice(0, byteArray[7]);
		var payload = String.fromCharCode.apply(null, new Uint8Array(arrayBuffer, 12 + byteArray[7]));

		return {
			method: method,
			payload: JSON.parse(payload)
		};
	}
};

FPNNClient._unpackAnswer = function (arrayBuffer, byteArray) {
	if (byteArray.length <= 16)
		return null;

	var dataView = new DataView(arrayBuffer);
	var payloadLen = dataView.getUint32(8, true);
	var seqNum = dataView.getUint32(12, true);

	if (byteArray.length != (16 + payloadLen))
		return null;

	var status = byteArray[7];

	var payload = String.fromCharCode.apply(null, new Uint8Array(arrayBuffer, 16));

	return {
		seqNum: seqNum,
		status: status,
		payload: JSON.parse(payload)
	};
};

FPNNClient._unpack = function (arrayBuffer) {
	var byteArray = new Uint8Array(arrayBuffer);
	if (byteArray.length <= 12)
		return null;

	if (byteArray[0] != 0x46) return null;
	if (byteArray[1] != 0x50) return null;
	if (byteArray[2] != 0x4e) return null;
	if (byteArray[3] != 0x4e) return null;

	if (byteArray[4] != 0x1) return null;
	if (byteArray[5] != 0x40) return null;

	if (byteArray[6] == 2) return FPNNClient._unpackAnswer(arrayBuffer, byteArray);
	if (byteArray[6] < 2) return FPNNClient._unpackQuest(arrayBuffer, byteArray);
	return null;
};