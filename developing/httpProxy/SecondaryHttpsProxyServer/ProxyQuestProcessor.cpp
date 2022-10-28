#include "FPLog.h"
#include "StringUtil.h"
#include "ProxyQuestProcessor.h"

FPAnswerPtr ProxyQuestProcessor::http(const FPReaderPtr args, const FPQuestPtr quest, const ConnectionInfo& ci)
{
	std::string method = args->wantString("method");
	if (method == "GET" || method == "get" || method.empty())
		;
	else if (method == "POST" || method == "post")
		;
	else
	{
		FPAWriter aw(3, quest);
		aw.param("httpCode", 501);
		aw.param("header", std::map<std::string, std::string>());
		aw.param("body", "<html><head><title>501 Not Implemented</title></head><body><h1>501 Not Implemented</h1></body></html>");
		
		return aw.take();
		//return FpnnErrorAnswer(quest, FPNN_EC_CORE_UNKNOWN_ERROR, std::string("Unsupported method: ").append(method));
	}

	std::string target = args->wantString("url");
	std::string body = args->wantString("body");

	std::vector<std::string> header;
	header = args->want("header", header);

	std::string url;
	const char* httpScheme = "http://";
	if (strncasecmp(target.c_str(), httpScheme, strlen(httpScheme)) == 0)
		url = target;
	else if (strncmp(target.c_str(), "//", 2) == 0)
		url.append("http:").append(target);
	else
		url.append(httpScheme).append(target);
		

	std::shared_ptr<IAsyncAnswer> async = genAsyncAnswer(quest);

	bool res = _urlEngine.visit(url, [async](MultipleURLEngine::Result &result) {
		
			std::map<std::string, std::string> headerMap;
			std::vector<std::string> headerList;
			std::string responseBody;

			int httpCode = result.responseCode;
			if (result.curlCode != CURLE_OK)
			{
				httpCode = 500;
				responseBody = "<html><head><title>500 Proxy Server Error</title></head><body><h1>Curl Code : ";
				responseBody.append(std::to_string(result.curlCode)).append("</h1></body></html>");
			}

			if (result.responseHeaderBuffer)
			{
				result.responseHeaderBuffer->getLines(headerList);
				for (auto& line: headerList)
				{
					std::vector<std::string> segments;
					StringUtil::split(line, ":", segments);

					if (segments.size() == 2)
						headerMap[segments[0]] = segments[1];
					else if (segments.size() > 2)
						headerMap[segments[0]] = line.substr(segments.size() + 1);
					else
						headerMap[segments[0]] = "";
				}
			}

			if (result.responseBuffer)
			{
				int len = result.responseBuffer->length();
				if (len > 0)
				{
					char *buf = (char*)malloc(len);
					result.responseBuffer->writeTo(buf, len, 0);
					responseBody.assign(buf, len);
					free(buf);
				}
			}

			FPAWriter aw(3, async->getQuest());
			aw.param("httpCode", httpCode);
			aw.param("header", headerMap);
			aw.param("body", responseBody);

			async->sendAnswer(aw.take());
		},
		120, true, body, header);

	if (!res)
		async->sendErrorAnswer(FPNN_EC_CORE_UNKNOWN_ERROR, "URL Engine visit failed.");

	return nullptr;
}

void ProxyQuestProcessor::loopThread()
{
	while (_running)
	{
		int cyc = 100;
		while (_running && cyc--)
			usleep(10000);

		std::set<uint32_t> expiredIds;
		const int64_t expiredSeconds = 600;
		int64_t threshold = slack_real_sec() - expiredSeconds;

		std::unique_lock<std::mutex> lck(_mutex);
		for (auto& pp: _httpsProxies)
		{
			if (pp.second->rawProcessor->activeTime <= threshold)
				expiredIds.insert(pp.first);
		}

		for (auto sid: expiredIds)
			_httpsProxies.erase(sid);
	}
}

void HttpsRawDataProcessor::cache(uint32_t seq, FPQuestPtr quest, QuestSenderPtr sender)
{
	std::unique_lock<std::mutex> lck(_mutex);
	_questCache[seq] = quest;

	if (sender.get() == _sender.get())
	{
		requireUpdateSender = true;
		_sender = nullptr;
	}
}

bool HttpsRawDataProcessor::sendData(uint8_t* buff, int size, QuestSenderPtr sender)
{
	uint32_t seq = ++_seq;

	FPQWriter qw(3, "httpsResponse");
	qw.param("sessionId", _sessionId);
	qw.param("seq", seq);
	qw.paramBinary("data", buff, size);

	FPQuestPtr quest = qw.take();
	HttpsRawDataProcessorPtr self = shared_from_this();

	bool status = sender->sendQuest(quest, [quest, size, self, seq, sender](FPAnswerPtr answer, int errorCode){
		if (errorCode != FPNN_EC_OK)
		{
			LOG_ERROR("Transfer data to browser failed. size %d, seq %u,error %d", size, seq, errorCode);
			self->cache(seq, quest, sender);
		}
		else
			self->activeTime = slack_real_sec();
	});

	if (!status)
	{
		LOG_ERROR("Transfer data to browser failed. size %d, seq %u", size, seq);
		cache(seq, quest, sender);
	}
	return status;
}

void HttpsRawDataProcessor::process(ConnectionInfoPtr, std::list<ReceivedRawData*>& dataList)
{
	QuestSenderPtr sender;
	{
		std::unique_lock<std::mutex> lck(_mutex);

		for (auto data: dataList)
			_rawCache.push_back(data);

		if (_sendToken == false)
			return;

		if (requireUpdateSender || !_sender)
			return;

		_sendToken = true;
		sender = _sender;

		dataList.clear();
		dataList.swap(_rawCache);
	}

	sendData(dataList, sender);

	std::unique_lock<std::mutex> lck(_mutex);
	_sendToken = true;
}

bool HttpsRawDataProcessor::checkUpdateSender()
{
	std::unique_lock<std::mutex> lck(_mutex);
	if (_sendToken == false)
		return false;

	if (requireUpdateSender || !_sender)
	{
		_sendToken = false;
		return true;
	}

	return false;
}

bool HttpsRawDataProcessor::resendCachedQuests(std::map<uint32_t, FPQuestPtr>& questCache, QuestSenderPtr sender)
{
	bool status = true;
	std::map<uint32_t, FPQuestPtr> dumpMap;
	HttpsRawDataProcessorPtr self = shared_from_this();

	for (auto& pp: questCache)
	{
		if (status)
		{
			uint32_t seq = pp.first;
			FPQuestPtr quest = pp.second;

			status = sender->sendQuest(quest, [quest, self, seq, sender](FPAnswerPtr answer, int errorCode){
				if (errorCode != FPNN_EC_OK)
				{
					LOG_ERROR("Transfer cached data to browser failed. seq %u, error %d", seq, errorCode);
					self->cache(seq, quest, sender);
				}
				else
					self->activeTime = slack_real_sec();
			});
			if (!status)
				cache(seq, quest, sender);
		}
		else
		{
			dumpMap[pp.first] = pp.second;
		}
	}
	
	if (dumpMap.size())
	{
		std::unique_lock<std::mutex> lck(_mutex);
		for (auto& pp: dumpMap)
			_questCache[pp.first] = pp.second;
	}

	return status;
}

bool HttpsRawDataProcessor::sendData(std::list<ReceivedRawData*>& dataList, QuestSenderPtr sender)
{
	uint32_t bufOffset = 0;

	bool success = true;
	std::list<ReceivedRawData*> dumpList;

	for (auto data: dataList)
	{
		if (success)
		{
			if (BufferSize - bufOffset >= data->len)
			{
				memcpy(_buffer + bufOffset, data->data, data->len);
				bufOffset += data->len;
				delete data;
			}
			else
			{
				if (bufOffset)
				{
					success = sendData(_buffer, (int)bufOffset, sender);
					bufOffset = 0;
				}
				
				if (success)
				{
					if (BufferSize >= data->len)
					{
						memcpy(_buffer, data->data, data->len);
						bufOffset = data->len;
						delete data;
					}
					else
					{
						success = sendData(data->data, (int)(data->len), sender);
						delete data;
					}
				}
				else
					dumpList.push_back(data);
			}
		}
		else
			dumpList.push_back(data);
	}

	if (success && bufOffset)
	{
		success = sendData(_buffer, (int)bufOffset, sender);
	}

	if (!success)
	{
		std::unique_lock<std::mutex> lck(_mutex);
		for (auto data: _rawCache)
			dumpList.push_back(data);

		dumpList.swap(_rawCache);
	}
	
	return success;
}

void HttpsRawDataProcessor::updateQuestSenderAndSendData(QuestSenderPtr sender)
{
	std::list<ReceivedRawData*> rawCache;
	std::map<uint32_t, FPQuestPtr> questCache;

	{
		std::unique_lock<std::mutex> lck(_mutex);
		if (_sendToken == false)
			return;

		requireUpdateSender = false;
		_sender = sender;

		rawCache.swap(_rawCache);
		questCache.swap(_questCache);
	}

	bool success = resendCachedQuests(questCache, sender);
	if (!success)
	{
		std::unique_lock<std::mutex> lck(_mutex);
		if (rawCache.size())
		{
			for (auto data: _rawCache)
				rawCache.push_back(data);

			rawCache.swap(_rawCache);
		}

		_sendToken = true;
		return;
	}

	sendData(rawCache, sender);

	std::unique_lock<std::mutex> lck(_mutex);
	_sendToken = true;
}


FPAnswerPtr ProxyQuestProcessor::initHttps(const FPReaderPtr args, const FPQuestPtr quest, const ConnectionInfo& ci)
{
	std::string host = args->wantString("host");
	uint32_t sessionId;

	{
		std::unique_lock<std::mutex> lck(_mutex);
		sessionId = _sessionIdBase++;
	}

	if (host.find(':') == std::string::npos)
		host.append(":443");

	RawTCPClientPtr client = RawTCPClient::createClient(host);

	QuestSenderPtr sender = genQuestSender(ci);
	HttpsRawDataProcessorPtr processor = std::make_shared<HttpsRawDataProcessor>(sessionId, sender);

	client->setRawDataProcessor(processor);

	if (client->connect())
	{
		ProxyInfoPtr proxyInfo(new ProxyInfo);
		proxyInfo->client = client;
		proxyInfo->rawProcessor = processor;

		{
			std::unique_lock<std::mutex> lck(_mutex);
			_httpsProxies[sessionId] = proxyInfo;
		}

		return FPAWriter(1, quest)("sessionId", sessionId);
	}

	return FpnnErrorAnswer(quest, FPNN_EC_CORE_INVALID_CONNECTION, std::string("Connect host ").append(host).append(" failed."));
}

FPAnswerPtr ProxyQuestProcessor::https(const FPReaderPtr args, const FPQuestPtr quest, const ConnectionInfo& ci)
{
	uint32_t sessionId = args->wantInt("sessionId");
	uint32_t seq = args->wantInt("seq");
	std::string data = args->wantString("data");

	ProxyInfoPtr proxy;
	{
		std::unique_lock<std::mutex> lck(_mutex);
		auto it = _httpsProxies.find(sessionId);
		if (it != _httpsProxies.end())
			proxy = it->second;
	}

	if (!proxy)
	{
		return FpnnErrorAnswer(quest, FPNN_EC_CORE_INVALID_CONNECTION, std::string("Session ").append(std::to_string(sessionId)).append(" is unfound."));
	}

	{
		std::string* rawData = new std::string();
		rawData->swap(data);

		proxy->rawProcessor->activeTime = slack_real_sec();

		std::unique_lock<std::mutex> lck(proxy->mutex);
		if (proxy->nextSendSeq == seq)
		{
			proxy->client->sendData(rawData);
			proxy->nextSendSeq += 1;

			bool goon = true;
			while (goon)
			{
				auto it = proxy->cache.find(proxy->nextSendSeq);
				goon = (it != proxy->cache.end());
				if (goon)
				{
					proxy->client->sendData(it->second);
					proxy->cache.erase(proxy->nextSendSeq);
					proxy->nextSendSeq += 1;
				}
			}
		}
		else
		{
			proxy->cache[seq] = rawData;
		}
	}
	
	return FPAWriter::emptyAnswer(quest);
}

FPAnswerPtr ProxyQuestProcessor::httpsClose(const FPReaderPtr args, const FPQuestPtr quest, const ConnectionInfo& ci)
{
	uint32_t sessionId = args->wantInt("sessionId");

	std::unique_lock<std::mutex> lck(_mutex);
	_httpsProxies.erase(sessionId);

	return FPAWriter::emptyAnswer(quest);
}