#ifndef UDP_Connection_h
#define UDP_Connection_h

#include "ParamTemplateThreadPoolArray.h"
#include "IOWorker.h"
#include "UDPIOBuffer.h"

namespace fpnn
{
	class UDPClient;
	typedef std::shared_ptr<UDPClient> UDPClientPtr;

	class UDPClientConnection: public BasicConnection
	{
		std::mutex _mutex;
		UDPIOBuffer _ioBuffer;
		std::weak_ptr<UDPClient> _client;
		IQuestProcessorPtr _questProcessor;		//-- Only used when active closed.

	public:
		UDPClientConnection(UDPClientPtr client, ConnectionInfoPtr connectionInfo, int MTU):
			BasicConnection(connectionInfo), _ioBuffer(NULL, connectionInfo->socket, MTU), _client(client)
		{
			_connectionInfo->token = (uint64_t)this;	//-- if use Virtual Derive, must redo this in subclass constructor.
			_connectionInfo->_mutex = &_mutex;
			_ioBuffer.initMutex(&_mutex);
			_ioBuffer.updateEndpointInfo(_connectionInfo->endpoint());
		}

		virtual ~UDPClientConnection() {}

		virtual bool waitForAllEvents();
		virtual enum ConnectionType connectionType() { return BasicConnection::UDPClientConnectionType; }
		virtual int send(bool& needWaitSendEvent, std::string* data = NULL);
		UDPClientPtr client() { return _client.lock(); }
		inline IQuestProcessorPtr questProcessor() { return _questProcessor; }

		inline void enableKeepAlive() { _ioBuffer.enableKeepAlive(); }
		inline bool isRequireClose() { return (_ioBuffer.isRequireClose() ? true : _ioBuffer.isTransmissionStopped()); }
		inline void setUntransmittedSeconds(int untransmittedSeconds) { _ioBuffer.setUntransmittedSeconds(untransmittedSeconds); }
		void sendCachedData(bool& needWaitSendEvent, bool socketReady = false);
		void sendData(bool& needWaitSendEvent, std::string* data, int64_t expiredMS, bool discardable);
		void sendCloseSignal(bool& needWaitSendEvent, IQuestProcessorPtr processor)
		{
			_ioBuffer.sendCloseSignal(needWaitSendEvent);
			_questProcessor = processor;
		}

		inline bool getRecvToken() { return _ioBuffer.getRecvToken(); }
		inline void returnRecvToken() { return _ioBuffer.returnRecvToken(); }
		bool recvData(std::list<FPQuestPtr>& questList, std::list<FPAnswerPtr>& answerList);
	};

	class UDPClientIOWorker: public ParamTemplateThreadPool<UDPClientConnection *>::IProcessor
	{
		void read(UDPClientConnection * connection);
		bool deliverAnswer(UDPClientConnection * connection, FPAnswerPtr answer);
		bool deliverQuest(UDPClientConnection * connection, FPQuestPtr quest);
		void closeConnection(UDPClientConnection * connection);

	public:
		UDPClientIOWorker() {}
		virtual ~UDPClientIOWorker() {}
		virtual void run(UDPClientConnection * connection);
	};
}

#endif