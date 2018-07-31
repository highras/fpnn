#ifndef FPNN_Client_IO_Worker_H
#define FPNN_Client_IO_Worker_H

#include "ParamTemplateThreadPoolArray.h"
#include "IOWorker.h"

namespace fpnn
{
	class TCPClient;
	typedef std::shared_ptr<TCPClient> TCPClientPtr;

	class TCPClientConnection;
	typedef std::shared_ptr<TCPClientConnection> TCPClientConnectionPtr;
	
	class TCPClientConnection: public TCPBasicConnection
	{
	private:
		std::weak_ptr<TCPClient> _client;

	public:
		virtual bool waitForAllEvents();
		virtual enum ConnectionType connectionType() { return BasicConnection::TCPClientConnectionType; }
		TCPClientPtr client() { return _client.lock(); }

		TCPClientConnection(TCPClientPtr client, std::mutex* mutex, int ioChunkSize, ConnectionInfoPtr connectionInfo):
			TCPBasicConnection(mutex, ioChunkSize, connectionInfo), _client(client)
		{
			_connectionInfo->token = (uint64_t)this;	//-- if using Virtual Derive, must do this. Else, this is just redo the action in base class.
		}

		~TCPClientConnection() {}
	};

	class TCPClientIOWorker: public ParamTemplateThreadPool<TCPClientConnection *>::IProcessor
	{
		bool read(TCPClientConnection * connection);
		bool deliverAnswer(TCPClientConnection * connection, FPAnswerPtr answer);
		bool deliverQuest(TCPClientConnection * connection, FPQuestPtr quest);

	public:
		TCPClientIOWorker() {}
		virtual ~TCPClientIOWorker() {}
		virtual void run(TCPClientConnection * connection);
	};
}

#endif
