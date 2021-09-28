#include <set>
#include <openssl/crypto.h>
#include "AutoRelease.h"
#include "StringUtil.h"
#include "NetworkUtility.h"
#include "MultipleURLEngine.h"

//-- G++ 8: for ignored unused-value warning triggered by CRYPTO_THREADID_set_callback()
#pragma GCC diagnostic ignored "-Wunused-value"

//-- G++ 8 & 9: for ignored unused-function warning triggered by OpenSSL required
//--            CRYPTO_THREADID_set_callback(OpenSSL_thread_Id_Func) & CRYPTO_set_locking_callback(OpenSSL_locking_callback)
#pragma GCC diagnostic ignored "-Wunused-function"

using namespace fpnn;
//====================================================//
//-        OpenSSL settings for multi-threads        -//
//====================================================//
//-- https://www.openssl.org/docs/man1.0.1/crypto/CRYPTO_lock.html
static std::mutex* G_SSL_mutexArray = NULL;

static void OpenSSL_thread_Id_Func(CRYPTO_THREADID *tid)
{
	CRYPTO_THREADID_set_pointer(tid, &errno);
}

static void OpenSSL_locking_callback(int mode, int n, const char *file, int line)
{
	if (mode & CRYPTO_LOCK)
		G_SSL_mutexArray[n].lock();
	else if (mode & CRYPTO_UNLOCK)
		G_SSL_mutexArray[n].unlock();
	else
	{
		if (mode & CRYPTO_READ || mode & CRYPTO_WRITE)
		{
			LOG_ERROR("OpenSSL_locking_callback: CRYPTO_READ | CRYPTO_WRITE without CRYPTO_LOCK & CRYPTO_UNLOCK");
		}
		else
			LOG_ERROR("OpenSSL_locking_callback: Unknown bits flag. %x", mode);
	}
}

static void SSL_thread_setup()
{
	if (G_SSL_mutexArray)
		return;

	G_SSL_mutexArray = new std::mutex[CRYPTO_num_locks()];

	CRYPTO_THREADID_set_callback(OpenSSL_thread_Id_Func);
	CRYPTO_set_locking_callback(OpenSSL_locking_callback);
}

static void SSL_thread_cleanup()
{
	if (!G_SSL_mutexArray)
		return;

	CRYPTO_THREADID_set_callback(NULL);
	CRYPTO_set_locking_callback(NULL);

	delete [] G_SSL_mutexArray;
	G_SSL_mutexArray = NULL;
}

//====================================================//
//-   Global Static Functions of MultipleURLEngine   -//
//====================================================//
static bool G_cleanSSL = false;
static bool G_cleanCurl = false;
static std::atomic<int> refenceCount(0);
static std::atomic<int64_t> concurrentConnectionCount(0);
bool MultipleURLEngine::init(bool initSSL, bool initCurl)
{
	if (initCurl && curl_global_init(CURL_GLOBAL_ALL))
	{
		LOG_ERROR("MultipleURLEngine curl_global_init() function failed!");
		return false;
	}

	if (initSSL)
		SSL_thread_setup();

	G_cleanSSL = initSSL;
	G_cleanCurl = initCurl;
	refenceCount++;
	return true;
}

void MultipleURLEngine::cleanup()
{
	refenceCount--;
	if (refenceCount > 0)
		return;

	if (G_cleanSSL)
		SSL_thread_cleanup();

	if (G_cleanCurl)
		curl_global_cleanup();
}

//========================================//
//- Thread local storage
//========================================//
void MultipleURLEngine::cleanLocalCache(std::vector<BaiscResultCallbackPtr> *taskVector)
{
	if (!taskVector)
		return;

	size_t count = taskVector->size();
	if (count)
	{
		LOG_ERROR("Local task cache has %d tasks not push into MultipleURLEngine when thread exiting.", (int)count);
	}
	for (size_t i = 0; i < count; i++)
	{
		(*taskVector)[i]->errored(VISIT_NOT_EXECUTED, "Task isn't executed since thread exit.");
		try
		{
			(*taskVector)[i]->run();
		}
		catch (...)
		{
			LOG_ERROR("Unknown exception be catched when running BaiscResultCallback in thread cleaning function.");
		}
	}
	delete taskVector;
}
thread_local std::unique_ptr<std::vector<MultipleURLEngine::BaiscResultCallbackPtr>, void(*)(std::vector<MultipleURLEngine::BaiscResultCallbackPtr> *)> MultipleURLEngine::_localTaskCache(NULL, cleanLocalCache); 

//========================================//
//- Executor of MultipleURLEngine
//========================================//
int MultipleURLEngine::Executor::socketCallback(CURL *easy, curl_socket_t s, int what, void *userp, void *socketp)
{
	MultipleURLEngine::Executor* executor = (MultipleURLEngine::Executor*)userp;
	
	int16_t filter = 0;

	if (what == CURL_POLL_REMOVE)
	{
		struct kevent ev;
		EV_SET(&ev, s, EVFILT_READ | EVFILT_WRITE, EV_DELETE, 0, 0, NULL);
		kevent(executor->_kqueue_fd, &ev, 1, NULL, 0, NULL);

		concurrentConnectionCount--;
		return 0;
	}

	if (what == CURL_POLL_IN || what == CURL_POLL_INOUT)
		filter |= EVFILT_READ;
	if (what == CURL_POLL_OUT || what == CURL_POLL_INOUT)
		filter |= EVFILT_WRITE;

	if (filter)
	{
		struct kevent ev;
		EV_SET(&ev, s, filter, EV_ADD | EV_CLEAR, 0, 0, NULL);

		if (kevent(executor->_kqueue_fd, &ev, 1, NULL, 0, NULL) == -1)
		{
			if (errno == EEXIST)
			{
				if (kevent(executor->_kqueue_fd, &ev, 1, NULL, 0, NULL) == -1)
					LOG_ERROR("MultipleURLEngine::Executor::socketCallback() mod socket to kqueue failed.");
			}
			else
				LOG_ERROR("MultipleURLEngine::Executor::socketCallback() adds socket to kqueue failed.");
		}
		else
		{
			int64_t ccc = concurrentConnectionCount++;
			if (ccc > 25000)
				LOG_ERROR("MultipleURLEngine concurrent connection count has reached 25000!!!");
		}
	}

	return 0;
}

int MultipleURLEngine::Executor::timerCallback(CURLM *multi, long timeout_ms, void *userp)
{
	if (timeout_ms < 0)
		return 0;

	MultipleURLEngine::Executor* executor = (MultipleURLEngine::Executor*)userp;
	if (timeout_ms > 0)
	{
		if (executor->_waitMilliseconds > timeout_ms)
			executor->_waitMilliseconds = timeout_ms;
	}

	if (timeout_ms == 0)
		executor->_immediateCall = true;

	return 0;
}

size_t MultipleURLEngine::Executor::writeCallback(char *ptr, size_t size, size_t nmemb, void *userdata)
{
	ChainBuffer* buffer = (ChainBuffer*)userdata;
	buffer->append(ptr, size * nmemb);
	return size * nmemb;
}

bool MultipleURLEngine::Executor::start()
{
	_multi = curl_multi_init();
	if (!_multi)
		return false;

	if (curl_multi_setopt(_multi, CURLMOPT_SOCKETFUNCTION, socketCallback) != CURLM_OK)
	{
		curl_multi_cleanup(_multi);
		_multi = NULL;
		return false;
	}
	if (curl_multi_setopt(_multi, CURLMOPT_SOCKETDATA, this) != CURLM_OK)
	{
		curl_multi_cleanup(_multi);
		_multi = NULL;
		return false;
	}
	if (curl_multi_setopt(_multi, CURLMOPT_TIMERFUNCTION, timerCallback) != CURLM_OK)
	{
		curl_multi_cleanup(_multi);
		_multi = NULL;
		return false;
	}
	if (curl_multi_setopt(_multi, CURLMOPT_TIMERDATA, this) != CURLM_OK)
	{
		curl_multi_cleanup(_multi);
		_multi = NULL;
		return false;
	}

	if (!initKqueue())
	{
		curl_multi_cleanup(_multi);
		_multi = NULL;
		return false;
	}

	_running = true;
	_activeTime = slack_mono_sec();
	_executor = std::thread(&Executor::execute_thread, this);
	return true;
}

bool MultipleURLEngine::Executor::initKqueue()
{
	_kqueue_fd = kqueue();
	if (_kqueue_fd == -1)
	{
		_kqueue_fd = 0;
		return false;
	}

	_kqueueEvents = new(std::nothrow) struct kevent[_max_events];
	if (_kqueueEvents == NULL)
	{
		close(_kqueue_fd);
		_kqueue_fd = 0;
		return false;
	}

	if (pipe(_eventNotifyFds))
	{
		stopKqueue();
		return false;
	}

	nonblockedFd(_eventNotifyFds[0]);

	struct kevent ev;
	EV_SET(&ev, _eventNotifyFds[0], EVFILT_READ, EV_ADD | EV_CLEAR, 0, 0, NULL);

	if (kevent(_kqueue_fd, &ev, 1, NULL, 0, NULL) == -1)
	{
		stopKqueue();
		return false;
	}

	return true;
}

void MultipleURLEngine::Executor::stopKqueue()
{
	if (_eventNotifyFds[0])
	{
		close(_eventNotifyFds[1]);
		close(_eventNotifyFds[0]);
		_eventNotifyFds[0] = 0;
		_eventNotifyFds[1] = 0;
	}
	
	if (_kqueue_fd)
	{
		close(_kqueue_fd);
		_kqueue_fd = 0;
	}

	if (_kqueueEvents)
	{
		delete [] _kqueueEvents;
		_kqueueEvents = NULL;
	}
}

void MultipleURLEngine::Executor::clean()
{
	{
		std::unique_lock<std::mutex> lck(_mutex);

		for (auto& cbPair: _infoMap)
			curl_multi_remove_handle(_multi, cbPair.first);

		stopKqueue();
		curl_multi_cleanup(_multi);
		_multi = NULL;
		_running = false;
	}

	for (auto& cbPair: _infoMap)
	{
		BaiscResultCallbackPtr callback = cbPair.second;
		callback->terminated();

		_load--;

		if (callback->syncedCallback())
		{
			SyncedResultCallback* sync = (SyncedResultCallback *)callback.get();
			sync->resultFilled();
		}
		else
		{
			if (_taskPool)
				_taskPool->wakeUp(callback);
			else
				fpnn::ClientEngine::wakeUpAnswerCallbackThreadPool(callback);
		}
	}

	cleanTaskQueue();
}

void MultipleURLEngine::Executor::checkTimeoutTask()
{
	int64_t now = slack_real_msec();

	std::set<CURL*> timeouted;

	for (auto& cbPair: _infoMap)
	{
		BaiscResultCallbackPtr callback = cbPair.second;
		if (callback->_expireSeconds == 0 || (now - callback->_startTime <= callback->_expireSeconds * 1000))
			continue;

		timeouted.insert(cbPair.first);
		curl_multi_remove_handle(_multi, cbPair.first);
		callback->expired();

		_load--;

		if (callback->syncedCallback())
		{
			SyncedResultCallback* sync = (SyncedResultCallback *)callback.get();
			sync->resultFilled();
		}
		else
		{
			if (_taskPool)
				_taskPool->wakeUp(callback);
			else
				fpnn::ClientEngine::wakeUpAnswerCallbackThreadPool(callback);
		}
	}

	for (auto& curl: timeouted)
		_infoMap.erase(curl);
}

void MultipleURLEngine::Executor::drainAddEvent(int fd)
{
	const int bufferSize = 32;
	unsigned char out[bufferSize];
	
	while (true)
	{
		int readBytes = (int)::read(fd, out, bufferSize);
		if (readBytes < bufferSize)
			return;
	}
}
void MultipleURLEngine::Executor::execute_thread()
{
	_busy = true;
	while (_running)
	{
		_activeTime = slack_mono_sec();

		bool needActive = true;
		int running_handles = 0;

		struct timespec TimeSpace;

		TimeSpace.tv_sec  = _waitMilliseconds / 1000;
		TimeSpace.tv_nsec = (_waitMilliseconds % 1000) * 1000 * 1000;

		int readyfdsCount = kevent(_kqueue_fd, NULL, 0, _kqueueEvents, _max_events, &TimeSpace);
		if (!_running)
		{
			clean();
			return;
		}

		_activeTime = slack_mono_sec();
		_waitMilliseconds = 5000;

		if (_queueSize > 0)
			takeOverTasks();

		if (readyfdsCount == -1)
		{
			if (errno == EINTR || errno == EFAULT)
				continue;

			if (errno == EBADF || errno == EINVAL)
			{
				LOG_ERROR("Invalid kqueue fd.");
			}
			else
			{
				LOG_ERROR("Unknown Error when kevent() errno: %d", errno);
			}

			clean();
			return;
		}

		for (int i = 0; i < readyfdsCount; i++)
		{
			if (_kqueueEvents[i].ident == _eventNotifyFds[0])
				drainAddEvent(_eventNotifyFds[0]);
			else
			{
				int curlMask = 0;

				if (_kqueueEvents[i].filter & EVFILT_READ)
					curlMask |= CURL_CSELECT_IN;
				if (_kqueueEvents[i].filter & EVFILT_WRITE)
					curlMask |= CURL_CSELECT_OUT;

				curl_multi_socket_action(_multi, _kqueueEvents[i].ident, curlMask, &running_handles);
				needActive = false;
			}
		}

		if (needActive || _immediateCall)
		{
			int running;
			do
			{
				_immediateCall = false;
				CURLMcode code = curl_multi_socket_action(_multi, CURL_SOCKET_TIMEOUT, 0, &running);
				if (code != CURLM_OK)
					LOG_ERROR("curl_multi_socket_action return %d when needActive or immediateCall.", code);
			} while (_immediateCall);
		}

		_activeTime = slack_mono_sec();
		checkConnectionState();
		checkTimeoutTask();

		if (_load == 0)
		{
			_activeTime = slack_mono_sec();
			{
				std::unique_lock<std::mutex> lck(_mutex);
				if (_load == 0)
				{
					_busy = false;
					while (_load == 0 && _running)
						_condition.wait(lck);
				}
				_busy = true;
			}

			_activeTime = slack_mono_sec();
			if (_queueSize > 0)
			{
				takeOverTasks();

				int running;
				do
				{
					_immediateCall = false;
					CURLMcode code = curl_multi_socket_action(_multi, CURL_SOCKET_TIMEOUT, 0, &running);
					if (code != CURLM_OK)
						LOG_ERROR("curl_multi_socket_action return %d after wake up.", code);
				} while (_immediateCall);
			}
		}
	}
	clean();
}

int MultipleURLEngine::Executor::takeOverTasks()
{
	std::list<BaiscResultCallbackPtr> queue;
	{
		std::unique_lock<std::mutex> lck(_mutex);
		int64_t realLoad = _load - _queueSize;
		while (_queueSize && (int)realLoad < _concurrentCount)
		{
			queue.push_back(_queue.front());
			_queue.pop_front();
			_queueSize--;
		}
	}

	int taken = 0;
	int64_t now = slack_real_msec();
	auto it = queue.begin();
	while (it != queue.end())
	{
		BaiscResultCallbackPtr task = *it;

		if (task->syncedCallback() == false)
			task->_startTime = now;

		CURLMcode code = curl_multi_add_handle(_multi, task->_handle);
		if (code == CURLM_OK)
		{
			_infoMap[task->_handle] = task;
			taken += 1;
		}
		else
		{
			_load--;
			task->errored(VISIT_ADD_TO_ENGINE_FAILED, "Add to curl multi-handle failed");

			if (task->syncedCallback())
			{
				SyncedResultCallback* sync = (SyncedResultCallback *)task.get();
				sync->resultFilled();
			}
			else
			{
				if (_taskPool)
					_taskPool->wakeUp(task);
				else
					fpnn::ClientEngine::wakeUpAnswerCallbackThreadPool(task);
			}
		}

		it++;
	}

	return taken;
}

void MultipleURLEngine::Executor::checkConnectionState()
{
	while (true)
	{
		int msgs_left;
		CURLMsg *msg = curl_multi_info_read(_multi, &msgs_left);
		if (!msg)
			return;

		if (msg->msg == CURLMSG_DONE)
		{
			CURL *easy = msg->easy_handle;
			BaiscResultCallbackPtr callback = _infoMap[easy];

			callback->completed(msg->data.result);
			curl_multi_remove_handle(_multi, easy);
			_load--;

			_infoMap.erase(easy);

			if (callback->syncedCallback())
			{
				SyncedResultCallback* sync = (SyncedResultCallback *)callback.get();
				sync->resultFilled();
			}
			else
			{
				if (_taskPool)
					_taskPool->wakeUp(callback);
				else
					fpnn::ClientEngine::wakeUpAnswerCallbackThreadPool(callback);
			}			
		}
	}
}

void MultipleURLEngine::Executor::checkTimeoutedQueuedSyncTask()
{
	int64_t now = slack_real_msec();

	std::unique_lock<std::mutex> lck(_mutex);
	for (auto it = _queue.begin(); it != _queue.end(); )
	{
		BaiscResultCallbackPtr &callback = *it;
		if (callback->_expireSeconds == 0)
		{
			it++;
			continue;
		}

		if (now - callback->_startTime > callback->_expireSeconds * 1000)
		{
			if (callback->syncedCallback())
			{
				SyncedResultCallback *sync = (SyncedResultCallback *)(callback.get());
				sync->terminated();
				sync->resultFilled();

				it = _queue.erase(it);
				_queueSize--;
				continue;
			}
			else
				it++;
		}
		else
			return;	
	}
}

void MultipleURLEngine::Executor::cleanTaskQueue()
{
	for (auto& callback: _queue)
	{
		callback->terminated();

		if (callback->syncedCallback())
		{
			SyncedResultCallback* sync = (SyncedResultCallback *)callback.get();
			sync->resultFilled();
		}
		else
		{
			if (_taskPool)
				_taskPool->wakeUp(callback);
			else
				fpnn::ClientEngine::wakeUpAnswerCallbackThreadPool(callback);
		}
	}
}

//========================================//
//- Functions of MultipleURLEngine
//========================================//
void MultipleURLEngine::rangeAdjust(int &value, int minValue, int maxValue)
{
	if (value > maxValue)
		value = maxValue;

	if (value < minValue)
		value = minValue;
}

void MultipleURLEngine::ceilingAdjust(int &targetValue, int comparedValue)
{
	if (targetValue < comparedValue)
		targetValue = comparedValue;
}
void MultipleURLEngine::floorAdjust(int &targetValue, int comparedValue)
{
	if (targetValue > comparedValue)
		targetValue = comparedValue;
}

MultipleURLEngine::MultipleURLEngine(int nConnsInPerThread, int initThreadCount, int perfectThreadCount,
			int maxThreadCount, int maxConcurrentCount, int tempThreadLatencySeconds,
			std::shared_ptr<TaskThreadPool> callbackPool):
			_nConnsInPerThread(nConnsInPerThread), _perfectThreadCount(perfectThreadCount),
			_maxThreadCount(maxThreadCount), _tempThreadLatencySeconds(tempThreadLatencySeconds),
			_callbackPool(callbackPool), _running(true)
{
	ReviseDataRelation(initThreadCount);
	//-- Currently, inputted maxConcurrentCount will be ignored.
	maxConcurrentCount = _maxThreadCount * _nConnsInPerThread;

	int systemUsablePorts = getSystemAvailablePortsCount();
	systemUsablePorts -= 100;	//-- remain 100 port for server's clients or other using.
	if (systemUsablePorts > 0 && systemUsablePorts < maxConcurrentCount)
		_maxConcurrentCount = systemUsablePorts;
	else
		_maxConcurrentCount = maxConcurrentCount;
	
	/*
	int mayConcurrent = _maxThreadCount * _nConnsInPerThread;
	if (mayConcurrent - _maxConcurrentCount > _nConnsInPerThread)
	{
		_maxThreadCount = _maxConcurrentCount / _nConnsInPerThread;
		if (_maxConcurrentCount % _nConnsInPerThread)
			_maxConcurrentCount = _maxThreadCount * _nConnsInPerThread;

		floorAdjust(_perfectThreadCount, _maxThreadCount);
	}*/

	_executors.reserve(_maxThreadCount);

	TaskThreadPool *taskPoolAddr = NULL;
	if (_callbackPool)
		taskPoolAddr = _callbackPool.get();
	else
		_engine = fpnn::ClientEngine::instance();

	for (int i = 0; i < initThreadCount; i++)
	{
		ExecutorPtr executor = std::make_shared<Executor>(_nConnsInPerThread, taskPoolAddr);
		if (executor->launch())
			_executors.push_back(executor);
	}

	_reclaimer = std::thread(&MultipleURLEngine::reclaimThread, this);

	refenceCount++;
}

MultipleURLEngine::~MultipleURLEngine()
{
	_running = false;
	_reclaimer.join();

	_executors.clear();
	refenceCount--;

	if (refenceCount <= 0)
		cleanup();
}

void MultipleURLEngine::ReviseDataRelation(int &initThreadCount)
{
	rangeAdjust(_nConnsInPerThread, 1, 200);
	rangeAdjust(initThreadCount, 1, 1000);
	rangeAdjust(_perfectThreadCount, 1, 1000);
	rangeAdjust(_maxThreadCount, 1, 1000);

	ceilingAdjust(_maxThreadCount, initThreadCount);
	floorAdjust(_perfectThreadCount, _maxThreadCount);
}

void MultipleURLEngine::reclaimThread()
{
	int ticket = 0;
	while (_running)
	{
		usleep(200 * 1000);
		ticket += 1;

		if (ticket == 600)
		{
			ticket = 0;
			reclaimCheck();
		}

		if (ticket % 5 == 0)
			checkTimeoutedQueuedSyncTask();
	}
}

void MultipleURLEngine::reclaimCheck()
{
	std::set<ExecutorPtr> reclaimed;

	std::unique_lock<std::mutex> lck(_mutex);
	int count = (int)_executors.size(); 
	if (count)
	{
		int64_t threshold = slack_mono_sec() - _tempThreadLatencySeconds;
		for (int c = count - 1; c >= _perfectThreadCount; c--)
		{
			if (_executors[c]->activeTime() < threshold)
			{
				reclaimed.insert(_executors[c]);
				_executors.erase(_executors.begin() + c);
			}
		}
	}
}

void MultipleURLEngine::checkTimeoutedQueuedSyncTask()
{
	std::unique_lock<std::mutex> lck(_mutex);
	for (ExecutorPtr& executor : _executors)
		executor->checkTimeoutedQueuedSyncTask();
}

int MultipleURLEngine::getSystemAvailablePortsCount()
{
	char buf[32];
	FILE *fp = fopen("/proc/sys/net/ipv4/ip_local_port_range", "r");
	if (!fp)
		return 0;

	size_t n = fread(buf, 1, 30, fp);
	fclose(fp);

	if (n < 3)
		return 0;

	buf[n] = 0;
	std::vector<std::string> elems;
	StringUtil::split(buf, " \t\r\n", elems);
	if (elems.size() != 2)
		return 0;

	return atoi(elems[1].c_str()) - atoi(elems[0].c_str());
}

std::string MultipleURLEngine::status()
{
	std::string status("{\"configParams\":{");
	status.append("\"nConnsInPerThread\":").append(std::to_string(_nConnsInPerThread));
	status.append(", \"perfectThreadCount\":").append(std::to_string(_perfectThreadCount));
	status.append(", \"maxThreadCount\":").append(std::to_string(_maxThreadCount));
	status.append(", \"maxConcurrentCount\":").append(std::to_string(_maxConcurrentCount));
	status.append("}, \"currentStatus\":{");

	int64_t totalLoads = 0;
	size_t currentThreads = 0;
	size_t queuedTaskCount = 0;

	{
		std::unique_lock<std::mutex> lck(_mutex);
		currentThreads = _executors.size();
		for (size_t i = 0; i < currentThreads; i++)
		{
			totalLoads += _executors[i]->load();
			queuedTaskCount += _executors[i]->queueSize();
		}
	}

	status.append("\"threads\":").append(std::to_string(currentThreads));
	status.append(", \"totalLoads\":").append(std::to_string(totalLoads));
	status.append(", \"connections\":").append(std::to_string(concurrentConnectionCount));
	status.append(", \"queuedTaskCount\":").append(std::to_string(queuedTaskCount));
	status.append("}}");

	return status;
}

void  MultipleURLEngine::status(StatusInfo &statusInfo)
{
	statusInfo.configInfo.connsInPerThread = _nConnsInPerThread;
	statusInfo.configInfo.perfectThreadCount = _perfectThreadCount;
	statusInfo.configInfo.maxThreadCount = _maxThreadCount;
	statusInfo.configInfo.maxConcurrentCount = _maxConcurrentCount;

	std::unique_lock<std::mutex> lck(_mutex);

	statusInfo.currentInfo.threads = (int64_t)_executors.size();
	statusInfo.currentInfo.connections = concurrentConnectionCount;
	statusInfo.currentInfo.loads = 0;
	statusInfo.currentInfo.queuedTaskCount = 0;

	for (size_t i = 0; i < _executors.size(); i++)
	{
		statusInfo.currentInfo.loads += _executors[i]->load();
		statusInfo.currentInfo.queuedTaskCount += _executors[i]->queueSize();
	}
}

bool MultipleURLEngine::pushTask(BaiscResultCallbackPtr task)
{
	int count = 0;
	bool requireAdd = false;

	int64_t minLoad = -1;
	int minLoadIdx = -1;

	{
		std::unique_lock<std::mutex> lck(_mutex);
		count = (int)_executors.size();
		if (count)
		{
			for (int c = 0; c < count; c++)
			{
				int64_t currload = _executors[c]->load();
				if (currload < _nConnsInPerThread)
				{
					_executors[c]->pushTask(task);
					return true;
				}

				if ((minLoad < 0) || (currload < minLoad))
				{
					minLoad = currload;
					minLoadIdx = c;
				}
			}

			if ((minLoad >= _nConnsInPerThread) && (count < _maxThreadCount))
				requireAdd = true;
			else
			{
				_executors[minLoadIdx]->pushTask(task);
				return true;
			}
		}
		else
			requireAdd = true;
	}

	if (requireAdd)
	{
		TaskThreadPool *taskPoolAddr = NULL;
		if (_callbackPool)
			taskPoolAddr = _callbackPool.get();

		ExecutorPtr executor = std::make_shared<Executor>(_nConnsInPerThread, taskPoolAddr);
		if (executor->launch())
		{
			executor->pushTask(task);

			std::unique_lock<std::mutex> lck(_mutex);
			_executors.push_back(executor);
		}
		else
			return false;
	}

	return true;
}

bool MultipleURLEngine::pushTasks(std::vector<BaiscResultCallbackPtr> &tasks)
{
	int threadCount;
	int64_t delta;
	int reduction;
	int64_t taskcount = (int64_t)tasks.size();

	std::vector<int64_t> fillArray;

	{
		std::unique_lock<std::mutex> lck(_mutex);
		for (size_t i = 0; i < _executors.size(); i++)
		{
			int64_t available = (int64_t)_nConnsInPerThread - (int64_t)_executors[i]->load();
			fillArray.push_back(available);
			if (available > 0)
				taskcount -= available;
		}

		threadCount = (int)_executors.size();
		while (taskcount > 0 && threadCount < _maxThreadCount)
		{
			taskcount -= (int64_t)_nConnsInPerThread;
			fillArray.push_back(_nConnsInPerThread);
			threadCount++;
		}

		delta = taskcount / threadCount;
		int remain = taskcount % threadCount;
		if (remain > 0)
		{
			delta += 1;
			reduction = remain;
		}
		else if (remain < 0)
		{
			delta += 1;
			reduction = -remain;
		}
		else
			reduction = 0;
		
		//-- begin push
		for (size_t i = 0; i < _executors.size(); i++)
		{
			int count = delta;
			if (fillArray[i] > 0)
				count += fillArray[i];

			if (count > 0)
			{
				std::vector<BaiscResultCallbackPtr> subtasks;
				subtasks.reserve(count);
				for (int c = 0; c < count; c++)
				{
					if (tasks.empty())
						break;

					subtasks.push_back(*(tasks.begin()));
					tasks.erase(tasks.begin());
				}

				if (subtasks.size())
					_executors[i]->pushTasks(subtasks);

				if (tasks.empty())
					return true;
			}

			if (reduction > 0)
			{
				reduction -= 1;
				if (reduction == 0)
					delta -= 1;
			}
		}

		threadCount -= (int)_executors.size();
	}

	//-- append new executors.
	if (threadCount)
	{
		TaskThreadPool *taskPoolAddr = NULL;
		if (_callbackPool)
			taskPoolAddr = _callbackPool.get();

		std::vector<ExecutorPtr> appendExecutors;

		for (int i = 0; i < threadCount; i++)
		{
			ExecutorPtr executor = std::make_shared<Executor>(_nConnsInPerThread, taskPoolAddr);
			if (executor->launch())
			{
				int64_t count = _nConnsInPerThread + delta;
				std::vector<BaiscResultCallbackPtr> subtasks;
				subtasks.reserve(count);
				for (int c = 0; c < count; c++)
				{
					if (tasks.empty())
						break;

					subtasks.push_back(*(tasks.begin()));
					tasks.erase(tasks.begin());
				}

				if (subtasks.size())
					executor->pushTasks(subtasks);

				appendExecutors.push_back(executor);

				if (tasks.empty())
					break;

				if (reduction > 0)
				{
					reduction -= 1;
					if (reduction == 0)
						delta -= 1;
				}
			}
		}

		{
			std::unique_lock<std::mutex> lck(_mutex);
			_executors.insert(_executors.end(), appendExecutors.begin(), appendExecutors.end());
		}
	}
	
	//-- if some exectors launch failed, some tasks are remained.
	if (tasks.empty() == false)
	{
		std::unique_lock<std::mutex> lck(_mutex);
		size_t pushCount = tasks.size()/_executors.size();
		if (tasks.size() % _executors.size())
			pushCount += 1;

		for (int i = 0; i < (int)_executors.size(); i++)
		{
			std::vector<BaiscResultCallbackPtr> subtasks;
			subtasks.reserve(pushCount);
			for (size_t c = 0; c < pushCount; c++)
			{
				if (tasks.empty())
					return true;

				subtasks.push_back(*(tasks.begin()));
				tasks.erase(tasks.begin());
			}

			if (subtasks.size())
				_executors[i]->pushTasks(subtasks);
		}
	}
	return true;
}

bool MultipleURLEngine::visit(const std::string& url, Result &result, int timeoutSeconds,
	bool saveResponseData, const std::string& postBody, const std::vector<std::string>& header)
{
	CURL *easy = curl_easy_init();
	if (!easy)
		return false;

	if (curl_easy_setopt(easy, CURLOPT_URL, url.c_str()) != CURLE_OK)
	{
		curl_easy_cleanup(easy);
		return false;
	}

	if (!visit(easy, result, timeoutSeconds, saveResponseData, postBody, header))
	{
		curl_easy_cleanup(easy);
		return false;
	}

	return true;
}

bool MultipleURLEngine::visit(CURL *curl, Result &result, int timeoutSeconds, bool saveResponseData,
		const std::string& postBody, const std::vector<std::string>& header)
{
	std::shared_ptr<SyncedResultCallback> callback = std::make_shared<SyncedResultCallback>(&result);	
	if (real_visit(curl, callback, timeoutSeconds, saveResponseData, header, postBody))
	{
		callback->waitResult();
		return true;
	}
	else
		return false;
}

bool MultipleURLEngine::visit(const std::string& url, ResultCallbackPtr callback, int timeoutSeconds,
	bool saveResponseData, const std::string& postBody, const std::vector<std::string>& header)
{
	CURL *easy = curl_easy_init();
	if (!easy)
		return false;

	if (curl_easy_setopt(easy, CURLOPT_URL, url.c_str()) != CURLE_OK)
	{
		curl_easy_cleanup(easy);
		return false;
	}

	if (!visit(easy, callback, timeoutSeconds, saveResponseData, postBody, header))
	{
		callback->_handle = NULL;
		curl_easy_cleanup(easy);
		return false;
	}

	return true;
}

bool MultipleURLEngine::visit(const std::string& url, std::function<void (Result &result)> callback,
	int timeoutSeconds, bool saveResponseData, const std::string& postBody, const std::vector<std::string>& header)
{
	CURL *easy = curl_easy_init();
	if (!easy)
		return false;

	if (curl_easy_setopt(easy, CURLOPT_URL, url.c_str()) != CURLE_OK)
	{
		curl_easy_cleanup(easy);
		return false;
	}

	std::shared_ptr<FunctionResultCallback>  fcb(new FunctionResultCallback(std::move(callback)));
	if (!real_visit(easy, fcb, timeoutSeconds, saveResponseData, header, postBody))
	{
		fcb->_handle = NULL;
		curl_easy_cleanup(easy);
		return false;
	}

	return true;
}

bool MultipleURLEngine::addToBatch(const std::string& url, ResultCallbackPtr callback, int timeoutSeconds,
	bool saveResponseData, const std::string& postBody, const std::vector<std::string>& header)
{
	CURL *easy = curl_easy_init();
	if (!easy)
		return false;

	if (curl_easy_setopt(easy, CURLOPT_URL, url.c_str()) != CURLE_OK)
	{
		curl_easy_cleanup(easy);
		return false;
	}

	if (!real_addToBatch(easy, callback, timeoutSeconds, saveResponseData, header, postBody))
	{
		callback->_handle = NULL;
		curl_easy_cleanup(easy);
		return false;
	}

	return true;
}

bool MultipleURLEngine::addToBatch(const std::string& url, std::function<void (Result &result)> callback,
	int timeoutSeconds, bool saveResponseData, const std::string& postBody, const std::vector<std::string>& header)
{
	CURL *easy = curl_easy_init();
	if (!easy)
		return false;

	if (curl_easy_setopt(easy, CURLOPT_URL, url.c_str()) != CURLE_OK)
	{
		curl_easy_cleanup(easy);
		return false;
	}

	std::shared_ptr<FunctionResultCallback>  fcb(new FunctionResultCallback(std::move(callback)));
	if (!real_addToBatch(easy, fcb, timeoutSeconds, saveResponseData, header, postBody))
	{
		fcb->_handle = NULL;
		curl_easy_cleanup(easy);
		return false;
	}

	return true;
}

bool MultipleURLEngine::prepareEasyHandle(CURL *curl, BaiscResultCallbackPtr callback, int timeoutSeconds,
		bool saveResponseData, const std::vector<std::string>& headers, const std::string& postBody)
{
	if (timeoutSeconds >= 0)
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeoutSeconds);

	if (headers.size() > 0)
	{
		for (auto& header: headers)
		{
			callback->_headerChunk = curl_slist_append(callback->_headerChunk, header.c_str());
		}

		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, callback->_headerChunk);
	}

	if (saveResponseData)
	{
		ChainBuffer *buf = new ChainBuffer();
		callback->_dataBuffer = buf;
		if (curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, Executor::writeCallback) != CURLE_OK)
			return false;
		if (curl_easy_setopt(curl, CURLOPT_WRITEDATA, buf) != CURLE_OK)
			return false;
	}

	if (postBody.length())
	{
		callback->_postBody = postBody;
		if (curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)callback->_postBody.length()) != CURLE_OK)
			return false;
		if (curl_easy_setopt(curl, CURLOPT_POSTFIELDS, callback->_postBody.data()) != CURLE_OK)
			return false;
	}

	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);

	callback->_handle = curl;
	callback->_expireSeconds = timeoutSeconds;
	
	return true;
}

bool MultipleURLEngine::real_visit(CURL *curl, BaiscResultCallbackPtr callback, int timeoutSeconds,
		bool saveResponseData, const std::vector<std::string>& headers, const std::string& postBody)
{
	if (!prepareEasyHandle(curl, callback, timeoutSeconds, saveResponseData, headers, postBody))
		return false;
	
	return pushTask(callback);
}

bool MultipleURLEngine::real_addToBatch(CURL *curl, BaiscResultCallbackPtr callback, int timeoutSeconds,
		bool saveResponseData, const std::vector<std::string>& headers, const std::string& postBody)
{
	if (!prepareEasyHandle(curl, callback, timeoutSeconds, saveResponseData, headers, postBody))
		return false;

	if (_localTaskCache == nullptr)
		_localTaskCache.reset(new std::vector<BaiscResultCallbackPtr>());

	_localTaskCache->push_back(callback);
	return true;
}

bool MultipleURLEngine::commitBatch()
{
	if (_localTaskCache == nullptr)
		return true;

	//-- current pushTasks() return true in all case.
	pushTasks(*_localTaskCache);
	_localTaskCache->clear();

	return true;
}
