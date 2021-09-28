#ifndef FPNN_Client_Engine_H
#define FPNN_Client_Engine_H

#include <mutex>
#include <atomic>
#include <memory>
#include <thread>
#include "HashMap.h"
//#include "ParamTemplateThreadPoolArray.h"
#include "TaskThreadPoolArray.h"
#include "ClientIOWorker.h"
#include "IQuestProcessor.h"
#include "ConcurrentSenderInterface.h"
#include "ConnectionReclaimer.h"
#include "PartitionedConnectionMap.h"
#include "GlobalIOPool.h"

namespace fpnn
{
	class BasicConnection;			//-- See IOWorker.h.
	class TCPClientIOWorker;		//-- See ClientIOWorker.h.
	class TCPClient;				//-- see TCPClient.h.
	class ClientEngine;
	typedef std::shared_ptr<ClientEngine> ClientEnginePtr;

	class ClientEngine: virtual public IConcurrentSender
	{
	private:
#ifdef __APPLE__
		int _kqueue_fd;
#else
		int _epoll_fd;
#endif
		static std::mutex _mutex;
		std::atomic<bool> _running;
		bool _started;		//-- void Client Engine restart. Client Engine only startup once.

		int _max_events;
#ifdef __APPLE__
		struct kevent* _kqueueEvents;
#else
		struct epoll_event* _epollEvents;
#endif

		size_t _ioBufferChunkSize;
		int _closeNotifyFds[2];

		uint16_t _cpuCount;
		uint16_t _ioThreadMin;
        uint16_t _ioThreadMax;

        uint16_t _workThreadMin;
        uint16_t _workThreadMax;

		uint16_t _duplexThreadMin; // --default set to 0
		uint16_t _duplexThreadMax; // --default set to 0

		size_t _questProcessPoolMaxQueueLength;

		static uint16_t _THREAD_MIN;
		static uint16_t _THREAD_MAX;

		PartitionedConnectionMap _connectionMap;
		std::shared_ptr<TCPClientIOWorker> _tcpIOWorker;
		std::shared_ptr<UDPClientIOWorker> _udpIOWorker;

		GlobalIOPoolPtr _ioPool;
		TaskThreadPoolArray _answerCallbackPool;		//-- answer callback & error/close event.
		TaskThreadPoolArray _questProcessPool;          //-- receive quest from server

		int64_t _timeoutQuest;
		std::thread _timeoutChecker;

		std::thread _loopThread;
		ConnectionReclaimerPtr _reclaimer;

		void prepare();
		bool init();
		bool start();
		void stop();
		void loopThread();

#ifdef __APPLE__
		void processEvent(struct kevent & event);
#else
		void processEvent(struct epoll_event & event);
#endif
		void sendCloseEvent();
		void clean();

		bool initClientVaribles();
		void timeoutCheckThread();

	public:
		ClientEngine();
		virtual ~ClientEngine()
		{
			 stop();
			 
			 if (_ioPool)
				_ioPool->setClientIOWorker(nullptr, nullptr);
		}

		/*===============================================================================
		  Instance & Reference.
		=============================================================================== */
		static ClientEngine* nakedInstance();
		static ClientEnginePtr instance();
		static bool created();

		/*===============================================================================
		  Call by framwwork.
		=============================================================================== */
		void closeUDPConnection(UDPClientConnection* connection);

		/*===============================================================================
		  Configuration.
		=============================================================================== */
		inline static void setMaxEvents(int maxCount) { nakedInstance()->_max_events = maxCount; }
		inline static void setIoBufferChunkSize(size_t size) { nakedInstance()->_ioBufferChunkSize = size; }
		inline static void setEstimateMaxConnections(size_t estimateMaxConnections, int partitionCount = 64)
		{
			size_t perSize = (estimateMaxConnections / partitionCount) + 1;
			if (perSize < 8)
				perSize = 8;

			nakedInstance()->_connectionMap.init(partitionCount, perSize);
		}

		//-- answer callback & error/close event.
		inline static void configAnswerCallbackThreadPool(int32_t initCount, int32_t perAppendCount, int32_t perfectCount, int32_t maxCount)
		{
			nakedInstance()->_answerCallbackPool.init(initCount, perAppendCount, perfectCount, maxCount);
		}

		inline static void configQuestProcessThreadPool(int32_t initCount, int32_t perAppendCount, int32_t perfectCount, int32_t maxCount, size_t maxQueueSize)
		{
			nakedInstance()->_questProcessPool.init(initCount, perAppendCount, perfectCount, maxCount, maxQueueSize);
		}

		inline static std::string answerCallbackPoolStatus(){
			if (created()){
				return nakedInstance()->_answerCallbackPool.infos();
			}
			return "{}";
		}

		inline static std::string questProcessPoolStatus(){
			if (created()){
				return nakedInstance()->_questProcessPool.infos();
			}
			return "{}";
		}

        inline static bool questProcessPoolExiting()
        {
        	return nakedInstance()->_questProcessPool.exiting();
        }

        inline static void setQuestTimeout(int64_t seconds)
		{
			nakedInstance()->_timeoutQuest = seconds * 1000;
		}
		inline static int64_t getQuestTimeout(){
			return nakedInstance()->_timeoutQuest / 1000;
		}
		/*===============================================================================
		  Operations.
		=============================================================================== */
		inline static bool startEngine()
		{
			return nakedInstance()->start();
		}
		
		inline void addNewConnection(BasicConnection* connection)
		{
			_connectionMap.insert(connection->socket(), connection);
		}
		inline BasicConnection* takeConnection(int fd)	//-- !!! ONLY used in Client Engine & IOWorker internal.
		{
			return _connectionMap.takeConnection(fd);
		}
		inline BasicConnection* takeConnection(const ConnectionInfo* ci)  //-- !!! Using for other case. e.g. TCPClient.
		{
			return _connectionMap.takeConnection(ci);
		}
		inline BasicAnswerCallback* takeCallback(int socket, uint32_t seqNum)
		{
			return _connectionMap.takeCallback(socket, seqNum);
		}
		inline void keepAlive(int socket, bool keepAlive)		//-- Only for ARQ UDP
		{
			_connectionMap.keepAlive(socket, keepAlive);
		}
		inline void setUDPUntransmittedSeconds(int socket, int untransmittedSeconds)
		{
			_connectionMap.setUDPUntransmittedSeconds(socket, untransmittedSeconds);
		}
		inline void executeConnectionAction(int socket, std::function<void (BasicConnection* conn)> action)		//-- Only for ARQ UDP
		{
			_connectionMap.executeConnectionAction(socket, std::move(action));
		}
		void clearConnectionQuestCallbacks(BasicConnection*, int errorCode);
		
		virtual void sendData(int socket, uint64_t token, std::string* data);
		virtual void sendUDPData(int socket, uint64_t token, std::string* data, int64_t expiredMS, bool discardable);

		/**
			All SendQuest():
				If return false, caller must free quest & callback.
				If return true, don't free quest & callback.
		*/
		virtual FPAnswerPtr sendQuest(int socket, uint64_t token, std::mutex* mutex, FPQuestPtr quest, int timeout = 0)
		{
			if (timeout == 0) timeout = _timeoutQuest;
			return _connectionMap.sendQuest(socket, token, mutex, quest, timeout, quest->isOneWay());
		}
		virtual bool sendQuest(int socket, uint64_t token, FPQuestPtr quest, AnswerCallback* callback, int timeout = 0)
		{
			if (timeout == 0) timeout = _timeoutQuest;
			return _connectionMap.sendQuest(socket, token, quest, callback, timeout, quest->isOneWay());
		}
		virtual bool sendQuest(int socket, uint64_t token, FPQuestPtr quest, std::function<void (FPAnswerPtr answer, int errorCode)> task, int timeout = 0)
		{
			if (timeout == 0) timeout = _timeoutQuest;
			return _connectionMap.sendQuest(socket, token, quest, std::move(task), timeout, quest->isOneWay());
		}
		//-- For UDP Client
		virtual FPAnswerPtr sendQuest(int socket, uint64_t token, std::mutex* mutex, FPQuestPtr quest, int timeout, bool discardableUDPQuest)
		{
			if (timeout == 0) timeout = _timeoutQuest;
			return _connectionMap.sendQuest(socket, token, mutex, quest, timeout, discardableUDPQuest);
		}
		virtual bool sendQuest(int socket, uint64_t token, FPQuestPtr quest, AnswerCallback* callback, int timeout, bool discardableUDPQuest)
		{
			if (timeout == 0) timeout = _timeoutQuest;
			return _connectionMap.sendQuest(socket, token, quest, callback, timeout, discardableUDPQuest);
		}
		virtual bool sendQuest(int socket, uint64_t token, FPQuestPtr quest, std::function<void (FPAnswerPtr answer, int errorCode)> task, int timeout, bool discardableUDPQuest)
		{
			if (timeout == 0) timeout = _timeoutQuest;
			return _connectionMap.sendQuest(socket, token, quest, std::move(task), timeout, discardableUDPQuest);
		}

		bool joinEpoll(BasicConnection* connection);
#ifdef __APPLE__
		bool waitForEvents(int16_t filter, const BasicConnection* connection);
#else
		bool waitForEvents(uint32_t baseEvent, const BasicConnection* connection);
#endif
		bool waitForRecvEvent(const BasicConnection* connection);
		bool waitForAllEvents(const BasicConnection* connection);
		void exitEpoll(const BasicConnection* connection);

		inline bool joinKqueue(BasicConnection* connection) { return joinEpoll(connection); }
		inline void exitKqueue(BasicConnection* connection) { exitEpoll(connection); }

		inline static bool wakeUpQuestProcessThreadPool(std::shared_ptr<ITaskThreadPool::ITask> task)
		{
			return nakedInstance()->_questProcessPool.wakeUp(task);
		}
		inline static bool wakeUpAnswerCallbackThreadPool(std::shared_ptr<ITaskThreadPool::ITask> task)
		{
			if (nakedInstance()->_answerCallbackPool.wakeUp(task))
				return true;

			if (startEngine())
				return nakedInstance()->_answerCallbackPool.wakeUp(task);
			else
				return false;
		}
		inline void reclaim(IReleaseablePtr object)
		{
			_reclaimer->reclaim(object);
		}

		size_t count() { return _connectionMap.count(); }
	};


	class ConnectionReclaimTask: virtual public IReleaseable
	{
		BasicConnection* _connection;

	public:
		ConnectionReclaimTask(BasicConnection* connection): _connection(connection) {}
		virtual ~ConnectionReclaimTask() { delete _connection; }
		virtual bool releaseable() { return (_connection->_refCount == 0); }
	};
	typedef std::shared_ptr<ConnectionReclaimTask> ConnectionReclaimTaskPtr;



	class ClientCloseTask: virtual public ITaskThreadPool::ITask, virtual public IReleaseable
	{
		bool _error;
		bool _executed;
		BasicConnection* _connection;
		IQuestProcessorPtr _questProcessor;

	public:
		ClientCloseTask(IQuestProcessorPtr questProcessor, BasicConnection* connection, bool error):
			_error(error), _executed(false), _connection(connection), _questProcessor(questProcessor) {}

		virtual ~ClientCloseTask()
		{
			if (!_executed)
				run();

			delete _connection;
		}

		virtual bool releaseable() { return (_connection->_refCount == 0); }
		virtual void run();
	};
}

#endif
