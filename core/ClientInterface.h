#ifndef FPNN_Client_Interface_H
#define FPNN_Client_Interface_H

#include <atomic>
#include <mutex>
#include <memory>
#include <functional>
#include <condition_variable>
#include "AnswerCallbacks.h"
#include "ClientEngine.h"
#include "IQuestProcessor.h"

namespace fpnn
{
	class Client;
	typedef std::shared_ptr<Client> ClientPtr;

	class Client
	{
	protected:
		enum class ConnStatus
		{
			NoConnected,
			Connecting,
			KeyExchanging,
			Connected,
		};

	protected:
		std::mutex _mutex;
		std::condition_variable _condition;		//-- Only for connected.
		bool _isIPv4;
		std::atomic<bool> _connected;
		ConnStatus _connStatus;
		ClientEnginePtr _engine;

		IQuestProcessorPtr _questProcessor;
		ConnectionInfoPtr	_connectionInfo;
		std::string _endpoint;

		TaskThreadPool* _answerCallbackPool;	//-- answer callback & error/close event.
		TaskThreadPoolPtr _questProcessPool;

		int64_t _timeoutQuest;
		bool _autoReconnect;

	private:
		void onErrorOrCloseEvent(BasicConnection* connection, bool error);

	public:
		Client(const std::string& host, int port, bool autoReconnect = true);
		virtual ~Client();
		/*===============================================================================
		  Call by framwwork.
		=============================================================================== */
		void connected(BasicConnection* connection);
		void willClose(BasicConnection* connection)		//-- must done in thread pool or other thread.
		{
			onErrorOrCloseEvent(connection, false);
		}
		void errorAndWillBeClosed(BasicConnection* connection)		//-- must done in thread pool or other thread.
		{
			onErrorOrCloseEvent(connection, true);
		}
		void dealAnswer(FPAnswerPtr answer, ConnectionInfoPtr connectionInfo);		//-- must done in thread pool or other thread.
		void processQuest(FPQuestPtr quest, ConnectionInfoPtr connectionInfo);
		void clearConnectionQuestCallbacks(BasicConnection* connection, int errorCode);
		/*===============================================================================
		  Call by anybody.
		=============================================================================== */
		inline bool connected() { return _connected; }
		inline const std::string& endpoint() { return _endpoint; }
		inline int socket()
		{
			std::unique_lock<std::mutex> lck(_mutex);
			return _connectionInfo->socket;
		}
		inline ConnectionInfoPtr connectionInfo()
		{
			std::unique_lock<std::mutex> lck(_mutex);
			return _connectionInfo;
		}
		/*===============================================================================
		  Call by Developer. Configure Function.
		=============================================================================== */
		inline void setQuestProcessor(IQuestProcessorPtr processor)
		{
			_questProcessor = processor;
			_questProcessor->setConcurrentSender(_engine.get());
		}

		inline void setQuestProcessThreadPool(TaskThreadPoolPtr questProcessPool)
		{
			_questProcessPool = questProcessPool;
		}

		inline void enableQuestProcessThreadPool(int32_t initCount, int32_t perAppendCount, int32_t perfectCount, int32_t maxCount, size_t maxQueueSize)
		{
			if (_questProcessPool)
				return;
			_questProcessPool.reset(new TaskThreadPool());
			_questProcessPool->init(initCount, perAppendCount, perfectCount, maxCount, maxQueueSize);
		}

		//-- answer callback & error/close event.
		inline void enableAnswerCallbackThreadPool(int32_t initCount, int32_t perAppendCount, int32_t perfectCount, int32_t maxCount)
		{
			if (_answerCallbackPool)
				return;
			_answerCallbackPool = new TaskThreadPool;
			_answerCallbackPool->init(initCount, perAppendCount, perfectCount, maxCount);
		}

		inline void setQuestTimeout(int64_t seconds)
		{
			_timeoutQuest = seconds * 1000;
		}
		inline int64_t getQuestTimeout()
		{
			return _timeoutQuest / 1000;
		}
		/*===============================================================================
		  Call by Developer.
		=============================================================================== */
		virtual bool connect() = 0;
		virtual void close();
		virtual bool reconnect();

		/**
			All SendQuest():
				If return false, caller must free quest & callback.
				If return true, don't free quest & callback.

			timeout in seconds.
		*/
		virtual FPAnswerPtr sendQuest(FPQuestPtr quest, int timeout = 0);
		virtual bool sendQuest(FPQuestPtr quest, AnswerCallback* callback, int timeout = 0);
		virtual bool sendQuest(FPQuestPtr quest, std::function<void (FPAnswerPtr answer, int errorCode)> task, int timeout = 0);

		static TCPClientPtr createTCPClient(const std::string& host, int port, bool autoReconnect = true);
		static TCPClientPtr createTCPClient(const std::string& endpoint, bool autoReconnect = true);

		static UDPClientPtr createUDPClient(const std::string& host, int port, bool autoReconnect = true);
		static UDPClientPtr createUDPClient(const std::string& endpoint, bool autoReconnect = true);
	};
}

#endif