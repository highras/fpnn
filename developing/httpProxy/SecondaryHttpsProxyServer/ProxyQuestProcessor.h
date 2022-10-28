#ifndef FPNN_Proxy_Quest_Processor_h
#define FPNN_Proxy_Quest_Processor_h

#include "IQuestProcessor.h"
#include "MultipleURLEngine.h"
#include "RawTransmission/RawTCPClient.h"

using namespace fpnn;

const int BufferSize = 4096;

class HttpsRawDataProcessor;
typedef std::shared_ptr<HttpsRawDataProcessor> HttpsRawDataProcessorPtr;

class HttpsRawDataProcessor: public IRawDataChargeProcessor, public std::enable_shared_from_this<HttpsRawDataProcessor>
{
	std::mutex _mutex;
	QuestSenderPtr _sender;

	bool _sendToken;
	uint32_t _seq;
	uint32_t _sessionId;
	uint8_t _buffer[BufferSize];
	std::list<ReceivedRawData*> _rawCache;
	std::map<uint32_t, FPQuestPtr> _questCache;

	bool sendData(uint8_t* buff, int size, QuestSenderPtr sender);
	bool resendCachedQuests(std::map<uint32_t, FPQuestPtr>& questCache, QuestSenderPtr sender);
	bool sendData(std::list<ReceivedRawData*>& dataList, QuestSenderPtr sender);

public:
	std::atomic<int64_t> activeTime;
	std::atomic<bool> requireUpdateSender;

public:
	HttpsRawDataProcessor(uint32_t sessionId, QuestSenderPtr sender):
		_sender(sender), _sendToken(true), _seq(0), _sessionId(sessionId), requireUpdateSender(false)
	{
		activeTime = slack_real_sec();
	}
	virtual ~HttpsRawDataProcessor()
	{
		for (auto data: _rawCache)
			delete data;
	}
	virtual void process(ConnectionInfoPtr connectionInfo, std::list<ReceivedRawData*>& dataList);
	bool checkUpdateSender();		//-- IF reutrn, send token is token.
	void updateQuestSenderAndSendData(QuestSenderPtr sender);
	void cache(uint32_t seq, FPQuestPtr quest, QuestSenderPtr sender);
};

struct ProxyInfo
{
	uint32_t nextSendSeq;
	std::mutex mutex;
	RawTCPClientPtr client;
	std::map<uint32_t, std::string*> cache;
	HttpsRawDataProcessorPtr rawProcessor;

	ProxyInfo(): nextSendSeq(1) {}
	~ProxyInfo()
	{
		for (auto& pp: cache)
			delete pp.second;
	}
};
typedef std::shared_ptr<ProxyInfo> ProxyInfoPtr;

class ProxyQuestProcessor: public IQuestProcessor
{
	QuestProcessorClassPrivateFields(ProxyQuestProcessor)

	MultipleURLEngine _urlEngine;
	std::mutex _mutex;
	std::map<uint32_t, ProxyInfoPtr> _httpsProxies;
	uint32_t _sessionIdBase;
	std::atomic<bool> _running;
	std::thread _loopThread;

	void loopThread();

public:
	FPAnswerPtr http(const FPReaderPtr args, const FPQuestPtr quest, const ConnectionInfo& ci);
	FPAnswerPtr https(const FPReaderPtr args, const FPQuestPtr quest, const ConnectionInfo& ci);
	FPAnswerPtr initHttps(const FPReaderPtr args, const FPQuestPtr quest, const ConnectionInfo& ci);
	FPAnswerPtr httpsClose(const FPReaderPtr args, const FPQuestPtr quest, const ConnectionInfo& ci);

	ProxyQuestProcessor(): _running(true)
	{
		MultipleURLEngine::init();

		registerMethod("http", &ProxyQuestProcessor::http);
		registerMethod("https", &ProxyQuestProcessor::https);
		registerMethod("initHttps", &ProxyQuestProcessor::initHttps);
		registerMethod("httpsClose", &ProxyQuestProcessor::httpsClose);

		_sessionIdBase = (uint32_t)slack_real_sec();
		_loopThread = std::thread(&ProxyQuestProcessor::loopThread, this);
	}

	~ProxyQuestProcessor()
	{
		_running = false;
		_loopThread.join();

		MultipleURLEngine::cleanup();
	}

	QuestProcessorClassBasicPublicFuncs
};

#endif