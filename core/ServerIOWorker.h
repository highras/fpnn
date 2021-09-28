#ifndef FPNN_Server_IO_Worker_H
#define FPNN_Server_IO_Worker_H

#include <unistd.h>
#ifndef __APPLE__
	#include <sys/epoll.h>
#endif
#include <atomic>
#include <string>
#include <queue>
#include <mutex>
#include <memory>
#include "IOWorker.h"
#include "ParamTemplateThreadPoolArray.h"
#include "AnswerCallbacks.h"

namespace fpnn
{
	class TCPServerConnection;
	typedef std::shared_ptr<TCPServerConnection> TCPServerConnectionPtr;

	namespace IOEventType
	{
		const int None = 0x0;
		const int Connected = 0x1;
		const int Send = 0x2;
		const int Recv = 0x4;
		const int Closed = 0x8;
		const int Error = 0x10;
	}

	class TCPServerConnection: public TCPBasicConnection
	{
	private:
#ifdef __APPLE__
		int _kqueue_fd;
#else
		int _epoll_fd;
#endif
		bool _joined;

	public:
		bool _disposable;
		bool _requireClose;

		virtual bool waitForAllEvents();
		virtual enum ConnectionType connectionType() { return TCPBasicConnection::TCPServerConnectionType; }

		bool joinEpoll();
		bool waitForRecvEvent();
		void exitEpoll();

		virtual int send(bool& needWaitSendEvent, std::string* data = NULL)
		{
			bool actualSent = false;
			//-- _activeTime vaule maybe in confusion after concurrent Sending on one connection.
			//-- But the probability is very low even server with high load. So, it hasn't be adjusted at current.
			_activeTime = slack_real_sec();
			int err = _sendBuffer.send(_connectionInfo->socket, needWaitSendEvent, actualSent, data);
			if (_disposable && err == 0 && actualSent && !needWaitSendEvent)
			{
				_requireClose = true;
				needWaitSendEvent = true;
			}
			return err;
		}

		void closeAfterSent(bool& needWaitSendEvent)
		{
			_disposable = true;
			_sendBuffer.stopAppendData();
			send(needWaitSendEvent);
		}
#ifdef __APPLE__
		TCPServerConnection(int kqueuefd, std::mutex* mutex, int ioChunkSize, ConnectionInfoPtr connectionInfo):
			TCPBasicConnection(mutex, ioChunkSize, connectionInfo), _kqueue_fd(kqueuefd), _joined(false), _disposable(false), _requireClose(false)
#else
		TCPServerConnection(int epollfd, std::mutex* mutex, int ioChunkSize, ConnectionInfoPtr connectionInfo):
			TCPBasicConnection(mutex, ioChunkSize, connectionInfo), _epoll_fd(epollfd), _joined(false), _disposable(false), _requireClose(false)
#endif
		{
			_connectionInfo->token = (uint64_t)this; //-- if using Virtual Derive, must do this. Else, this is just redo the action in base class.
		}

		~TCPServerConnection() {}
	};

	struct RequestPackage
	{
		int _ioEvent;
		ConnectionInfoPtr _connectionInfo;
		TCPServerConnection* _connection;		//-- just for Connect, Error, Close event.
		FPQuestPtr _quest;

		RequestPackage(int event, ConnectionInfoPtr info, FPQuestPtr quest):
			_ioEvent(event), _connectionInfo(info), _connection(nullptr), _quest(quest) {}
		RequestPackage(int event, ConnectionInfoPtr info, TCPServerConnection* ioPackage):
			_ioEvent(event), _connectionInfo(info), _connection(ioPackage), _quest(nullptr) {}
	};

	class TCPEpollServer;
	class TCPServerIOWorker: public ParamTemplateThreadPool<TCPServerConnection *>::IProcessor
	{
		volatile bool _serverIsStopping;		//-- If std::atomic<bool> is not good, we test volatile key word at first here.
		TCPEpollServer* _server;
		std::shared_ptr<ParamTemplateThreadPoolArray<RequestPackage *>> _workerPool;

		bool read(TCPServerConnection * connection, bool& additionalSend);
		bool sendData(TCPServerConnection * connection, bool& fdInvalid, bool& needWaitSendEvent);

		bool deliverAnswer(TCPServerConnection * connection, FPAnswerPtr answer);
		bool deliverQuest(TCPServerConnection * connection, FPQuestPtr quest, bool& additionalSend);	//-- additionalSend: don't assign false.
		bool processECDH(TCPServerConnection * connection, FPQuestPtr quest);  //-- ECDH: Elliptic Curve Diffie–Hellman key Exchange
		bool processPing(TCPServerConnection * connection, FPQuestPtr quest);  //-- For keep alive.
		bool returnServerStoppingAnswer(TCPServerConnection * connection, FPQuestPtr quest);

		void closeConnection(TCPServerConnection * connection, bool error);

	public:
		TCPServerIOWorker(): _serverIsStopping(false) {}
		TCPServerIOWorker(std::shared_ptr<ParamTemplateThreadPoolArray<RequestPackage *>> workerPool): _workerPool(workerPool) {}

		virtual ~TCPServerIOWorker() {}
		virtual void run(TCPServerConnection * connection);

		inline void setWorkerPool(std::shared_ptr<ParamTemplateThreadPoolArray<RequestPackage *>> workerPool) { _workerPool = workerPool; }
		inline void setServer(TCPEpollServer* server) { _server = server; }
		inline void serverWillStop() { _serverIsStopping = true; }
	};
}

#endif
