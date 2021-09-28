#ifndef FPNN_Multiple_URL_Engine_H
#define FPNN_Multiple_URL_Engine_H
#include <unistd.h>
#include <string>
#include <atomic>
#include <list>
#include <mutex>
#include <vector>
#include <thread>
#include <memory>
#include <exception>
#include <functional>
#include <unordered_map>
#include <condition_variable>
#include <curl/curl.h>
#include "TaskThreadPoolArray.h"
#include "msec.h"
#include "FPLog.h"
#include "ChainBuffer.h"
#include "ClientEngine.h"

//-- G++ 9: for unused the returned values of write() in MultipleURLEngine::Executor::pushTask()/pushTasks()/stop().
#pragma GCC diagnostic ignored "-Wunused-result"

namespace fpnn {
class MultipleURLEngine
{
public:
	enum VisitStateCode
	{
		VISIT_OK = 0,
		VISIT_READY,
		VISIT_NOT_EXECUTED,
		VISIT_EXPIRED,
		VISIT_TERMINATED,
		VISIT_ADD_TO_ENGINE_FAILED,
		VISIT_UNKNOWN_ERROR
	};

	typedef std::unique_ptr<CURL, void(*)(CURL*)> CURLPtr;

	struct Result
	{
		CURLPtr curlHandle;
		int64_t startTime;	//-- msec
		int responseCode;
		CURLcode curlCode;
		enum VisitStateCode visitState;
		char* errorInfo;
		std::shared_ptr<ChainBuffer> responseBuffer;

		Result(): curlHandle(0, curl_easy_cleanup), startTime(0), responseCode(0),
			curlCode(CURLE_OK), visitState(VISIT_READY), errorInfo(0)
			{}
		~Result() {}
	};

private:
	//=================================================================//
	//- Basic Result Callback
	//=================================================================//
	class BaiscResultCallback: public ITaskThreadPool::ITask
	{
		friend class Executor;
		friend class MultipleURLEngine;

	protected:
		int64_t _startTime;
		CURL* _handle;
		struct curl_slist *_headerChunk;
		std::string _postBody;
		ChainBuffer *_dataBuffer;
		long _responseCode;
		CURLcode _curlCode;
		int64_t _expireSeconds;
		enum VisitStateCode _visitState;
		char* _errorInfo;

		void expired() { _visitState = VISIT_EXPIRED; _curlCode = CURLE_OPERATION_TIMEDOUT; }
		void errored(enum VisitStateCode code, const char *errorInfo)
		{
			_visitState = code;
			_errorInfo = (char *)errorInfo;
			_curlCode = CURLE_UNKNOWN_OPTION;
		}
		void terminated() { _visitState = VISIT_TERMINATED; _curlCode = CURLE_ABORTED_BY_CALLBACK; }
		void completed(CURLcode code)
		{
			if (code == CURLE_OK)
			{
				_visitState = VISIT_OK;
				curl_easy_getinfo(_handle, CURLINFO_RESPONSE_CODE, &_responseCode);
			}
			else if (code == CURLE_OPERATION_TIMEDOUT)
				_visitState = VISIT_EXPIRED;
			else
				_visitState = VISIT_UNKNOWN_ERROR;

			_curlCode = code;
		}
		void fillResult(Result *result)
		{
			if (!result)
				return;

			result->curlHandle = CURLPtr(takeCurlHandle(), curl_easy_cleanup);
			result->startTime = _startTime;
			result->responseCode = (int)_responseCode;
			result->curlCode = _curlCode;
			result->visitState = _visitState;
			result->errorInfo = _errorInfo;
			result->responseBuffer.reset(takeBuffer());
		}

	public:
		BaiscResultCallback(): _startTime(0), _handle(0), _headerChunk(NULL), _dataBuffer(0),
			_responseCode(0), _curlCode(CURLE_OK), _expireSeconds(300), _visitState(VISIT_READY),
			_errorInfo(0) {}
		virtual ~BaiscResultCallback()
		{
			if (_handle)
				curl_easy_cleanup(_handle);

			if (_headerChunk)
				curl_slist_free_all(_headerChunk);

			if (_dataBuffer)
				delete _dataBuffer;
		}

		virtual bool syncedCallback() { return false; }
		int64_t startTime() { return _startTime; }
		CURL* takeCurlHandle()
		{
			CURL* h = _handle;
			_handle = NULL;
			return h;
		}
		ChainBuffer* takeBuffer()
		{
			ChainBuffer* b = _dataBuffer;
			_dataBuffer = 0;
			return b;
		}
	};
	typedef std::shared_ptr<BaiscResultCallback> BaiscResultCallbackPtr;

	//=================================================================//
	//- Synced Result Callback
	//=================================================================//
	class SyncedResultCallback: public BaiscResultCallback
	{
		bool _triggered;
		Result *_result;
		std::mutex _mutex;
		std::condition_variable _condition;

	public:
		SyncedResultCallback(Result* result): BaiscResultCallback(), _triggered(false), _result(result) {}

		virtual ~SyncedResultCallback()
		{
			if (!_triggered)
				resultFilled();
		}
		virtual void run() final {}
		virtual bool syncedCallback() { return true; }

		void resultFilled()
		{
			fillResult(_result);
			std::unique_lock<std::mutex> lck(_mutex);
			_condition.notify_one();
			_triggered = true;
		}

		void waitResult()
		{
			std::unique_lock<std::mutex> lck(_mutex);
  			while (_visitState == VISIT_READY)
  				_condition.wait(lck);
		}
	};

	//=================================================================//
	//- Function Result Callback
	//=================================================================//
	class FunctionResultCallback: public BaiscResultCallback
	{
	private:
		std::function<void (Result &result)> _function;

	public:
		explicit FunctionResultCallback(std::function<void (Result &result)> function):
			BaiscResultCallback(), _function(function) {}
		virtual ~FunctionResultCallback()
		{
		}

  		virtual void run() final
  		{
  			Result result;
  			fillResult(&result);
  			_function(result);
  		}
	};

public:
	//=================================================================//
	//- Result Callback:
	//=================================================================//
	class ResultCallback: public BaiscResultCallback
	{
	public:
		ResultCallback(): BaiscResultCallback() {}
		virtual ~ResultCallback()
		{
		}
		virtual void run() final
		{
			if (_visitState == VISIT_OK)
			{
				Result result;
  				fillResult(&result);
  				onCompleted(result);
			}
			else
			{
				CURL* easy = takeCurlHandle();
				CURLPtr cup(easy, curl_easy_cleanup);
				if (_visitState == VISIT_EXPIRED)
				{
					onExpired(std::move(cup));
				}
				else if (_visitState == VISIT_TERMINATED)
				{
					onTerminated(std::move(cup));
				}
				else
				{
					onException(std::move(cup), _visitState, _errorInfo);
				}
			}
		}

		virtual void onCompleted(Result &result) = 0;
		virtual void onExpired(CURLPtr curl_unique_ptr) = 0;
		virtual void onTerminated(CURLPtr curl_unique_ptr) = 0;
		virtual void onException(CURLPtr curl_unique_ptr, enum VisitStateCode errorCode, const char *errorInfo) = 0;
	};
	typedef std::shared_ptr<ResultCallback> ResultCallbackPtr;

private:
	//=================================================================//
	//- Executor
	//=================================================================//
	class Executor
	{
	private:
#ifdef __APPLE__
		int _kqueue_fd;
		int _max_events;
		struct kevent* _kqueueEvents;
#else
		int _epoll_fd;
		int _max_events;
		struct epoll_event* _epollEvents;
#endif
		int _eventNotifyFds[2];
		int _waitMilliseconds;
		int _concurrentCount;
		bool _immediateCall;

		std::mutex _mutex;
		std::condition_variable _condition;
		int64_t _activeTime;

		std::list<BaiscResultCallbackPtr> _queue;
		std::atomic<int64_t> _load;
		std::atomic<int64_t> _queueSize;
		std::thread _executor;

		volatile bool _running;
		volatile bool _busy;

		CURLM *_multi;
		TaskThreadPool* _taskPool;
		std::unordered_map<CURL*, BaiscResultCallbackPtr> _infoMap;

	private:
		void execute_thread();
#ifdef __APPLE__
		bool initKqueue();
		void stopKqueue();
#else
		bool initEpoll();
		void stopEpoll();
#endif
		bool start();
		void clean();
		void cleanTaskQueue();

		static int socketCallback(CURL *easy, curl_socket_t s, int what, void *userp, void *socketp);
		static int timerCallback(CURLM *multi, long timeout_ms, void *userp);

		int takeOverTasks();
		void checkTimeoutTask();
		void drainAddEvent(int fd);
		void checkConnectionState();

	public:
		static size_t writeCallback(char *ptr, size_t size, size_t nmemb, void *userdata);

		Executor(int concurrentCount, TaskThreadPool* taskPool):
#ifdef __APPLE__
			_kqueue_fd(0), _max_events(1000), _kqueueEvents(0), _waitMilliseconds(500),
#else
			_epoll_fd(0), _max_events(1000), _epollEvents(0), _waitMilliseconds(500),
#endif
			_concurrentCount(concurrentCount), _immediateCall(false), _load(0), _queueSize(0),
			_running(false), _busy(false), _multi(0), _taskPool(taskPool)
		{
			_eventNotifyFds[0] = 0;
			_eventNotifyFds[1] = 0;
			_activeTime = slack_mono_sec();
		}
		~Executor() { stop(); }

		bool pushTask(BaiscResultCallbackPtr task)
		{
			task->_startTime = slack_real_msec();
			std::unique_lock<std::mutex> lck(_mutex);
			if (!_running)
				return false;

			_load++;
			_queueSize++;
			_queue.push_back(task);
			if (_busy)
			{
				write(_eventNotifyFds[1], this, 4);
			}
			else
			{
				_condition.notify_one();
			}

			return true;
		}
		bool pushTasks(std::vector<BaiscResultCallbackPtr> &tasks)
		{
			int64_t now = slack_real_msec();
			std::unique_lock<std::mutex> lck(_mutex);
			if (!_running)
				return false;

			_load.fetch_add((int64_t)tasks.size());
			_queueSize.fetch_add((int64_t)tasks.size());

			for (auto task: tasks)
			{
				task->_startTime = now;
				_queue.push_back(task);
			}

			if (_busy)
			{
				write(_eventNotifyFds[1], this, 4);
			}
			else
			{
				_condition.notify_one();
			}

			return true;
		}
		bool launch()
		{
			std::unique_lock<std::mutex> lck(_mutex);
			if (_running)
				return false;

			return start();
		}
		void stop()
		{
			{
				std::unique_lock<std::mutex> lck(_mutex);
				if (_running)
				{
					_running = false;
					
					if (_busy)
						write(_eventNotifyFds[1], this, 4);
					else
						_condition.notify_one();
				}
			}

			_executor.join();
		}

		void checkTimeoutedQueuedSyncTask();

		int64_t load() { return _load; }
		int64_t queueSize() { return _queueSize; }
		int64_t activeTime() { return _activeTime; }
	};
	typedef std::shared_ptr<Executor> ExecutorPtr;

public:
	static bool init(bool initSSL = true, bool initCurl = true);
	static void cleanup();

	/**
	 *  If maxThreadCount <= 0, maxThreadCount will be assigned 1000.
	 *  If callbackPool is nullptr, MultipleURLEngine will use the answerCallbackPool of ClientEngine.
	 */
	MultipleURLEngine(int nConnsInPerThread = 200, int initThreadCount = 1, int perfectThreadCount = 10,
						int maxThreadCount = 100, int maxConcurrentCount = 25000,
						int tempThreadLatencySeconds = 120,
						std::shared_ptr<TaskThreadPool> callbackPool = nullptr);
	~MultipleURLEngine();
	
	//==================================================================//
	//=  Single URL 
	//==================================================================//
	//---------------------------------------//
	//-          Sync Interfaces            -//
	//---------------------------------------//
	bool visit(const std::string& url, Result &result, int timeoutSeconds = 120,
		bool saveResponseData = true, const std::string& postBody = std::string(),
		const std::vector<std::string>& header = std::vector<std::string>());

	/* If return true, don't cleanup curl; if return false, please cleanup curl. */
	bool visit(CURL *curl, Result &result, int timeoutSeconds = 120,
		bool saveResponseData = true, const std::string& postBody = std::string(),
		const std::vector<std::string>& header = std::vector<std::string>());

	//---------------------------------------//
	//-  Async Interfaces: callback classes -//
	//---------------------------------------//
	bool visit(const std::string& url, ResultCallbackPtr callback, int timeoutSeconds = 120,
		bool saveResponseData = true, const std::string& postBody = std::string(),
		const std::vector<std::string>& header = std::vector<std::string>());

	/* If return true, don't cleanup curl; if return false, please cleanup curl. */
	inline bool visit(CURL *curl, ResultCallbackPtr callback, int timeoutSeconds = 120,
		bool saveResponseData = true, const std::string& postBody = std::string(),
		const std::vector<std::string>& header = std::vector<std::string>())
	{
		return real_visit(curl, callback, timeoutSeconds, saveResponseData, header, postBody);
	}

	//---------------------------------------//
	//-  Async Interfaces: Lambda callback  -//
	//---------------------------------------//
	bool visit(const std::string& url, std::function<void (Result &result)> callback,
		int timeoutSeconds = 120, bool saveResponseData = true, const std::string& postBody = std::string(),
		const std::vector<std::string>& header = std::vector<std::string>());

	/* If return true, don't cleanup curl; if return false, please cleanup curl. */
	inline bool visit(CURL *curl, std::function<void (Result &result)> callback,
		int timeoutSeconds = 120, bool saveResponseData = true, const std::string& postBody = std::string(),
		const std::vector<std::string>& header = std::vector<std::string>())
	{
		std::shared_ptr<FunctionResultCallback>  fcb(new FunctionResultCallback(std::move(callback)));
		return real_visit(curl, fcb, timeoutSeconds, saveResponseData, header, postBody);
	}

	//==================================================================//
	//=  Batch URLs 
	//==================================================================//
	//---------------------------------------//
	//-  Async Interfaces: callback classes -//
	//---------------------------------------//
	bool addToBatch(const std::string& url, ResultCallbackPtr callback, int timeoutSeconds = 120,
		bool saveResponseData = true, const std::string& postBody = std::string(),
		const std::vector<std::string>& header = std::vector<std::string>());

	/* If return true, don't cleanup curl; if return false, please cleanup curl. */
	inline bool addToBatch(CURL *curl, ResultCallbackPtr callback, int timeoutSeconds = 120,
		bool saveResponseData = true, const std::string& postBody = std::string(),
		const std::vector<std::string>& header = std::vector<std::string>())
	{
		return real_addToBatch(curl, callback, timeoutSeconds, saveResponseData, header, postBody);
	}

	//---------------------------------------//
	//-  Async Interfaces: Lambda callback  -//
	//---------------------------------------//
	bool addToBatch(const std::string& url, std::function<void (Result &result)> callback,
		int timeoutSeconds = 120, bool saveResponseData = true, const std::string& postBody = std::string(),
		const std::vector<std::string>& header = std::vector<std::string>());

	/* If return true, don't cleanup curl; if return false, please cleanup curl. */
	inline bool addToBatch(CURL *curl, std::function<void (Result &result)> callback,
		int timeoutSeconds = 120, bool saveResponseData = true, const std::string& postBody = std::string(),
		const std::vector<std::string>& header = std::vector<std::string>())
	{
		std::shared_ptr<FunctionResultCallback>  fcb(new FunctionResultCallback(std::move(callback)));
		return real_addToBatch(curl, fcb, timeoutSeconds, saveResponseData, header, postBody);
	}

	bool commitBatch();
	std::string status();

	struct StatusInfo
	{
		struct ConfigInfo
		{
			int connsInPerThread;		//-- 每个线程最大任务并发数。
			int perfectThreadCount;		//-- 空闲时，保留线程限制
			int maxThreadCount;			//-- 最大线程数
			int maxConcurrentCount;		//-- 并发总上限
		} configInfo;
		
		struct CurrentInfo
		{
			int64_t threads;	//-- 当前线程数
			int64_t loads;		//-- 当前总任务数
			int64_t connections;	//-- 当前已建立连接的数目
			int64_t queuedTaskCount;	//-- 当前 queued 住的任务数量
		} currentInfo;
	};

	void status(StatusInfo &statusInfo);

private:
	int _nConnsInPerThread;
	int _perfectThreadCount;
	int _maxThreadCount;
	int _maxConcurrentCount;
	int _tempThreadLatencySeconds;
	std::vector<ExecutorPtr> _executors;
	std::shared_ptr<TaskThreadPool> _callbackPool;
	fpnn::ClientEnginePtr _engine;

	bool _running;
	std::mutex _mutex;
	std::thread _reclaimer;

	void rangeAdjust(int &value, int minValue, int maxValue);
	void ceilingAdjust(int &targetValue, int comparedValue);
	void floorAdjust(int &targetValue, int comparedValue);
	void ReviseDataRelation(int &initThreadCount);
	bool pushTask(BaiscResultCallbackPtr task);
	bool pushTasks(std::vector<BaiscResultCallbackPtr> &tasks);
	void reclaimThread();
	void reclaimCheck();
	void checkTimeoutedQueuedSyncTask();
	int getSystemAvailablePortsCount();

	bool prepareEasyHandle(CURL *curl, BaiscResultCallbackPtr callback, int timeoutSeconds,
		bool saveResponseData, const std::vector<std::string>& headers, const std::string& postBody);

	bool real_visit(CURL *curl, BaiscResultCallbackPtr callback, int timeoutSeconds,
		bool saveResponseData, const std::vector<std::string>& headers, const std::string& postBody);

	bool real_addToBatch(CURL *curl, BaiscResultCallbackPtr callback, int timeoutSeconds,
		bool saveResponseData, const std::vector<std::string>& headers, const std::string& postBody);

	static void cleanLocalCache(std::vector<BaiscResultCallbackPtr> *);
	static thread_local std::unique_ptr<std::vector<BaiscResultCallbackPtr>, void(*)(std::vector<BaiscResultCallbackPtr> *)> _localTaskCache; 
};
typedef std::shared_ptr<MultipleURLEngine> MultipleURLEnginePtr;
}
#endif
