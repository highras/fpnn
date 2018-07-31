#ifndef FPNN_UDP_Client_IO_Worker_H
#define FPNN_UDP_Client_IO_Worker_H

#include "ParamTemplateThreadPoolArray.h"
#include "UDPClientIOBuffer.h"
#include "IOWorker.h"

namespace fpnn
{
	class UDPClient;
	typedef std::shared_ptr<UDPClient> UDPClientPtr;

	class UDPClientConnection;
	typedef std::shared_ptr<UDPClientConnection> UDPClientConnectionPtr;
	
	class UDPClientConnection: public BasicConnection
	{
	private:
		std::mutex* _mutex;		//-- only using for sendBuffer and sendToken
		bool _recvToken;

		std::weak_ptr<UDPClient> _client;
		UDPClientSendBuffer _sendBuffer;

	public:
		virtual bool waitForAllEvents();
		virtual enum ConnectionType connectionType() { return BasicConnection::UDPClientConnectionType; }
		UDPClientPtr client() { return _client.lock(); }

		UDPClientConnection(UDPClientPtr client, std::mutex* mutex, ConnectionInfoPtr connectionInfo):
			BasicConnection(connectionInfo), _mutex(mutex), _recvToken(true), _client(client), _sendBuffer(mutex, connectionInfo)
		{
			//-- This is different from UDP Server connection using.
			_connectionInfo->token = (uint64_t)this;	//-- if using Virtual Derive, must do this. Else, this is just redo the action in base class.
		}
		~UDPClientConnection() {}

		virtual int send(bool& needWaitSendEvent, std::string* data = NULL)
		{
			bool actualSent = false;
			//-- _activeTime vaule maybe in confusion after concurrent Sending on one connection.
			//-- But the probability is very low even server with high load. So, it hasn't be adjusted at current.
			_activeTime = slack_real_sec();
			_sendBuffer.send(needWaitSendEvent, actualSent, data);
			return 0;
		}

		void sendData(std::string* data)	//-- using by UDP client.
		{
			bool needWaitSendEvent = false;
			send(needWaitSendEvent, data);
			if (needWaitSendEvent)
				waitForAllEvents();
		}

		inline bool getRecvToken()
		{
			std::unique_lock<std::mutex> lck(*_mutex);
			if (!_recvToken)
				return false;

			_recvToken = false;
			return true;
		}
		inline void returnRecvToken()
		{
			std::unique_lock<std::mutex> lck(*_mutex);
			_recvToken = true;
		}
	};

	class UDPClientIOWorker: public ParamTemplateThreadPool<UDPClientConnection *>::IProcessor
	{
		void read(UDPClientConnection * connection);
		bool deliverAnswer(UDPClientConnection * connection, FPAnswerPtr answer);
		bool deliverQuest(UDPClientConnection * connection, FPQuestPtr quest);

	public:
		UDPClientIOWorker() {}
		virtual ~UDPClientIOWorker() {}
		virtual void run(UDPClientConnection * connection);
	};
}

#endif
