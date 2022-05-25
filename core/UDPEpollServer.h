#ifndef FPNN_UDP_Epoll_Server_H
#define FPNN_UDP_Epoll_Server_H

#include <atomic>
#include <string>
#ifdef __APPLE__
	#include <sys/types.h>
	#include <sys/event.h>
	#include <sys/time.h>
#endif
#include "Config.h"
#include "ParamTemplateThreadPoolArray.h"
#include "TaskThreadPoolArray.h"
#include "ServerController.h"
#include "ServerInterface.h"
#include "RecordIPList.h"
#include "UDP.v2/UDPIOBuffer.v2.h"
#include "GlobalIOPool.h"
#include "UDPServerIOWorker.h"
#include "ConnectionReclaimer.h"
#include "UDPServerMasterProcessor.h"
#include "ConcurrentSenderInterface.h"
#include "KeyExchange.h"

namespace fpnn
{
#define FPNN_DEFAULT_UDP_WORK_POOL_QUEUE_SIZE	2000000

	class UDPEpollServer;
	typedef std::shared_ptr<UDPEpollServer> UDPServerPtr;

	class UDPConnectionCache
	{
		std::unordered_map<std::string, UDPServerConnection*> _cache;

	public:
		~UDPConnectionCache() { reset(); }

		void insert(UDPServerReceiver& receiver, UDPServerConnection* conn);
		UDPServerConnection* check(UDPServerReceiver& receiver);
		inline std::unordered_map<std::string, UDPServerConnection*>& getCache() { return _cache; }
		inline void reset() { _cache.clear(); }
	};

	class UDPEpollServer: virtual public ServerInterface, virtual public IConcurrentUDPSender
	{
	private:
		uint16_t _port;
		std::string _ip;
		uint16_t _port6;
		std::string _ipv6;

		struct sockaddr_in  _serverAddr;
		struct sockaddr_in6 _serverAddr6;

		int _socket;
		int _socket6;
#ifdef __APPLE__
		int _kqueue_fd;
#else
		int _epoll_fd;
#endif

		const int _max_events;
#ifdef __APPLE__
		struct kevent* _kqueueEvents;
#else
		struct epoll_event* _epollEvents;
#endif

		std::atomic<bool> _running;
		volatile bool _stopping;
		std::atomic<bool> _stopSignalNotified;

		int _closeNotifyFds[2];
		size_t _maxWorkerPoolQueueLength;
		int64_t _timeoutQuest; //only valid when as a client

		std::set<int> _newSockets;
		std::set<int> _newSockets6;
		UDPServerReceiver _receiver;
		UDPConnectionCache _connectionCache;
		UDPServerConnectionMap _connectionMap;
		std::shared_ptr<UDPServerIOWorker> _ioWorker;
		std::shared_ptr<UDPServerMasterProcessor> _serverMasterProcessor;

		GlobalIOPoolPtr _ioPool;
		std::shared_ptr<ParamTemplateThreadPoolArray<UDPRequestPackage *>> _workerPool;
		std::shared_ptr<TaskThreadPoolArray> _answerCallbackPool;

		bool _encryptEnabled;
		ECCKeyExchange _keyExchanger;
		std::atomic<bool> _enableIPWhiteList;
		IPWhiteList _ipWhiteList;
		ConnectionReclaimerPtr _reclaimer;

		static UDPServerPtr _server;

	private:
		FPLogBasePtr _heldLogger;

	private:
		int getCPUCount();
		bool initServerVaribles();
		bool prepare();
		bool init();
		bool initIPv4();
		bool initIPv6();
#ifdef __APPLE__
		bool initKqueue();
#else
		bool initEpoll();
#endif
		void exitEpoll(int socket);
		const char* createSocket(int socketDomain, const struct sockaddr *addr, socklen_t addrlen, int& newSocket);
		void initFailClean(const char* fail_info);
		void createConnections(int socket);
		void createConnections6(int socket6);
		bool checkSourceAddress();
		void newConnection(int newSocket, bool isIPv4);
		void recheckNewSockets();
		void scheduleNewConnections();
#ifdef __APPLE__
		void processEvent(struct kevent & event);
#else
		void processEvent(struct epoll_event & event);
#endif
		void returnServerStoppingAnswer(UDPServerConnection* conn, FPQuestPtr quest);
		void reclaimeConnection(UDPServerConnection* conn);
		void clearConnectionQuestCallbacks(UDPServerConnection* conn, int errorCode);
		void clearRemnantConnections();
		void clean();
		void exitCheck();
		void internalTerminateConnection(UDPServerConnection* conn, int socket, UDPSessionEventType type, int errorCode);
		bool sendQuestWithBasicAnswerCallback(int socket, uint64_t token, FPQuestPtr quest, BasicAnswerCallback* callback, int timeout, bool discardable);

	private:
#ifdef __APPLE__
		UDPEpollServer(): _port(0), _port6(0), _socket(0), _socket6(0), _kqueue_fd(0), _max_events(16),
			_kqueueEvents(NULL), _running(false), _stopping(false), _stopSignalNotified(false),
#else
		UDPEpollServer(): _port(0), _port6(0), _socket(0), _socket6(0), _epoll_fd(0), _max_events(16),
			_epollEvents(NULL), _running(false), _stopping(false), _stopSignalNotified(false),
#endif
			_maxWorkerPoolQueueLength(FPNN_DEFAULT_UDP_WORK_POOL_QUEUE_SIZE),
			_timeoutQuest(FPNN_DEFAULT_QUEST_TIMEOUT * 1000),
			_serverMasterProcessor(new UDPServerMasterProcessor()), _encryptEnabled(false), _enableIPWhiteList(false)
		{
			_closeNotifyFds[0] = 0;
			_closeNotifyFds[1] = 0;

			ServerController::installSignal();

			Config::initSystemVaribles();

			initServerVaribles();

			_reclaimer = ConnectionReclaimer::instance();
		}

	public:
		/*===============================================================================
		  Call by framwwork.
		=============================================================================== */
		void connectionConnectedEventCompleted(UDPServerConnection* conn);
		void terminateConnection(int socket, UDPSessionEventType type, int errorCode);
		bool deliverSessionEvent(UDPServerConnection* conn, UDPSessionEventType type);
		void deliverAnswer(UDPServerConnection* conn, FPAnswerPtr answer);
		void deliverQuest(UDPServerConnection* conn, FPQuestPtr quest);
		bool joinEpoll(int socket);
		bool waitForAllEvents(int socket);
		bool waitForRecvEvent(int socket);

#ifdef __APPLE__
		inline bool joinKqueue(int socket) { return joinEpoll(socket); }
#endif

	public:
		virtual ~UDPEpollServer()
		{
			exitCheck();
			
			if (_ioPool)
				_ioPool->setServerIOWorker((std::shared_ptr<UDPServerIOWorker>)nullptr);
		}

		static UDPServerPtr create()
		{
			if(!_server)
				_server.reset(new UDPEpollServer());
			return _server;
		}

		static UDPEpollServer* nakedInstance()
		{
			return _server.get();
		}
		static UDPServerPtr instance()
		{
			return _server;
		}

		//-----------------------------------------------//
		//-- Inherit from Server Interface
		//-----------------------------------------------//
		virtual bool startup()
		{
			bool status = prepare() ? init() : false;
			if(status)
			{
				ServerController::startTimeoutCheckThread();
				_serverMasterProcessor->getQuestProcessor()->start();
			}
			return status;
		}
		virtual void run();
		virtual void stop();

		virtual void setIP(const std::string& ip) { _ip = ip; }
		virtual void setIPv6(const std::string& ipv6) { _ipv6 = ipv6; }
		virtual void setPort(unsigned short port) { _port = port; }
		virtual void setPort6(unsigned short port) { _port6 = port; }
		virtual void setBacklog(int backlog) {}

		virtual uint16_t port() const { return _port; }
		virtual uint16_t port6() const { return _port6; }
		virtual std::string ip() const { return _ip; }
		virtual std::string ipv6() const { return _ipv6; }

		//-----------------------------------------------//
		//-- Basic configurations & properties.
		//-----------------------------------------------//
		virtual void setQuestProcessor(IQuestProcessorPtr questProcessor)
		{
			questProcessor->setConcurrentSender(this);
			_serverMasterProcessor->setQuestProcessor(questProcessor);
		}
		virtual IQuestProcessorPtr getQuestProcessor()
		{
			return _serverMasterProcessor->getQuestProcessor();
		}
		inline bool isMasterMethod(const std::string& method)
		{
			return _serverMasterProcessor->isMasterMethod(method);
		}
		inline void setQuestTimeout(int64_t seconds)
		{
			_timeoutQuest = seconds * 1000;
		}
		inline int64_t getQuestTimeout()
		{
			return _timeoutQuest / 1000;
		}

		inline int currentConnections()
		{
			return _connectionMap.connectionCount();
		}
		inline int validARQConnections()
		{
			return _serverMasterProcessor->validARQConnectionCount();
		}

		inline void configWorkerThreadPool(int32_t initCount, int32_t perAppendCount, int32_t perfectCount, int32_t maxCount, size_t maxQueueSize)
		{
			int arraySize = getCPUCount();
			if (initCount > 0 && arraySize > initCount)
				arraySize = initCount;

			_workerPool.reset(new ParamTemplateThreadPoolArray<UDPRequestPackage *>(arraySize, _serverMasterProcessor));
			_workerPool->init(initCount, perAppendCount, perfectCount, maxCount, maxQueueSize);
		}
		inline void enableAnswerCallbackThreadPool(int32_t initCount, int32_t perAppendCount, int32_t perfectCount, int32_t maxCount)
		{
			if (_answerCallbackPool)
				return;

			int arraySize = getCPUCount();
			if (initCount > 0 && arraySize > initCount)
				arraySize = initCount;
			
			_answerCallbackPool.reset(new TaskThreadPoolArray(arraySize));
			_answerCallbackPool->init(initCount, perAppendCount, perfectCount, maxCount);
		}
		virtual std::string workerPoolStatus()
		{
			if (_workerPool)
				return _workerPool->infos();
			
			return "{}";
		}
		virtual std::string answerCallbackPoolStatus()
		{
			if (_answerCallbackPool){
				return _answerCallbackPool->infos();
			}
			return "{}";
		}

		static void enableForceEncryption();
		inline bool encrpytionEnabled() { return _encryptEnabled; }
		inline bool enableEncryptor(const std::string& curve, const std::string& privateKey)
		{
			_encryptEnabled = _keyExchanger.init(curve, privateKey);
			return _encryptEnabled;
		}

		inline bool ipWhiteListEnabled() { return _enableIPWhiteList; }
		inline void enableIPWhiteList(bool enable = true)
		{
			_enableIPWhiteList = enable;
		}
		inline bool addIPToWhiteList(const std::string& ip)
		{
			return _ipWhiteList.addIP(ip);
		}
		inline bool addSubnetToWhiteList(const std::string& ip, int subnetMaskBits)
		{
			return _ipWhiteList.addIPv4SubNet(ip, subnetMaskBits);
		}
		inline void removeIPFromWhiteList(const std::string& ip)
		{
			_ipWhiteList.removeIP(ip);
		}

		virtual void sendData(int socket, uint64_t token, std::string* data, int64_t expiredMS, bool discardable);

		/**
			All SendQuest():
				If return false, caller must free quest & callback.
				If return true, don't free quest & callback.
		*/
		virtual FPAnswerPtr sendQuest(int socket, uint64_t token, FPQuestPtr quest, int timeout = 0, bool discardable = true);
		virtual bool sendQuest(int socket, uint64_t token, FPQuestPtr quest, AnswerCallback* callback, int timeout = 0, bool discardable = true)
		{
			if (timeout == 0) timeout = _timeoutQuest;
			return sendQuestWithBasicAnswerCallback(socket, token, quest, callback, timeout, discardable);
		}
		virtual bool sendQuest(int socket, uint64_t token, FPQuestPtr quest, std::function<void (FPAnswerPtr answer, int errorCode)> task, int timeout = 0, bool discardable = true)
		{
			if (timeout == 0) timeout = _timeoutQuest;

			BasicAnswerCallback* t = new FunctionAnswerCallback(std::move(task));
			if (sendQuestWithBasicAnswerCallback(socket, token, quest, t, timeout, discardable))
				return true;
			else
			{
				delete t;
				return false;
			}
		}
		
		void closeConnection(int socket, bool force = false);
		void periodSendingCheck();
		void checkTimeout();
	};
}

#endif