#ifndef FPNN_FPNN_TCP_Client_h
#define FPNN_FPNN_TCP_Client_h

#include "../../../IOBuffer.h"
#include "../../../RawTransmission/RawTCPClient.h"
#include "FpnnClientCenter.h"
#include "FpnnTCPCodec.h"

namespace fpnn
{
	class FpnnTCPSuperQuestProcessor: public IQuestProcessor
	{
		QuestProcessorClassPrivateFields(FpnnTCPSuperQuestProcessor)
		
		IQuestProcessorPtr _realProcessor;

	public:
		virtual ~FpnnTCPSuperQuestProcessor() {}

		virtual void connected(const ConnectionInfo& info);
		virtual void connectionWillClose(const ConnectionInfo& info, bool closeByError);

		inline void setQuestProcessor(IQuestProcessorPtr processor)
		{
			_realProcessor = processor;
		}

		QuestProcessorClassBasicPublicFuncs
	};

	class FpnnTCPClient
	{
		std::shared_ptr<FpnnTCPSuperQuestProcessor> _superProcessor;
		std::shared_ptr<TCPFPNNProtocolProcessor> _rawDataProcessor;
		RawTCPClientPtr _client;
		bool _autoReconnect;

		bool sendQuestWithBasicAnswerCallback(FPQuestPtr quest, BasicAnswerCallback* callback, int msec_timeout);

	public:
		FpnnTCPClient(const std::string& endpoint, bool autoReconnect = true): _autoReconnect(autoReconnect)
		{
			_client = RawTCPClient::createClient(endpoint, autoReconnect);

			_rawDataProcessor = std::make_shared<TCPFPNNProtocolProcessor>();
			_client->setRawDataProcessor(_rawDataProcessor);		

			_superProcessor = std::make_shared<FpnnTCPSuperQuestProcessor>();
			_client->setQuestProcessor(_superProcessor);
		}

		virtual ~FpnnTCPClient()
		{
			close();
		}

		inline void setQuestProcessor(IQuestProcessorPtr processor)
		{
			_superProcessor->setQuestProcessor(processor);
			_rawDataProcessor->setQuestProcessor(processor);
		}

		inline bool connected() { return _client->connected(); }
		inline const std::string& endpoint() { return _client->endpoint(); }
		inline int socket() { return _client->socket(); }
		inline ConnectionInfoPtr connectionInfo() { return _client->connectionInfo(); }

		inline bool reconnect() { return _client->reconnect(); }
		inline bool connect() { return _client->connect(); }
		void close() { return _client->close(); }

		virtual FPAnswerPtr sendQuest(FPQuestPtr quest, int timeout = 0);
		virtual bool sendQuest(FPQuestPtr quest, AnswerCallback* callback, int timeout = 0);
		virtual bool sendQuest(FPQuestPtr quest, std::function<void (FPAnswerPtr answer, int errorCode)> task, int timeout = 0);
	};
}

#endif