#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <atomic>
#include <set>
#include "FPLog.h"
#include "NetworkUtility.h"
//#include "ServerController.h"
#include "TCPClient.h"
#include "UDPClient.h"
#include "ClientEngine.h"
#include "Setting.h"
#include "msec.h"
#include "Config.h"

//-- For unused the returned values of pipe() & write() in ClientEngine::stop().
#pragma GCC diagnostic ignored "-Wunused-result"

using namespace fpnn;

std::mutex ClientEngine::_mutex;
static std::atomic<bool> _created(false);
static ClientEnginePtr _engine;

uint16_t ClientEngine::_THREAD_MIN = 2;
uint16_t ClientEngine::_THREAD_MAX = 256;

ClientEngine* ClientEngine::nakedInstance()
{
	if (!_created)
	{
		std::unique_lock<std::mutex> lck(_mutex);
		if (!_created)
		{
			_engine.reset(new ClientEngine);
			_created = true;
		}
	}
	return _engine.get();
}
ClientEnginePtr ClientEngine::instance()
{
	if (!_created)
	{
		std::unique_lock<std::mutex> lck(_mutex);
		if (!_created)
		{
			_engine.reset(new ClientEngine);
			_created = true;
		}
	}
	return _engine;
}

bool ClientEngine::created(){
	return _created;
}

ClientEngine::ClientEngine(): _kqueue_fd(0), _running(false), _started(false), _max_events(1000),
	_kqueueEvents(NULL), _ioBufferChunkSize(256), _timeoutQuest(FPNN_DEFAULT_QUEST_TIMEOUT * 1000)
{
	_cpuCount = (uint16_t)sysconf(_SC_NPROCESSORS_ONLN);
	if (_cpuCount < 2)
		_cpuCount = 2;

	_workThreadMin = _cpuCount;
	_workThreadMax = _cpuCount;
	_ioThreadMin = _cpuCount;
	_ioThreadMax = _cpuCount;

	_duplexThreadMin = 0;
	_duplexThreadMax = 0;

	_questProcessPoolMaxQueueLength = 10 * 10000;

	int workArraySize = Setting::getInt("FPNN.client.work.thread.min.size", _cpuCount);
	if (workArraySize <= 0 || _cpuCount < workArraySize)
		workArraySize = _cpuCount;

	int duplexArraySize = Setting::getInt("FPNN.client.duplex.thread.min.size", _cpuCount);
	if (duplexArraySize <= 0 || _cpuCount < duplexArraySize)
		duplexArraySize = _cpuCount;

	_answerCallbackPool.config(workArraySize);
	_questProcessPool.config(duplexArraySize);

	initClientVaribles();

	_closeNotifyFds[0] = 0;
	_closeNotifyFds[1] = 0;
	_reclaimer = ConnectionReclaimer::instance();
}

bool ClientEngine::initClientVaribles(){
	Config::initClientVaribles();

	_timeoutQuest= Setting::getInt("FPNN.client.quest.timeout", FPNN_DEFAULT_QUEST_TIMEOUT) * 1000;

	_ioBufferChunkSize = Setting::getInt("FPNN.client.iobuffer.chunk.size", 256);

	_max_events = Setting::getInt("FPNN.client.max.event", 1000);

	_questProcessPoolMaxQueueLength = Setting::getInt("FPNN.client.work.queue.max.size", 10 * 10000);
	
	_workThreadMin = Setting::getInt("FPNN.client.work.thread.min.size", _cpuCount);
	_workThreadMax = Setting::getInt("FPNN.client.work.thread.max.size", _cpuCount);

	_ioThreadMin = Setting::getInt("FPNN.global.io.thread.min.size", _cpuCount);
	_ioThreadMax = Setting::getInt("FPNN.global.io.thread.max.size", _cpuCount);

	_duplexThreadMin = Setting::getInt("FPNN.client.duplex.thread.min.size", 0);
	_duplexThreadMax = Setting::getInt("FPNN.client.duplex.thread.max.size", 0);

	if(_workThreadMin == 0) _workThreadMin = _THREAD_MIN;
    else if(_workThreadMin > _THREAD_MAX) _workThreadMin = _THREAD_MAX;

    if(_workThreadMax == 0) _workThreadMax = _THREAD_MIN;
    else if(_workThreadMax > _THREAD_MAX) _workThreadMax = _THREAD_MAX;

    if(_ioThreadMin == 0) _ioThreadMin = _THREAD_MIN;
    else if(_ioThreadMin > _THREAD_MAX) _ioThreadMin = _THREAD_MAX;

    if(_ioThreadMax == 0) _ioThreadMax = _THREAD_MIN;
    else if(_ioThreadMax > _THREAD_MAX) _ioThreadMax = _THREAD_MAX;

    if(_workThreadMin > _workThreadMax) _workThreadMin = _workThreadMax;
    if(_ioThreadMin > _ioThreadMax) _ioThreadMin = _ioThreadMax;
	if(_duplexThreadMin > _duplexThreadMax) _duplexThreadMin = _duplexThreadMax;

	return true;
}

int getCPUCount()
{
	int cpuCount = (int)sysconf(_SC_NPROCESSORS_ONLN);
	return (cpuCount < 2) ? 2 : cpuCount;
}

void ClientEngine::prepare()
{
	const size_t minIOBufferChunkSize = 64;
	if (_ioBufferChunkSize < minIOBufferChunkSize)
		_ioBufferChunkSize = minIOBufferChunkSize;
	
	if (!_connectionMap.inited())
		setEstimateMaxConnections(128);

	if (!_answerCallbackPool.inited())
		configAnswerCallbackThreadPool(_workThreadMin, 1, _workThreadMin, _workThreadMax);

	_tcpIOWorker.reset(new TCPClientIOWorker);
	_udpIOWorker.reset(new UDPClientIOWorker);
	
	if (_ioPool == nullptr)
	{
		_ioPool = GlobalIOPool::instance();
		_ioPool->setClientIOWorker(_tcpIOWorker, _udpIOWorker);
		/** 
			Just ensure io Pool is inited. If is inited, it will not reconfig or reinit. */
		_ioPool->init(_ioThreadMin, 1, _ioThreadMin, _ioThreadMax);
	}

	if (!_questProcessPool.inited())
	{
		if (_duplexThreadMin > 0 && _duplexThreadMax > 0)
			configQuestProcessThreadPool(_duplexThreadMin, 1, _duplexThreadMin, _duplexThreadMax, _questProcessPoolMaxQueueLength);
		else
		{
			int cpuCount = getCPUCount();
			configQuestProcessThreadPool(0, 1, cpuCount, cpuCount, _questProcessPoolMaxQueueLength);
		}
	}
}

bool ClientEngine::init()
{
	_kqueue_fd = kqueue();
	if (_kqueue_fd == -1)
	{
		_kqueue_fd = 0;
		LOG_FATAL("Create kqueue failed.");
		return false;
	}

	_kqueueEvents = new(std::nothrow) struct kevent[_max_events];
	if (_kqueueEvents == NULL)
	{
		close(_kqueue_fd);
		_kqueue_fd = 0;
		LOG_FATAL("Create events matrix failed.");
		return false;
	}

	_running = true;
	return true;
}

bool ClientEngine::start()
{
	if (_running)
		return true;

	std::unique_lock<std::mutex> lck(_mutex);
	if (_running)
		return true;

	if (_started)
		return false;

	prepare();
	if (init())
	{
		_loopThread = std::thread(&ClientEngine::loopThread, this);
		_timeoutChecker = std::thread(&ClientEngine::timeoutCheckThread, this);
		//ServerController::startTimeoutCheckThread();
		_started = true;
		return true;
	}
	return false;
}

void ClientEngine::stop()
{
	if (!_running)
		return;
	
	_running = false;

	{
		pipe(_closeNotifyFds);

		nonblockedFd(_closeNotifyFds[0]);

		struct kevent ev;
		EV_SET(&ev, _closeNotifyFds[0], EVFILT_READ | EVFILT_WRITE, EV_ADD | EV_CLEAR, 0, 0, NULL);
		kevent(_kqueue_fd, &ev, 1, NULL, 0, NULL);

		write(_closeNotifyFds[1], this, 4);
	}

	LOG_INFO("Client engine is stopping ...");

	_timeoutChecker.join();
	//ServerController::joinTimeoutCheckThread();
	_loopThread.join();

	//-- Thread pool is runnng. It will be free when server destroy.
	//-- or need to release at this point?
}

void ClientEngine::sendCloseEvent()
{
	std::set<int> fdSet;
	_connectionMap.getAllSocket(fdSet);
	
	for (int socket: fdSet)
	{
		struct kevent	ev;
		ev.ident = socket;
		ev.flags = EV_EOF;

		processEvent(ev);
	}
}

void ClientEngine::clean()
{
	close(_closeNotifyFds[1]);
	close(_closeNotifyFds[0]);
	_closeNotifyFds[0] = 0;
	_closeNotifyFds[1] = 0;
	
	sendCloseEvent();

	_connectionMap.waitForEmpty();
	_answerCallbackPool.release();
	_questProcessPool.release();

	close(_kqueue_fd);
	_kqueue_fd = 0;

	if (_kqueueEvents)
	{
		delete [] _kqueueEvents;
		_kqueueEvents = NULL;
	}

	LOG_INFO("Client engine stopped.");
}

void ClientEngine::loopThread()
{
	while (true)
	{
		int readyfdsCount = kevent(_kqueue_fd, NULL, 0, _kqueueEvents, _max_events, NULL);
		if (!_running)
		{
			clean();
			return;
		}

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
			processEvent(_kqueueEvents[i]);
		}
	}
}

void ClientEngine::clearConnectionQuestCallbacks(BasicConnection* connection, int errorCode)
{
	for (auto callbackPair: connection->_callbackMap)
	{
		BasicAnswerCallback* callback = callbackPair.second;
		if (callback->syncedCallback())		//-- check first, then fill result.
			callback->fillResult(NULL, errorCode);
		else
		{
			callback->fillResult(NULL, errorCode);

			BasicAnswerCallbackPtr task(callback);
			_answerCallbackPool.wakeUp(task);
		}
	}
	// connection->_callbackMap.clear(); //-- If necessary.
}

void ClientEngine::processEvent(struct kevent & event)
{
	int socket = event.ident;

	//-- Please care the order.
	if (event.flags & EV_EOF)
	{
		//-- close event processor
		BasicConnection* orgConn = takeConnection(socket);
		if (orgConn == NULL)
			return;

		exitEpoll(orgConn);
		clearConnectionQuestCallbacks(orgConn, FPNN_EC_CORE_CONNECTION_CLOSED);

		if (orgConn->connectionType() != BasicConnection::UDPClientConnectionType)
		{
			TCPClientConnection* conn = (TCPClientConnection*)orgConn;
			TCPClientPtr client = conn->client();
			if (client)
			{
				client->willClose(conn);
				return;
			}
		}
		else
		{
			UDPClientConnection* conn = (UDPClientConnection*)orgConn;
			UDPClientPtr client = conn->client();
			if (client)
			{
				client->willClose(conn);
				return;
			}
			else
			{
				IQuestProcessorPtr processor = conn->questProcessor();
				if (processor)
				{
					std::shared_ptr<ClientCloseTask> task(new ClientCloseTask(processor, conn, false));
					_answerCallbackPool.wakeUp(task);
					_reclaimer->reclaim(task);
				}
				else
				{
					ConnectionReclaimTaskPtr task(new ConnectionReclaimTask(conn));
					_reclaimer->reclaim(task);
				}
				return;
			}
		}

		LOG_ERROR("This codes (Engine::close) is impossible touched. This is just a safety inspection. If this ERROR triggered, please tell swxlion to add old CloseErrorTask class back, and fix it.");
		ConnectionReclaimTaskPtr task(new ConnectionReclaimTask(orgConn));
		//CloseErrorTaskPtr task(new CloseErrorTask(orgConn, false));
		//_answerCallbackPool.wakeUp(task);
		_reclaimer->reclaim(task);
		return;
	}
	else if (event.flags & EV_ERROR)
	{
		//-- error event processor
		BasicConnection* orgConn = takeConnection(socket);
		if (orgConn == NULL)
			return;

		exitEpoll(orgConn);
		clearConnectionQuestCallbacks(orgConn, FPNN_EC_CORE_UNKNOWN_ERROR);

		if (orgConn->connectionType() != BasicConnection::UDPClientConnectionType)
		{
			TCPClientConnection* conn = (TCPClientConnection*)orgConn;
			TCPClientPtr client = conn->client();
			if (client)
			{
				client->errorAndWillBeClosed(conn);
				return;
			}
		}
		else
		{
			UDPClientConnection* conn = (UDPClientConnection*)orgConn;
			UDPClientPtr client = conn->client();
			if (client)
			{
				client->errorAndWillBeClosed(conn);
				return;
			}
			else
			{
				IQuestProcessorPtr processor = conn->questProcessor();
				if (processor)
				{
					std::shared_ptr<ClientCloseTask> task(new ClientCloseTask(processor, conn, true));
					_answerCallbackPool.wakeUp(task);
					_reclaimer->reclaim(task);
				}
				else
				{
					ConnectionReclaimTaskPtr task(new ConnectionReclaimTask(conn));
					_reclaimer->reclaim(task);
				}
				return;
			}
		}

		LOG_ERROR("This codes (Engine::error) is impossible touched. This is just a safety inspection. If this ERROR triggered, please tell swxlion to add old CloseErrorTask class back, and fix it.");
		ConnectionReclaimTaskPtr task(new ConnectionReclaimTask(orgConn));
		//CloseErrorTaskPtr task(new CloseErrorTask(orgConn, true));
		//_answerCallbackPool.wakeUp(task);
		_reclaimer->reclaim(task);
		return;
	}

	BasicConnection* conn = _connectionMap.signConnection(socket, event.filter);
	if (conn)
		_ioPool->wakeUp(conn);
}

void ClientEngine::sendData(int socket, uint64_t token, std::string* data)
{
	if (!_connectionMap.sendData(socket, token, data))
	{
		delete data;
		LOG_WARN("Data not send at socket %d, address: %s. socket maybe closed.", socket, NetworkUtil::getPeerName(socket).c_str());
	}
}

void ClientEngine::sendUDPData(int socket, uint64_t token, std::string* data, int64_t expiredMS, bool discardable)
{
	if (expiredMS == 0)
		expiredMS = slack_real_msec() + _timeoutQuest;

	if (!_connectionMap.sendUDPData(socket, token, data, expiredMS, discardable))
	{
		delete data;
		LOG_WARN("UDP Data not send at socket %d, address: %s. socket maybe closed.", socket, NetworkUtil::getPeerName(socket).c_str());
	}
}

bool ClientEngine::waitForRecvEvent(const BasicConnection* connection)
{
	return waitForEvents(EVFILT_READ, connection);
}
bool ClientEngine::waitForAllEvents(const BasicConnection* connection)
{
	return waitForEvents(EVFILT_READ | EVFILT_WRITE, connection);
}

bool ClientEngine::joinEpoll(BasicConnection* connection)
{
	if (_running == false)
		if (!startEngine())
			return false;

	int socket = connection->socket();
	if (!nonblockedFd(socket))
	{
		LOG_ERROR("Change socket to non-blocked failed. %s", connection->_connectionInfo->str().c_str());
		return false;
	}

	_connectionMap.insert(socket, connection);

	int16_t filter = EVFILT_READ;
	if (connection->_connectionInfo->isSSL())
		filter |= EVFILT_WRITE;

	struct kevent ev;
	EV_SET(&ev, socket, filter, EV_ADD | EV_CLEAR | EV_ONESHOT, 0, 0, NULL);

	if (kevent(_kqueue_fd, &ev, 1, NULL, 0, NULL) == -1)
	{
		_connectionMap.remove(socket);
		return false;
	}
	else
		return true;
}

bool ClientEngine::waitForEvents(int16_t filter, const BasicConnection* connection)
{
	struct kevent ev;
	EV_SET(&ev, connection->socket(), filter, EV_ADD | EV_CLEAR | EV_ONESHOT, 0, 0, NULL);

	if (kevent(_kqueue_fd, &ev, 1, NULL, 0, NULL) == -1)
	{
		if (errno == ENOENT)
			return false;
		
		LOG_INFO("Client engine wait socket event failed. Socket: %d, address: %s, errno: %d", connection->socket(),NetworkUtil::getPeerName(connection->socket()).c_str(), errno);
		return false;
	}
	else
		return true;
}

void ClientEngine::exitEpoll(const BasicConnection* connection)
{
	struct kevent ev;
	EV_SET(&ev, connection->socket(), EVFILT_READ | EVFILT_WRITE, EV_DELETE, 0, 0, NULL);

	kevent(_kqueue_fd, &ev, 1, NULL, 0, NULL);
}

void ClientEngine::closeUDPConnection(UDPClientConnection* connection)
{
	exitEpoll(connection);

	UDPClientPtr client = connection->client();
	if (client)
	{
		client->clearConnectionQuestCallbacks(connection, FPNN_EC_CORE_CONNECTION_CLOSED);
		client->willClose(connection);
	}
	else
	{
		clearConnectionQuestCallbacks(connection, FPNN_EC_CORE_CONNECTION_CLOSED);
		
		IQuestProcessorPtr processor = connection->questProcessor();
		if (processor)
		{
			std::shared_ptr<ClientCloseTask> task(new ClientCloseTask(processor, connection, false));
			wakeUpAnswerCallbackThreadPool(task);
			ConnectionReclaimer::nakedInstance()->reclaim(task);
		}
		else
		{
			ConnectionReclaimTaskPtr task(new ConnectionReclaimTask(connection));
			ConnectionReclaimer::nakedInstance()->reclaim(task);
		}
	}
}

void ClientEngine::timeoutCheckThread()
{
	while (_running)
	{
		//-- Step 1: UDP period sending check

		int cyc = 100;
		int udpSendingCheckSyc = 5;
		while (_running && cyc--)
		{
			udpSendingCheckSyc -= 1;
			if (udpSendingCheckSyc == 0)
			{
				udpSendingCheckSyc = 5;
				std::unordered_set<UDPClientConnection*> invalidOrExpiredConnections;
				_connectionMap.periodUDPSendingCheck(invalidOrExpiredConnections);

				for (UDPClientConnection* conn: invalidOrExpiredConnections)
					closeUDPConnection(conn);
			}

			usleep(10000);
		}


		//-- Step 2: TCP client keep alive

		std::list<TCPClientConnection*> invalidConnections;

		_connectionMap.TCPClientKeepAlive(invalidConnections);
		for (auto conn: invalidConnections)
		{
			exitEpoll(conn);
			clearConnectionQuestCallbacks(conn, FPNN_EC_CORE_INVALID_CONNECTION);

			TCPClientPtr client = conn->client();
			if (client)
			{
				client->errorAndWillBeClosed(conn);
			}
			else
			{
				LOG_ERROR("This codes (Engine::error) is impossible touched. This is just a safety inspection. If this ERROR triggered, please tell swxlion to add old CloseErrorTask class back, and fix it.");
				ConnectionReclaimTaskPtr task(new ConnectionReclaimTask(conn));
				//CloseErrorTaskPtr task(new CloseErrorTask(orgConn, true));
				//_answerCallbackPool.wakeUp(task);
				_reclaimer->reclaim(task);
			}
		}

		//-- Step 3: clean timeouted callbacks

		int64_t current = slack_real_msec();
		std::list<std::map<uint32_t, BasicAnswerCallback*> > timeouted;

		_connectionMap.extractTimeoutedCallback(current, timeouted);
		for (auto bacMap: timeouted)
		{
			for (auto bacPair: bacMap)
			{
				if (bacPair.second)
				{
					BasicAnswerCallback* callback = bacPair.second;
					if (callback->syncedCallback())		//-- check first, then fill result.
						callback->fillResult(NULL, FPNN_EC_CORE_TIMEOUT);
					else
					{
						callback->fillResult(NULL, FPNN_EC_CORE_TIMEOUT);

						BasicAnswerCallbackPtr task(callback);
						_answerCallbackPool.wakeUp(task);
					}
				}
			}
		}
	}
}

void ClientCloseTask::run()
{
	_executed = true;

	if (_questProcessor)
	try
	{
		_questProcessor->connectionWillClose(*(_connection->_connectionInfo), _error);
	}
	catch (const FpnnError& ex){
		LOG_ERROR("ClientCloseTask::run() error:(%d)%s. %s", ex.code(), ex.what(), _connection->_connectionInfo->str().c_str());
	}
	catch (...)
	{
		LOG_ERROR("Unknown error when calling ClientCloseTask::run() function. %s", _connection->_connectionInfo->str().c_str());
	}
}
