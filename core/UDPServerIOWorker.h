#ifndef UDP_Server_IO_Worker_h
#define UDP_Server_IO_Worker_h

#include <list>
#include <unordered_map>
#include <arpa/inet.h>
#include "ParamTemplateThreadPoolArray.h"
#include "IOWorker.h"
#include "UDP.v2/UDPIOBuffer.v2.h"

namespace fpnn
{
	struct UDPRawBuffer
	{
		uint8_t* buffer;
		int len;

		UDPRawBuffer(int len_): len(len_)
		{
			buffer = (uint8_t*)malloc(len_);
		}
		~UDPRawBuffer() { free(buffer); }
	};

	class UDPServerReceiver
	{
		const int _UDPMaxDataLen;			//-- Without IPv6 jumbogram.
		uint8_t* _buffer;

		struct sockaddr_in _addr;
		struct sockaddr_in6 _addr6;
		socklen_t _addrlen;
		socklen_t _addrlen6;

	public:
		//-- Output area:
		bool isIPv4;
		std::string ip;
		int port;

		void* sockaddr;
		socklen_t requiredAddrLen;

		UDPRawBuffer* udpRawData;

	public:
		UDPServerReceiver(): _UDPMaxDataLen(FPNN_UDP_MAX_DATA_LENGTH)
		{
			_buffer = (uint8_t*)malloc(_UDPMaxDataLen);
			_addrlen = (socklen_t)sizeof(struct sockaddr_in);
			_addrlen6 = (socklen_t)sizeof(struct sockaddr_in6);
		}
		~UDPServerReceiver()
		{
			free(_buffer);
		}

		bool recvIPv4(int socket);
		bool recvIPv6(int socket);
	};

	enum class UDPConnectionEventStatus
	{
		UnCalling,
		Calling,
		Called
	};

	class UDPServerConnection: public BasicConnection
	{
		std::mutex _mutex;
		UDPIOBuffer _ioBuffer;

		bool _requireCallCloseEvent;
		volatile bool _requireClose;
		UDPConnectionEventStatus _connectedEventStatus;
		std::list<UDPRawBuffer*> _rawData;
		std::list<FPQuestPtr> _questCache;

	public:
#ifdef __APPLE__
		int kqueuefd;
#else
		int epollfd;
#endif
		int64_t sendingTurn;					//-- Only operated by UDPServerConnectionMap.
		volatile bool firstSchedule;
		volatile bool connectedEventCalled;

	public:
		UDPServerConnection(ConnectionInfoPtr connectionInfo, int MTU):
			BasicConnection(connectionInfo), _ioBuffer(NULL, connectionInfo->socket, MTU),
			_requireCallCloseEvent(false), _requireClose(false),
			_connectedEventStatus(UDPConnectionEventStatus::UnCalling), sendingTurn(0),
			firstSchedule(true), connectedEventCalled(false)
		{
			_connectionInfo->token = (uint64_t)this;	//-- if use Virtual Derive, must redo this in subclass constructor.
			_connectionInfo->_mutex = &_mutex;
			_ioBuffer.initMutex(&_mutex);
			_ioBuffer.updateEndpointInfo(_connectionInfo->endpoint());

			bool replace = Config::UDP::_server_connection_reentry_replace_for_all_ip
				|| (Config::UDP::_server_connection_reentry_replace_for_private_ip
				&& _connectionInfo->isPrivateIP());
				
			_ioBuffer.configConnectionReentry(replace);
		}

		virtual ~UDPServerConnection()
		{
			for (auto rawData: _rawData)
				delete rawData;
		}

		inline bool isEncrypted() { return _ioBuffer.isEncrypted(); }
		inline void setKeyExchanger(ECCKeyExchange* exchanger) { _ioBuffer.setKeyExchanger(exchanger); }

		virtual bool waitForAllEvents();
		virtual enum ConnectionType connectionType() { return BasicConnection::UDPServerConnectionType; }
		virtual int send(bool& needWaitSendEvent, std::string* data = NULL);

		inline void appendRawData(UDPRawBuffer* data)
		{
			// std::unique_lock<std::mutex> lck(_mutex);
			_rawData.push_back(data);
		}
		void appendQuest(std::list<FPQuestPtr>& questList);
		inline void swapCachedQuests(std::list<FPQuestPtr>& cachedQuests)
		{
			std::unique_lock<std::mutex> lck(_mutex);
			_questCache.swap(cachedQuests);
		}
		void parseCachedRawData(bool& requireClose, bool& requireConnectedEvent);

		inline void markActiveCloseSignal() { _ioBuffer.markActiveCloseSignal(); }
		inline void requireClose() { _requireClose = true; }
		inline bool isRequireClose()
		{
			return ((_ioBuffer.isRequireClose() || _requireClose) ? true : _ioBuffer.isTransmissionStopped());
		}

		void extractTimeoutedCallback(int64_t now, std::unordered_map<uint32_t, BasicAnswerCallback*>& timeoutedCallbacks);
		inline void swapCallbackMap(std::unordered_map<uint32_t, BasicAnswerCallback*>& callbackMap)
		{
			std::unique_lock<std::mutex> lck(_mutex);
			callbackMap.swap(_callbackMap);
		}

		BasicAnswerCallback* takeCallback(uint32_t seqNum);
		void initRecvFinishCheck(std::list<FPQuestPtr>& deliverableQuests, bool& requireConnectedEvent);

		inline bool isConnectedEventTriggered(bool requireCallCloseEvent)
		{
			std::unique_lock<std::mutex> lck(_mutex);
			_requireCallCloseEvent = requireCallCloseEvent;
			return _connectedEventStatus == UDPConnectionEventStatus::Called;
		}

		inline bool connectedEventCompleted()
		{
			std::unique_lock<std::mutex> lck(_mutex);
			connectedEventCalled = true;
			_connectedEventStatus = UDPConnectionEventStatus::Called;
			return !_requireCallCloseEvent;
		}

		bool sendQuestWithBasicAnswerCallback(FPQuestPtr quest, BasicAnswerCallback* callback, int timeout, bool discardable);

		inline bool getRecvToken() { return _ioBuffer.getRecvToken(); }
		inline void returnRecvToken() { return _ioBuffer.returnRecvToken(); }
		void sendCachedData(bool& needWaitSendEvent, bool socketReady);
		void sendData(bool& needWaitSendEvent, std::string* data, int64_t expiredMS, bool discardable);
		bool recvData(std::list<FPQuestPtr>& questList, std::list<FPAnswerPtr>& answerList);

		inline void sendData(std::string* data, int64_t expiredMS, bool discardable)
		{
			bool needWaitSendEvent = false;
			sendData(needWaitSendEvent, data, expiredMS, discardable);
			if (needWaitSendEvent)
				waitForAllEvents();
		}
	};

	class UDPServerConnectionMap
	{
		std::mutex _mutex;
		std::unordered_map<int, UDPServerConnection*> _connections;
		std::map<int64_t, std::unordered_set<UDPServerConnection*>> _scheduleMap;

		void removeFromScheduleMap(UDPServerConnection* conn);
		void changeSchedule(UDPServerConnection* conn, int64_t sendingTurn);
		void putRescheduleMapBack(std::unordered_map<int64_t, std::unordered_set<UDPServerConnection*>>& rescheduleMap);

	public:
		// void insert(int socket, UDPServerConnection* conn);
		void insert(const std::unordered_map<std::string, UDPServerConnection*>& cache);
		UDPServerConnection* remove(int socket);
#ifdef __APPLE__
		UDPServerConnection* signConnection(int socket, uint16_t filter);
#else
		UDPServerConnection* signConnection(int socket, uint32_t events);
#endif
		void markAllConnectionsActiveCloseSignal();
		bool markConnectionActiveCloseSignal(int socket);
		void removeAllConnections(std::list<UDPServerConnection*>& connections);
		void extractInvalidConnectionsAndCallbcks(std::list<UDPServerConnection*>& invalidConnections,
			std::unordered_map<uint32_t, BasicAnswerCallback*>& timeoutedCallbacks);

		inline int connectionCount()
		{
			std::unique_lock<std::mutex> lck(_mutex);
			return (int)_connections.size();
		}
		void periodSending();
		bool sendData(int socket, uint64_t token, std::string* data, int64_t expiredMS, bool discardable);
		bool sendQuestWithBasicAnswerCallback(int socket, uint64_t token, FPQuestPtr quest, BasicAnswerCallback* callback, int timeout, bool discardable);
	};

	class UDPEpollServer;
	class UDPServerIOWorker: public ParamTemplateThreadPool<UDPServerConnection *>::IProcessor
	{
		volatile bool _serverIsStopping;		//-- If std::atomic<bool> is not good, we test volatile key word at first here.
		UDPEpollServer* _server;

		void read(UDPServerConnection * connection);
		void initRead(UDPServerConnection * connection);
		void initConnection(UDPServerConnection * connection);

	public:
		UDPServerIOWorker(): _serverIsStopping(false) {}
		virtual ~UDPServerIOWorker() {}

		inline void setServer(UDPEpollServer* server) { _server = server; }
		inline void serverWillStop() { _serverIsStopping = true; }

		virtual void run(UDPServerConnection * connection);
	};
}

#endif
