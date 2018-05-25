#ifndef FPNN_TCP_Epoll_Server_H
#define FPNN_TCP_Epoll_Server_H

#include <atomic>
#include <mutex>
#include <memory>
#include <vector>
#include <thread>
#include <signal.h>
#include <sys/sysinfo.h>
#include "ParamTemplateThreadPoolArray.h"
#include "TaskThreadPoolArray.h"
#include "ServerInterface.h"
#include "ServerIOWorker.h"
#include "FPMessage.h"
#include "ServerMasterProcessor.h"
#include "ConcurrentSenderInterface.h"
#include "ConnectionReclaimer.h"
#include "PartitionedConnectionMap.h"
#include "Setting.h"
#include "Config.h"
#include "GlobalIOPool.h"
#include "RecordIPList.h"
#include "KeyExchange.h"

struct sockaddr_in;

namespace fpnn
{
#define FPNN_SERVER_VERSION				"0.8.1"
#define FPNN_DEFAULT_WORK_POOL_QUEUE_SIZE	2000000
#define FPNN_DEFAULT_MAX_CONNECTION			500000
#define FPNN_DEFAULT_SOCKET_BACKLOG			10000
#define FPNN_DEFAULT_MAX_EVENT				10000
#define FPNN_DEFAULT_IO_BUFFER_CHUNK_SIZE	256

#define FPNN_SEND_QUEUE_MUTEX_COUNT			128

	class TCPEpollServer;
	typedef std::shared_ptr<TCPEpollServer> ServerPtr;

	class TCPEpollServer: virtual public ServerInterface, virtual public IConcurrentSender
	{
	private:
		uint16_t _port;
		std::string _ip;
		uint16_t _port6;
		std::string _ipv6;
		int _backlog;

		int _socket;
		int _socket6;
		int _epoll_fd;

		int _max_events;
		struct epoll_event* _epollEvents;

		bool _listenIPv6;
		std::atomic<bool> _running;
		volatile bool _stopping;
		std::atomic<bool> _stopped;
		std::atomic<int> _connectionCount;

		size_t _ioBufferChunkSize;
		int _closeNotifyFds[2];

		uint16_t _cpuCount;
		uint16_t _ioThreadMin;
		uint16_t _ioThreadMax;
		
		uint16_t _workThreadMin;
		uint16_t _workThreadMax;

		uint16_t _duplexThreadMin; // --default set to 0
		uint16_t _duplexThreadMax; // --default set to 0

		size_t _maxWorkerPoolQueueLength;
		int _maxConnections;

		time_t _started;
		time_t _compiled;
		std::string _sName;
		std::string _version;

		bool _checkQuestTimeout;
		int64_t _timeoutQuest; //only valid when as a client
		int64_t _timeoutIdle;
		std::thread _timeoutChecker;
		std::thread _stopSignalThread;

		bool _logServerStatusInfos;
		int _logStatusIntervalSeconds;

		static std::mutex _sendQueueMutex[FPNN_SEND_QUEUE_MUTEX_COUNT];
		static uint16_t _THREAD_MIN;
		static uint16_t _THREAD_MAX;

		PartitionedConnectionMap _connectionMap;
		std::shared_ptr<TCPServerIOWorker> _ioWorker;
		std::shared_ptr<ServerMasterProcessor> _serverMasterProcessor;

		GlobalIOPoolPtr _ioPool;
		std::shared_ptr<ParamTemplateThreadPoolArray<RequestPackage *>> _workerPool;
		std::shared_ptr<TaskThreadPoolArray> _answerCallbackPool; //-- send quest to client

		bool _encryptEnabled;
		ECCKeyExchange _keyExchanger;
		std::atomic<bool> _enableIPWhiteList;
		IPWhiteList _ipWhiteList;
		ConnectionReclaimerPtr _reclaimer;

	private:
		bool							prepare();
		bool							init();
		bool							initIPv6();
		void							acceptIPv4Connection();
		void							acceptIPv6Connection();
		void							newConnection(int newSocket, ConnectionInfoPtr ci);
		void							processEvent(struct epoll_event & event);
		void							cleanForNewConnectionError(TCPServerConnection*, RequestPackage*, const char* log_info, bool connectionInCache);
		void							cleanForExistConnectionError(TCPServerConnection*, RequestPackage*, const char* log_info);
		void							fatalFailClean(const char* fail_info);
		void							clean();
		void							sendCloseEvent();
		void							timeoutCheckThread();

	private:
		TCPEpollServer()
			: _port(0), _port6(0), _backlog(FPNN_DEFAULT_SOCKET_BACKLOG), _socket(0), _socket6(0), _epoll_fd(0), 
			_max_events(FPNN_DEFAULT_MAX_EVENT), 
			_epollEvents(NULL), _listenIPv6(false), _running(false), _stopping(false), _stopped(true), _connectionCount(0),
			_ioBufferChunkSize(FPNN_DEFAULT_IO_BUFFER_CHUNK_SIZE),
			_maxWorkerPoolQueueLength(FPNN_DEFAULT_WORK_POOL_QUEUE_SIZE),
			_maxConnections(FPNN_DEFAULT_MAX_CONNECTION),
			_started(time(NULL)), _version(FPNN_SERVER_VERSION), _checkQuestTimeout(false),
			_timeoutQuest(FPNN_DEFAULT_QUEST_TIMEOUT * 1000),
			_timeoutIdle(FPNN_DEFAULT_IDLE_TIMEOUT * 1000),
			_logServerStatusInfos(false), _logStatusIntervalSeconds(60),
			_serverMasterProcessor(new ServerMasterProcessor()), _encryptEnabled(false), _enableIPWhiteList(false)
		{
			install_signal();

			initSystemVaribles();

			_sName = Setting::getString("FPNN.server.name", "FPNN.TEST");

			std::string logEndpoint = Setting::getString("FPNN.server.log.endpoint", "std::cout");
			std::string logRoute = Setting::getString("FPNN.server.log.route", "FPNN.TEST");
			std::string logLevel = Setting::getString("FPNN.server.log.level", "DEBUG");
			FPLog::init(logEndpoint, logRoute, logLevel, _sName);

			_maxConnections = Setting::getInt("FPNN.server.max.connections", FPNN_DEFAULT_MAX_CONNECTION);
			_logServerStatusInfos = Setting::getBool("FPNN.server.status.logStatusInfos", false);
			_logStatusIntervalSeconds = Setting::getInt("FPNN.server.status.logStatusInterval", 60);
			if (_logStatusIntervalSeconds < 1)
				_logServerStatusInfos = false;

			_cpuCount = get_nprocs();
			if (_cpuCount < 2)
				_cpuCount = 2;
	
			_workThreadMin = _cpuCount;
			_workThreadMax = _cpuCount;
			_ioThreadMin = _cpuCount;
			_ioThreadMax = _cpuCount;

			_duplexThreadMin = 0;
			_duplexThreadMax = 0;

			std::string built = std::string("") + __DATE__ + " " + __TIME__;  
			struct tm t;
			strptime(built.c_str(), "%b %d %Y %H:%M:%S", &t);
			_compiled = mktime(&t);

			initServerVaribles();

			_closeNotifyFds[0] = 0;
			_closeNotifyFds[1] = 0;
			_reclaimer = ConnectionReclaimer::instance();
		}
		static ServerPtr _server;
		static std::atomic<bool> _stopSignalTriggered;
	public:
		virtual ~TCPEpollServer()
		{
			if (_ioPool)
				_ioPool->setServerIOWorker(nullptr);
		}

		static ServerPtr create(){
			if(!_server) _server.reset(new TCPEpollServer());
			return _server;
		}

		static TCPEpollServer* nakedInstance()
		{
			return _server.get();
		}
		static ServerPtr instance()
		{
			return _server;
		}

		virtual bool startup()
		{
			bool status = prepare() ? init() : false;
			if (_listenIPv6 && initIPv6() == false)
			{
				LOG_ERROR("Init IPv6 failed.");
			}

			if(status){
				_timeoutChecker = std::thread(&TCPEpollServer::timeoutCheckThread, this);
				_serverMasterProcessor->getQuestProcessor()->start();
			}
			return status;
		}
		virtual void run();
		virtual void stop();

		/*===============================================================================
		  Basic configurations & properties.
		=============================================================================== */
		virtual void setIP(const std::string& ip) { _ip = ip; }
		virtual void setIPv6(const std::string& ipv6) { _ipv6 = ipv6; }
		virtual void setPort(uint16_t port) { _port = port; }
		virtual void setBacklog(int backlog) { _backlog = backlog; }

		virtual uint16_t port() const { return _port; }
		virtual std::string ip() const { return _ip; }
		virtual std::string ipv6() const { return _ipv6; }	
		virtual int32_t backlog() const { return _backlog; }

		const std::string& sName() const { return _sName; }
		const std::string& version() const { return _version; }
		time_t started() const { return _started; }
		time_t compiled() const { return _compiled; }

		inline void setMaxEvents(int maxCount) { _max_events = maxCount; }
		inline void setIoBufferChunkSize(size_t size) { _ioBufferChunkSize = size; }

		int maxEvent() const { return _max_events; }
		size_t ioBufferChunkSize() const { return _ioBufferChunkSize; }

		inline void setQuestProcessor(IQuestProcessorPtr questProcessor)
		{
			questProcessor->setConcurrentSender(this);
			_serverMasterProcessor->setQuestProcessor(questProcessor);
		}
		inline void setEstimateMaxConnections(size_t estimateMaxConnections, int partitionCount = 128)
		{
			size_t perSize = (estimateMaxConnections / partitionCount) + 1;
			if (perSize < 32)
				perSize = 32;

			_connectionMap.init(partitionCount, perSize);
		}
		inline void setMaxConnectionLimitation(int maxConnectionLimitation)
		{
			_maxConnections = maxConnectionLimitation;
		}
		inline int maxConnectionLimitation()
		{
			return _maxConnections;
		}
		inline int currentConnections()
		{
			return _connectionCount;
		}
		inline void decreaseConnectionCount()
		{
			_connectionCount--;
		}
		inline void configWorkerThreadPool(int32_t initCount, int32_t perAppendCount, int32_t perfectCount, int32_t maxCount, size_t maxQueueSize)
		{
			_workerPool.reset(new ParamTemplateThreadPoolArray<RequestPackage *>(_cpuCount, _serverMasterProcessor));
			_workerPool->init(initCount, perAppendCount, perfectCount, maxCount, maxQueueSize);
		}
		inline void configWorkerThreadPool(IQuestProcessorPtr questProcessor,
						int32_t initCount, int32_t perAppendCount, int32_t perfectCount, int32_t maxCount, size_t maxQueueSize)
		{
			setQuestProcessor(questProcessor);
			configWorkerThreadPool(initCount, perAppendCount, perfectCount, maxCount, maxQueueSize);
		}
		inline void enableAnswerCallbackThreadPool(int32_t initCount, int32_t perAppendCount, int32_t perfectCount, int32_t maxCount)
		{
			if (_answerCallbackPool)
				return;

			_answerCallbackPool.reset(new TaskThreadPoolArray(_cpuCount));
			_answerCallbackPool->init(initCount, perAppendCount, perfectCount, maxCount);
		}

		/*===============================================================================
		  Operations.
		=============================================================================== */
		inline TCPServerConnection* takeConnection(int fd)	//-- !!! ONLY used in Client Engine & IOWorker internal.
		{
			return (TCPServerConnection*)_connectionMap.takeConnection(fd);
		}
		inline TCPServerConnection* takeConnection(const ConnectionInfo* ci)	//-- !!! Using for other case.
		{
			return (TCPServerConnection*)_connectionMap.takeConnection(ci);
		}
		void clearConnectionQuestCallbacks(TCPServerConnection*, int errorCode);

		virtual void sendData(int socket, uint64_t token, std::string* data);

		/**
			All SendQuest():
				If return false, caller must free quest & callback.
				If return true, don't free quest & callback.
		*/
		virtual FPAnswerPtr sendQuest(int socket, uint64_t token, std::mutex* mutex, FPQuestPtr quest, int timeout = 0)
		{
			_checkQuestTimeout = true;
			if (timeout == 0) timeout = _timeoutQuest;
			return _connectionMap.sendQuest(socket, token, mutex, quest, timeout);
		}
		virtual bool sendQuest(int socket, uint64_t token, FPQuestPtr quest, AnswerCallback* callback, int timeout = 0)
		{
			_checkQuestTimeout = true;
			if (timeout == 0) timeout = _timeoutQuest;
			return _connectionMap.sendQuest(socket, token, quest, callback, timeout);
		}
		virtual bool sendQuest(int socket, uint64_t token, FPQuestPtr quest, std::function<void (FPAnswerPtr answer, int errorCode)> task, int timeout = 0)
		{
			_checkQuestTimeout = true;
			if (timeout == 0) timeout = _timeoutQuest;
			return _connectionMap.sendQuest(socket, token, quest, std::move(task), timeout);
		}

		
		void dealAnswer(int socket, FPAnswerPtr answer);
		void closeConnection(const ConnectionInfo* ci);
		inline void closeConnection(const ConnectionInfo& ci)
		{
			closeConnection(&ci);
		}

		/*
			Experimental interface.
			** Maybe change or remove in following version. **
		*/
		void closeConnectionAfterSent(const ConnectionInfo* ci);
		inline void closeConnectionAfterSent(const ConnectionInfo& ci)
		{
			closeConnectionAfterSent(&ci);
		}

		/*===============================================================================
		  Basic configurations & properties.
		=============================================================================== */
		inline bool isMasterMethod(const std::string& method)
		{
			return _serverMasterProcessor->isMasterMethod(method);
		}
		inline void setQuestTimeout(int64_t seconds)
		{
			_timeoutQuest = seconds * 1000;
		}
		inline int64_t getQuestTimeout(){
			return _timeoutQuest / 1000;
		}
		inline void setIdleTimeout(int64_t seconds)
		{
			_timeoutIdle = seconds * 1000;
		}
		inline int64_t getIdleTimeout(){
			return _timeoutIdle / 1000;
		}
		inline void setStatusInfosLogStatus(bool logStatusInfos)
		{
			_logServerStatusInfos = logStatusInfos;
		}
		inline bool getStatusInfosLogStatus()
		{
			return _logServerStatusInfos;
		}
		inline void setStatusInfosLogIntervalSeconds(int seconds)
		{
			_logStatusIntervalSeconds = seconds;
		}
		inline int getStatusInfosLogIntervalSeconds()
		{
			return _logStatusIntervalSeconds;
		}

		inline std::string workerPoolStatus(){
			if (_workerPool){
				return _workerPool->infos();
			}
			return "{}";
		}
		inline std::string answerCallbackPoolStatus(){
			if (_answerCallbackPool){
				return _answerCallbackPool->infos();
			}
			return "{}";
		}
		inline bool encrpytionEnabled() { return _encryptEnabled; }
		inline bool enableEncryptor(const std::string& curve, const std::string& privateKey)
		{
			_encryptEnabled = _keyExchanger.init(curve, privateKey);
			return _encryptEnabled;
		}
		static void enableForceEncryption();
		//-- Params: please refer KeyExchange.h.
		inline bool calcEncryptorKey(uint8_t* key, uint8_t* iv, int keylen, const std::string& peerPublicKey)
		{
			return _keyExchanger.calcKey(key, iv, keylen, peerPublicKey);
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
	private:
		void install_signal();
		static void stop_handler(int sig);
		bool initSystemVaribles();
		bool initServerVaribles();
	};
	
}

#endif
