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

	//=================================================================//
	//- TCP Client Keep Alive Structures
	//=================================================================//
	struct TCPClientKeepAliveParams
	{
		int pingTimeout;			//-- In milliseconds
		int pingInterval;			//-- In milliseconds
		int maxPingRetryCount;

		virtual void config(const TCPClientKeepAliveParams* params)
		{
			pingTimeout = params->pingTimeout;
			pingInterval = params->pingInterval;
			maxPingRetryCount = params->maxPingRetryCount;
		}
		virtual ~TCPClientKeepAliveParams() {}
	};

	struct TCPClientKeepAliveInfos: public TCPClientKeepAliveParams
	{
	private:
		int unreceivedThreshold;
		int64_t lastReceivedMS;
		int64_t lastPingSentMS;

	public:
		TCPClientKeepAliveInfos(): lastPingSentMS(0)
		{
			lastReceivedMS = slack_real_msec();
		}
		virtual ~TCPClientKeepAliveInfos() {}

		virtual void config(const TCPClientKeepAliveParams* params)
		{
			TCPClientKeepAliveParams::config(params);
			unreceivedThreshold = pingTimeout * maxPingRetryCount + pingInterval;
		}

		inline void updateReceivedMS() { lastReceivedMS = slack_real_msec(); }
		inline void updatePingSentMS() { lastPingSentMS = slack_real_msec(); }
		inline int isRequireSendPing()	//-- If needed, return timeout; else, return 0.
		{
			int64_t now = slack_real_msec();
			if ((now >= lastReceivedMS + pingInterval) && (now >= lastPingSentMS + pingTimeout))
				return pingTimeout;
			else
				return 0;
		}

		inline bool isLost()
		{
			return (slack_real_msec() > (lastReceivedMS + unreceivedThreshold));
		}
	};

	class KeepAliveCallback: public AnswerCallback
	{
		ConnectionInfoPtr _connectionInfo;

	public:
		KeepAliveCallback(ConnectionInfoPtr ci): _connectionInfo(ci) {}

		virtual void onAnswer(FPAnswerPtr) {}
		virtual void onException(FPAnswerPtr answer, int errorCode)
		{
			LOG_ERROR("Keep alive ping for %s failed. Code: %d, infos: %s",
				_connectionInfo->str().c_str(), errorCode, answer ? answer->json().c_str() : "<N/A>");
		}
	};

	struct TCPClientSharedKeepAlivePingDatas
	{
		FPQuestPtr quest;
		std::string* rawData;
		uint32_t seqNum;

		TCPClientSharedKeepAlivePingDatas(): rawData(0) {}
		~TCPClientSharedKeepAlivePingDatas() { if (rawData) delete rawData; }

		void build()
		{
			if (!quest)
			{
				quest = FPQWriter::emptyQuest("*ping");
				rawData = quest->raw();
				seqNum = quest->seqNumLE();
			}
		}
	};
	
	//=================================================================//
	//- TCPClientConnection
	//=================================================================//
	class TCPClientConnection: public TCPBasicConnection
	{
	private:
		std::mutex _mutex;
		std::weak_ptr<TCPClient> _client;
		TCPClientKeepAliveInfos* _keepAliveInfos;

	public:
		virtual bool waitForAllEvents();
		virtual enum ConnectionType connectionType() { return BasicConnection::TCPClientConnectionType; }
		TCPClientPtr client() { return _client.lock(); }

		TCPClientConnection(TCPClientPtr client, int ioChunkSize, ConnectionInfoPtr connectionInfo):
			TCPBasicConnection(NULL, ioChunkSize, connectionInfo), _client(client), _keepAliveInfos(NULL)
		{
			_connectionInfo->token = (uint64_t)this;	//-- if using Virtual Derive, must do this. Else, this is just redo the action in base class.
			resetMutex(&_mutex);
		}

		~TCPClientConnection()
		{
			if (_keepAliveInfos)
				delete _keepAliveInfos;
		}

		void configKeepAlive(const TCPClientKeepAliveParams* params)
		{
			if (_keepAliveInfos == NULL)
				_keepAliveInfos = new TCPClientKeepAliveInfos;
			
			_keepAliveInfos->config(params);
		}

		int isRequireKeepAlive(bool& isLost)
		{
			if (!_keepAliveInfos)
			{
				isLost = false;
			}
			else
			{
				isLost = _keepAliveInfos->isLost();
				if (!isLost)
					return _keepAliveInfos->isRequireSendPing();
			}

			return 0;
		}

		inline void updateKeepAliveMS()
		{
			_keepAliveInfos->updatePingSentMS();
		}

		inline void updateReceivedMS()
		{
			if (_keepAliveInfos)
				_keepAliveInfos->updateReceivedMS();
		}
	};

	//=================================================================//
	//- TCPClientIOWorker
	//=================================================================//
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
