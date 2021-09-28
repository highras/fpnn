#ifndef FPNN_TCP_Epoll_Server_H
#define FPNN_TCP_Epoll_Server_H

#include <atomic>
#include <mutex>
#include <memory>
#include <vector>
#include <thread>
#include "ParamTemplateThreadPoolArray.h"
#include "TaskThreadPoolArray.h"
#include "ServerController.h"
#include "ServerInterface.h"
#include "ServerIOWorker.h"
#include "FPMessage.h"
#include "TCPServerMasterProcessor.h"
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
#define FPNN_DEFAULT_WORK_POOL_QUEUE_SIZE	2000000
#define FPNN_DEFAULT_MAX_CONNECTION			500000
#define FPNN_DEFAULT_SOCKET_BACKLOG			10000
#define FPNN_DEFAULT_MAX_EVENT				10000
#define FPNN_DEFAULT_IO_BUFFER_CHUNK_SIZE	256

#define FPNN_SEND_QUEUE_MUTEX_COUNT			128

	class TCPEpollServer;
	typedef std::shared_ptr<TCPEpollServer> TCPServerPtr;

	class TCPEpollServer: virtual public ServerInterface, virtual public IConcurrentSender
	{
		struct SocketInfo
		{
			uint16_t port;
			std::string ip;
			int socket;

			SocketInfo(): port(0), socket(0) {}
			void init(const std::vector<std::string>& ipItems, const std::vector<std::string>& portItems);
			void close();
		};

	private:
		struct SocketInfo _ipv4;
		struct SocketInfo _ipv6;
		struct SocketInfo _sslIPv4;
		struct SocketInfo _sslIPv6;
		int _backlog;
#ifdef __APPLE__
		int _kqueue_fd;
#else
		int _epoll_fd;
#endif

		int _max_events;
#ifdef __APPLE__
		struct kevent* _kqueueEvents;
#else
		struct epoll_event* _epollEvents;
#endif

		std::atomic<bool> _running;
		volatile bool _stopping;
		std::atomic<bool> _stopSignalNotified;
		std::atomic<int> _connectionCount;

		size_t _ioBufferChunkSize;
		int _closeNotifyFds[2];

		uint16_t _duplexThreadMin; // --default set to 0
		uint16_t _duplexThreadMax; // --default set to 0

		size_t _maxWorkerPoolQueueLength;
		int _maxConnections;

		volatile bool _checkQuestTimeout;
		int64_t _timeoutQuest; //only valid when as a client
		int64_t _timeoutIdle;

		static std::mutex _sendQueueMutex[FPNN_SEND_QUEUE_MUTEX_COUNT];

		PartitionedConnectionMap _connectionMap;
		std::shared_ptr<TCPServerIOWorker> _ioWorker;
		std::shared_ptr<TCPServerMasterProcessor> _serverMasterProcessor;

		GlobalIOPoolPtr _ioPool;
		std::shared_ptr<ParamTemplateThreadPoolArray<RequestPackage *>> _workerPool;
		std::shared_ptr<TaskThreadPoolArray> _answerCallbackPool; //-- send quest to client

		bool _encryptEnabled;
		ECCKeyExchange _keyExchanger;
		std::atomic<bool> _enableIPWhiteList;
		IPWhiteList _ipWhiteList;
		ConnectionReclaimerPtr _reclaimer;

	private:
		int								getCPUCount();
		bool							prepare();
		bool							init();
		bool							initIPv4(struct SocketInfo& info);
		bool							initIPv6(struct SocketInfo& info);
#ifdef __APPLE__
		bool							initKqueue();
#else
		bool							initEpoll();
#endif
		void							acceptIPv4Connection(int socket, bool ssl);
		void							acceptIPv6Connection(int socket, bool ssl);
		void							newConnection(int newSocket, ConnectionInfoPtr ci);
#ifdef __APPLE__
		void							processEvent(struct kevent & event);
#else
		void							processEvent(struct epoll_event & event);
#endif
		void							cleanForNewConnectionError(TCPServerConnection*, RequestPackage*, const char* log_info, bool connectionInCache);
		void							cleanForExistConnectionError(TCPServerConnection*, RequestPackage*, const char* log_info);
		void							initFailClean(const char* fail_info);
		void							clean();
		void							exitCheck();
		void							sendCloseEvent();

	private:
		TCPEpollServer()
#ifdef __APPLE__
			: _backlog(FPNN_DEFAULT_SOCKET_BACKLOG), _kqueue_fd(0), 
			_max_events(FPNN_DEFAULT_MAX_EVENT), 
			_kqueueEvents(NULL), _running(false), _stopping(false), _stopSignalNotified(false), _connectionCount(0),
#else
			: _backlog(FPNN_DEFAULT_SOCKET_BACKLOG), _epoll_fd(0), 
			_max_events(FPNN_DEFAULT_MAX_EVENT), 
			_epollEvents(NULL), _running(false), _stopping(false), _stopSignalNotified(false), _connectionCount(0),
#endif
			_ioBufferChunkSize(FPNN_DEFAULT_IO_BUFFER_CHUNK_SIZE),
			_maxWorkerPoolQueueLength(FPNN_DEFAULT_WORK_POOL_QUEUE_SIZE),
			_maxConnections(FPNN_DEFAULT_MAX_CONNECTION), _checkQuestTimeout(false),
			_timeoutQuest(FPNN_DEFAULT_QUEST_TIMEOUT * 1000),
			_timeoutIdle(FPNN_DEFAULT_IDLE_TIMEOUT * 1000),
			_serverMasterProcessor(new TCPServerMasterProcessor()), _encryptEnabled(false), _enableIPWhiteList(false)
		{
			ServerController::installSignal();

			initSystemVaribles();

			_maxConnections = Setting::getInt("FPNN.server.max.connections", FPNN_DEFAULT_MAX_CONNECTION);

			_duplexThreadMin = 0;
			_duplexThreadMax = 0;

			initServerVaribles();

			_closeNotifyFds[0] = 0;
			_closeNotifyFds[1] = 0;
			_reclaimer = ConnectionReclaimer::instance();
		}
		static TCPServerPtr _server;
	public:
		virtual ~TCPEpollServer()
		{
			exitCheck();
			
			if (_ioPool)
				_ioPool->setServerIOWorker((std::shared_ptr<TCPServerIOWorker>)nullptr);
		}

		static TCPServerPtr create(){
			if(!_server) _server.reset(new TCPEpollServer());
			return _server;
		}

		static TCPEpollServer* nakedInstance()
		{
			return _server.get();
		}
		static TCPServerPtr instance()
		{
			return _server;
		}

		virtual bool startup()
		{
			bool status = prepare() ? init() : false;
			if(status){
				ServerController::startTimeoutCheckThread();
				_serverMasterProcessor->getQuestProcessor()->start();
			}
			return status;
		}
		virtual void run();
		virtual void stop();

		/*===============================================================================
		  Basic configurations & properties.
		=============================================================================== */
		virtual void setIP(const std::string& ip) { _ipv4.ip = ip; }
		virtual void setIPv6(const std::string& ipv6) { _ipv6.ip = ipv6; }
		virtual void setPort(uint16_t port) { _ipv4.port = port; }
		virtual void setPort6(unsigned short port) { _ipv6.port = port; }
		virtual void setBacklog(int backlog) { _backlog = backlog; }

		virtual uint16_t port() const { return _ipv4.port; }
		virtual uint16_t port6() const { return _ipv6.port; }
		virtual std::string ip() const { return _ipv4.ip; }
		virtual std::string ipv6() const { return _ipv6.ip; }
		virtual uint16_t sslPort() const { return _sslIPv4.port; }
		virtual uint16_t sslPort6() const { return _sslIPv6.port; }
		virtual std::string sslIP() const { return _sslIPv4.ip; }
		virtual std::string sslIP6() const { return _sslIPv6.ip; }
		virtual int32_t backlog() const { return _backlog; }

		inline void setMaxEvents(int maxCount) { _max_events = maxCount; }
		inline void setIoBufferChunkSize(size_t size) { _ioBufferChunkSize = size; }

		int maxEvent() const { return _max_events; }
		size_t ioBufferChunkSize() const { return _ioBufferChunkSize; }

		virtual void setQuestProcessor(IQuestProcessorPtr questProcessor)
		{
			questProcessor->setConcurrentSender(this);
			_serverMasterProcessor->setQuestProcessor(questProcessor);
		}
		virtual IQuestProcessorPtr getQuestProcessor()
		{
			return _serverMasterProcessor->getQuestProcessor();
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
			_workerPool.reset(new ParamTemplateThreadPoolArray<RequestPackage *>(getCPUCount(), _serverMasterProcessor));
			_workerPool->init(initCount, perAppendCount, perfectCount, maxCount, maxQueueSize);
		}
		inline void enableAnswerCallbackThreadPool(int32_t initCount, int32_t perAppendCount, int32_t perfectCount, int32_t maxCount)
		{
			if (_answerCallbackPool)
				return;

			_answerCallbackPool.reset(new TaskThreadPoolArray(getCPUCount()));
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

		virtual std::string workerPoolStatus(){
			if (_workerPool){
				return _workerPool->infos();
			}
			return "{}";
		}
		virtual std::string answerCallbackPoolStatus(){
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
		
		void checkTimeout();

	private:
		bool initSystemVaribles();
		bool initServerVaribles();
	};
	
}

#endif
