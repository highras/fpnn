#ifndef FPNN_UDP_Epoll_Server_H
#define FPNN_UDP_Epoll_Server_H

#include <atomic>
#include <string>
#include "Config.h"
#include "ParamTemplateThreadPoolArray.h"
#include "TaskThreadPoolArray.h"
#include "ServerController.h"
#include "ServerInterface.h"
#include "RecordIPList.h"
#include "UDPIOBuffer.h"
#include "UDPSessions.h"
#include "UDPServerMasterProcessor.h"
#include "ConcurrentSenderInterface.h"

namespace fpnn
{
#define FPNN_DEFAULT_UDP_WORK_POOL_QUEUE_SIZE	2000000

	class UDPEpollServer;
	typedef std::shared_ptr<UDPEpollServer> UDPServerPtr;

	class UDPEpollServer: virtual public ServerInterface, virtual public IConcurrentUDPSender
	{
	private:
		uint16_t _port;
		std::string _ip;
		uint16_t _port6;
		std::string _ipv6;

		int _socket;
		int _socket6;
		int _epoll_fd;

		const int _max_events;
		struct epoll_event* _epollEvents;

		std::atomic<bool> _running;
		volatile bool _stopping;
		std::atomic<bool> _stopSignalNotified;

		int _closeNotifyFds[2];
		size_t _maxWorkerPoolQueueLength;
		int64_t _timeoutQuest; //only valid when as a client

		UDPSendBuffer _sendBuffer;
		UDPSendBuffer _sendBuffer6;
		PartitionedCallbackMap _callbackMap;
		std::shared_ptr<UDPServerMasterProcessor> _serverMasterProcessor;
		std::shared_ptr<ParamTemplateThreadPoolArray<UDPRequestPackage *>> _workerPool;
		std::shared_ptr<TaskThreadPoolArray> _answerCallbackPool;

		std::atomic<bool> _enableIPWhiteList;
		IPWhiteList _ipWhiteList;

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
		bool initEpoll();
		void initFailClean(const char* fail_info);
		void updateSocketStatus(int socket, bool needWaitSendEvent);
		void processSendEvent(int socket);
		void processEvent(struct epoll_event & event, UDPRecvBuffer &recvBuffer);
		void deliverAnswer(ConnectionInfoPtr connInfo, FPAnswerPtr answer);
		void deliverQuest(ConnectionInfoPtr connInfo, FPQuestPtr quest);
		bool returnServerStoppingAnswer(ConnectionInfoPtr connInfo, FPQuestPtr quest);
		void clearRemnantCallbacks();
		void clean();
		void exitCheck();
		bool sendQuestWithBasicAnswerCallback(ConnectionInfoPtr connInfo, FPQuestPtr quest, BasicAnswerCallback* callback, int timeout);

	private:
		UDPEpollServer(): _port(0), _port6(0), _socket(0), _socket6(0), _epoll_fd(0), _max_events(16),
			_epollEvents(NULL), _running(false), _stopping(false), _stopSignalNotified(false),
			_maxWorkerPoolQueueLength(FPNN_DEFAULT_UDP_WORK_POOL_QUEUE_SIZE),
			_timeoutQuest(FPNN_DEFAULT_QUEST_TIMEOUT * 1000),
			_serverMasterProcessor(new UDPServerMasterProcessor()), _enableIPWhiteList(false)
		{
			_closeNotifyFds[0] = 0;
			_closeNotifyFds[1] = 0;

			ServerController::installSignal();

			Config::initSystemVaribles();

			initServerVaribles();
		}
	public:
		virtual ~UDPEpollServer() { exitCheck(); }

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

		inline void configWorkerThreadPool(int32_t initCount, int32_t perAppendCount, int32_t perfectCount, int32_t maxCount, size_t maxQueueSize)
		{
			_workerPool.reset(new ParamTemplateThreadPoolArray<UDPRequestPackage *>(getCPUCount(), _serverMasterProcessor));
			_workerPool->init(initCount, perAppendCount, perfectCount, maxCount, maxQueueSize);
		}
		inline void enableAnswerCallbackThreadPool(int32_t initCount, int32_t perAppendCount, int32_t perfectCount, int32_t maxCount)
		{
			if (_answerCallbackPool)
				return;

			_answerCallbackPool.reset(new TaskThreadPoolArray(getCPUCount()));
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

		virtual void sendData(ConnectionInfoPtr connInfo, std::string* data);
		//virtual void sendAnswer(ConnectionInfoPtr connInfo, FPAnswerPtr answer);

		/**
			All SendQuest():
				If return false, caller must free quest & callback.
				If return true, don't free quest & callback.
		*/
		virtual FPAnswerPtr sendQuest(ConnectionInfoPtr connInfo, FPQuestPtr quest, int timeout = 0);
		virtual bool sendQuest(ConnectionInfoPtr connInfo, FPQuestPtr quest, AnswerCallback* callback, int timeout = 0)
		{
			if (timeout == 0) timeout = _timeoutQuest;
			return sendQuestWithBasicAnswerCallback(connInfo, quest, callback, timeout);
		}
		virtual bool sendQuest(ConnectionInfoPtr connInfo, FPQuestPtr quest, std::function<void (FPAnswerPtr answer, int errorCode)> task, int timeout = 0)
		{
			if (timeout == 0) timeout = _timeoutQuest;

			BasicAnswerCallback* t = new FunctionAnswerCallback(std::move(task));
			if (sendQuestWithBasicAnswerCallback(connInfo, quest, t, timeout))
				return true;
			else
			{
				delete t;
				return false;
			}
		}
		
		void checkTimeout();
	};
}

#endif