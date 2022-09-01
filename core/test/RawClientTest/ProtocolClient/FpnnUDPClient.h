#ifndef FPNN_FPNN_UDP_Client_h
#define FPNN_FPNN_UDP_Client_h

#include "../../../RawTransmission/RawUDPClient.h"
#include "FpnnClientCenter.h"
#include "FpnnUDPCodec.h"

namespace fpnn
{
	class FpnnUDPSuperQuestProcessor: public IQuestProcessor
	{
		QuestProcessorClassPrivateFields(FpnnUDPSuperQuestProcessor)

		IQuestProcessorPtr _realProcessor;
		UDPFPNNProtocolProcessorPtr _rawDataProcessor;

	public:
		virtual ~FpnnUDPSuperQuestProcessor() {}

		virtual void connected(const ConnectionInfo& info);
		virtual void connectionWillClose(const ConnectionInfo& info, bool closeByError);

		inline void setQuestProcessor(IQuestProcessorPtr processor)
		{
			_realProcessor = processor;
		}

		inline void setUDPProtocolProcessor(UDPFPNNProtocolProcessorPtr processor)
		{
			_rawDataProcessor = processor;
		}

		QuestProcessorClassBasicPublicFuncs
	};
	
	class FpnnUDPClient
	{
		std::shared_ptr<FpnnUDPSuperQuestProcessor> _superProcessor;
		std::shared_ptr<UDPFPNNProtocolProcessor> _rawDataProcessor;
		RawUDPClientPtr _client;
		bool _autoReconnect;

	public:
		FpnnUDPClient(const std::string& endpoint, bool autoReconnect = true): _autoReconnect(autoReconnect)
		{
			_client = RawUDPClient::createClient(endpoint, autoReconnect);

			ConnectionInfoPtr connInfo = _client->connectionInfo();

			_rawDataProcessor = std::make_shared<UDPFPNNProtocolProcessor>(connInfo->ip, connInfo->port);
			_client->setRawDataProcessor(_rawDataProcessor);		

			_superProcessor = std::make_shared<FpnnUDPSuperQuestProcessor>();
			_client->setQuestProcessor(_superProcessor);

			_superProcessor->setUDPProtocolProcessor(_rawDataProcessor);

			ClientCenter::registerUDPProcessor(_rawDataProcessor);
		}

		virtual ~FpnnUDPClient()
		{
			close();
			ClientCenter::unregisterUDPProcessor(_rawDataProcessor);
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