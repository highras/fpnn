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
		std::mutex _mutex;
		std::weak_ptr<TCPClient> _client;

	public:
		virtual bool waitForAllEvents();
		virtual enum ConnectionType connectionType() { return BasicConnection::TCPClientConnectionType; }
		TCPClientPtr client() { return _client.lock(); }

		TCPClientConnection(TCPClientPtr client, int ioChunkSize, ConnectionInfoPtr connectionInfo):
			TCPBasicConnection(NULL, ioChunkSize, connectionInfo), _client(client)
		{
			_connectionInfo->token = (uint64_t)this;	//-- if using Virtual Derive, must do this. Else, this is just redo the action in base class.
			resetMutex(&_mutex);
		}

		~TCPClientConnection() {}
	};

	class TCPClientIOWorker: public ParamTemplateThreadPool<TCPClientConnection *>::IProcessor
	{
		bool read(TCPClientConnection * connection);
		bool deliverAnswer(TCPClientConnection * connection, FPAnswerPtr answer);
		bool deliverQuest(TCPClientConnection * connection, FPQuestPtr quest);
		void closeConnection(TCPClientConnection * connection);

	public:
		TCPClientIOWorker() {}
		virtual ~TCPClientIOWorker() {}
		virtual void run(TCPClientConnection * connection);
	};
}

#endif
