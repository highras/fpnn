#ifndef FPNN_FPNN_UDP_Codec_h
#define FPNN_FPNN_UDP_Codec_h

#include "../../../IQuestProcessor.h"
#include "../../../UDP.v2/UDPIOBuffer.v2.h"
#include "../../../RawTransmission/RawTransmissionCommon.h"

namespace fpnn
{
	class UDPFPNNProtocolProcessor: public IRawDataProcessor, public std::enable_shared_from_this<UDPFPNNProtocolProcessor>
	{
		int _port;
		int _socket;
		std::mutex _mutex;
		std::string _ip;
		std::shared_ptr<UDPIOBuffer> _ioBuffer;
		IQuestProcessorPtr _questProcessor;
		std::unordered_map<uint32_t, BasicAnswerCallback*> _callbackMap;

		void clearCallbacks();
		void dealQuest(ConnectionInfoPtr connectionInfo, FPQuestPtr quest);
		void dealAnswer(ConnectionInfoPtr connectionInfo, FPAnswerPtr answer);
		bool sendQuestWithBasicAnswerCallback(FPQuestPtr quest, BasicAnswerCallback* callback, int msec_timeout);

	public:
		UDPFPNNProtocolProcessor(const std::string& ip, int port): _port(port), _socket(0), _ip(ip) {}
		virtual ~UDPFPNNProtocolProcessor()
		{
			close();
		}

		virtual void process(ConnectionInfoPtr connectionInfo, uint8_t* buffer, uint32_t len);
		inline void setQuestProcessor(IQuestProcessorPtr processor)
		{
			_questProcessor = processor;
		}
		inline IQuestProcessorPtr getQuestProcessor()
		{
			return _questProcessor;
		}

		void connect(int socket, bool isPrivateIP);
		void close();

		void cleanExpiredCallbacks();
		void sendData(std::string* data, int64_t msec_timeout = 0);
		void sendCacheData();

		virtual FPAnswerPtr sendQuest(FPQuestPtr quest, int timeout = 0);
		virtual bool sendQuest(FPQuestPtr quest, AnswerCallback* callback, int timeout = 0);
		virtual bool sendQuest(FPQuestPtr quest, std::function<void (FPAnswerPtr answer, int errorCode)> task, int timeout = 0);
	};
	typedef std::shared_ptr<UDPFPNNProtocolProcessor> UDPFPNNProtocolProcessorPtr;
}

#endif