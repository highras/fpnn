#include <string.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/sysinfo.h>
#include <errno.h>
#include "FpnnError.h"
#include "FPLog.h"
#include "msec.h"
#include "Setting.h"
#include "HostLookup.h"
#include "NetworkUtility.h"
#include "UDPEpollServer.h"

using namespace fpnn;

UDPServerPtr UDPEpollServer::_server = nullptr;

static void adjustThreadPoolParams(int &minThread, int &maxThread, int constMin, int constMax)
{
	if (minThread < constMin)
		minThread = constMin;
	else if (minThread > constMax)
		minThread = constMax;

	if (maxThread < constMin)
		maxThread = constMin;
	else if (maxThread > constMax)
		maxThread = constMax;

	if (minThread > maxThread)
		minThread = maxThread;
}

int UDPEpollServer::getCPUCount()
{
	int cpuCount = get_nprocs();
	return (cpuCount < 2) ? 2 : cpuCount;
}

bool UDPEpollServer::initServerVaribles()
{
	Config::initServerVaribles();

	int port = Setting::getInt(std::vector<std::string>{
		"FPNN.server.udp.ipv4.listening.port",
		"FPNN.server.ipv4.listening.port",
		"FPNN.server.listening.port"}, 0);
	if (port)
	{
		if(port <= 1024 || port > 65535){
			throw FPNN_ERROR_CODE_FMT(FpnnCoreError, FPNN_EC_CORE_UNKNOWN_ERROR, "Invalide UDP port(%d), should between(1024~~65535)", port);
		}
		_port = port;

		_ip = Setting::getString(std::vector<std::string>{
			"FPNN.server.udp.ipv4.listening.ip",
			"FPNN.server.ipv4.listening.ip",
			"FPNN.server.listening.ip",
			}, "");
		if(_ip == "*") _ip = "";//listen all ip
	}

	int port6 = Setting::getInt(std::vector<std::string>{
		"FPNN.server.udp.ipv6.listening.port",
		"FPNN.server.ipv6.listening.port",
		}, 0);
	if (port6)
	{
		if(port6 <= 1024 || port6 > 65535){
			throw FPNN_ERROR_CODE_FMT(FpnnCoreError, FPNN_EC_CORE_UNKNOWN_ERROR, "Invalide UDP port(%d) for IPv6, should between(1024~~65535)", port6);
		}
		_port6 = port6;

		_ipv6 = Setting::getString(std::vector<std::string>{
			"FPNN.server.udp.ipv6.listening.ip",
			"FPNN.server.ipv6.listening.ip"
			}, "");
		if(_ipv6 == "*") _ipv6 = "";//listen all ip
	}

	if (_port == 0 && _port6 == 0)
		throw FPNN_ERROR_CODE_MSG(FpnnCoreError, FPNN_EC_CORE_UNKNOWN_ERROR, "Invalide UDP port. IPv4 & IPv6 port are all unconfigurated.");

	_timeoutQuest = Setting::getInt(std::vector<std::string>{
		"FPNN.server.udp.quest.timeout",
		"FPNN.server.quest.timeout"}, FPNN_DEFAULT_QUEST_TIMEOUT) * 1000;
	_maxWorkerPoolQueueLength = Setting::getInt(std::vector<std::string>{
		"FPNN.server.udp.work.queue.max.size",
		"FPNN.server.work.queue.max.size"}, FPNN_DEFAULT_UDP_WORK_POOL_QUEUE_SIZE);

	_heldLogger = FPLog::instance();

	return true;
}

bool UDPEpollServer::prepare()
{
	if (_serverMasterProcessor->checkQuestProcessor() == false)
	{
		LOG_FATAL("Invalide Quest Processor for UDP server.");
		return false;
	}

	_serverMasterProcessor->setServer(this);

	if (_workerPool == nullptr)
	{
		int cpuCount = getCPUCount();
		int workThreadMin = Setting::getInt(std::vector<std::string>{
			"FPNN.server.udp.work.thread.min.size", "FPNN.server.work.thread.min.size"}, cpuCount);
		int workThreadMax = Setting::getInt(std::vector<std::string>{
			"FPNN.server.udp.work.thread.max.size", "FPNN.server.work.thread.max.size"}, cpuCount);

		adjustThreadPoolParams(workThreadMin, workThreadMax, 2, 256);
		configWorkerThreadPool(workThreadMin, 1, workThreadMin, workThreadMax, _maxWorkerPoolQueueLength);
	}

	{
		int duplexThreadMin = Setting::getInt(std::vector<std::string>{
			"FPNN.server.udp.duplex.thread.min.size", "FPNN.server.duplex.thread.min.size"}, 0);
		int duplexThreadMax = Setting::getInt(std::vector<std::string>{
			"FPNN.server.udp.duplex.thread.max.size", "FPNN.server.duplex.thread.max.size"}, 0);

		if (_answerCallbackPool == nullptr && duplexThreadMin > 0 && duplexThreadMax > 0)
		{
			adjustThreadPoolParams(duplexThreadMin, duplexThreadMax, 1, 256);
			enableAnswerCallbackThreadPool(duplexThreadMin, 1, duplexThreadMin, duplexThreadMax);
		}
	}

	if (!_ip.empty())
		_ip = HostLookup::get(_ip);

	//if (!_ipv6.empty())
	//	_ipv6 = HostLookup::get(_ipv6);

	_callbackMap.init(128);

	return true;
}


bool UDPEpollServer::init()
{
	if (!initEpoll())
		return false;

	if (_port)
		if (!initIPv4())
			return false;

	if (_port6)
		if (!initIPv6())
			return false;

	return true;
}

bool UDPEpollServer::initIPv4()
{
	struct sockaddr_in serverAddr;

	memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET; 
	serverAddr.sin_port = htons(_port);

	if (_ip.empty())
		serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	else
		serverAddr.sin_addr.s_addr = inet_addr(_ip.c_str());
	
	if (serverAddr.sin_addr.s_addr == INADDR_NONE)
	{
		std::string errorMsg("Invalid UDP IPv4 address: ");
		errorMsg.append(_ip);
		initFailClean(errorMsg.c_str());
		return false;
	}

	_socket = socket(AF_INET, SOCK_DGRAM, 0);
	if (_socket == -1)
	{
		_socket = 0;
		initFailClean("Create UDP IPv4 socket failed.");
		return false;
	}

	int reuse_addr = 1;
	if (-1 == setsockopt(_socket, SOL_SOCKET, SO_REUSEADDR, (void*)&(reuse_addr), sizeof(reuse_addr)))
	{
		initFailClean("UDP IPv4 socket resuing failed.");
		return false;
	}

	if (!nonblockedFd(_socket))
	{
		initFailClean("Change UDP IPv4 socket to non-blocked failed.");
		return false;
	}

	struct epoll_event ev;
	ev.data.fd = _socket;
	ev.events = EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLET | EPOLLRDHUP;

	if (-1 == epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, _socket, &ev))
	{
		initFailClean("Add UDP IPv4 listening socket to epoll failed.");
		return false;
	}

	if (-1 == bind(_socket, (struct sockaddr *)(&serverAddr), sizeof(serverAddr))) 
	{
		initFailClean("Bind UDP IPv4 listening socket failed.");
		return false;
	}

	return true;
}

bool UDPEpollServer::initIPv6()
{
	struct sockaddr_in6 serverAddr;

	memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin6_family = AF_INET6; 
	serverAddr.sin6_port = htons(_port6);

	if (_ipv6.empty())
		serverAddr.sin6_addr = in6addr_any;
	else if (inet_pton(AF_INET6, _ipv6.c_str(), &serverAddr.sin6_addr) != 1)
	{
		std::string errorMsg("Invalid UDP IPv6 address: ");
		errorMsg.append(_ipv6);
		initFailClean(errorMsg.c_str());
		return false;
	}

	_socket6 = socket(AF_INET6, SOCK_DGRAM, 0);
	if (_socket6 == -1)
	{
		_socket6 = 0;
		initFailClean("Create UDP IPv6 socket failed.");
		return false;
	}

	int reuse_addr = 1;
	if (-1 == setsockopt(_socket6, SOL_SOCKET, SO_REUSEADDR, (void*)&(reuse_addr), sizeof(reuse_addr)))
	{
		initFailClean("UDP IPv6 socket resuing failed.");
		return false;
	}

	if (!nonblockedFd(_socket6))
	{
		initFailClean("Change UDP IPv6 socket to non-blocked failed.");
		return false;
	}

	struct epoll_event ev;
	ev.data.fd = _socket6;
	ev.events = EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLET | EPOLLRDHUP;

	if (-1 == epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, _socket6, &ev))
	{
		initFailClean("Add UDP IPv6 listening socket to epoll failed.");
		return false;
	}

	if (-1 == bind(_socket6, (struct sockaddr *)(&serverAddr), sizeof(serverAddr))) 
	{
		initFailClean("Bind UDP IPv6 listening socket failed.");
		return false;
	}

	return true;
}

bool UDPEpollServer::initEpoll()
{
	//-- Initialize epoll.
	const int epoll_size = 1000000; //-- Since Linux 2.6.8, the size argument is ignored, but must be greater than zero.
	_epoll_fd = epoll_create(epoll_size);
	if (_epoll_fd == -1)
	{
		_epoll_fd = 0;
		LOG_FATAL("Create epoll failed.");
		return false;
	}

	_epollEvents = new(std::nothrow) epoll_event[_max_events];
	if (_epollEvents == NULL)
	{
		initFailClean("Create epoll events matrix for UDP server failed.");
		return false;
	}

	return true;
}

void UDPEpollServer::initFailClean(const char* fail_info)
{
	if (_socket > 0)
		close(_socket);

	if (_socket6 > 0)
		close(_socket6);

	close(_epoll_fd);

	_socket = 0;
	_socket6 = 0;
	_epoll_fd = 0;

	if (_epollEvents)
	{
		delete [] _epollEvents;
		_epollEvents = NULL;
	}

	if (fail_info)
		LOG_FATAL(fail_info);
}

void UDPEpollServer::updateSocketStatus(int socket, bool needWaitSendEvent)
{
	struct epoll_event	ev;
	ev.data.fd = socket;
	ev.events = EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLET | EPOLLRDHUP;

	if (needWaitSendEvent)
		ev.events |= EPOLLOUT;
	
	if (epoll_ctl(_epoll_fd, EPOLL_CTL_MOD, socket, &ev) != 0)
		LOG_ERROR("Modify server listened UDP socket %d %s send event error.", socket, needWaitSendEvent ? "with" : "without");
}

void UDPEpollServer::run()
{
	_running = true;
	_stopping = false;
	_stopSignalNotified = false;

	UDPRecvBuffer recvBuffer;

	while (true)
	{
		int readyfdsCount = epoll_wait(_epoll_fd, _epollEvents, _max_events, -1);
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
				LOG_ERROR("Invalid epoll fd.");
			}
			else
			{
				LOG_ERROR("Unknown Error when epoll_wait() errno: %d", errno);
			}

			clean();
			return;
		}

		for (int i = 0; i < readyfdsCount; i++)
			processEvent(_epollEvents[i], recvBuffer);
	}
}

void UDPEpollServer::processEvent(struct epoll_event & event, UDPRecvBuffer &recvBuffer)
{
	int socket = event.data.fd;
	bool ipv4Socket = (socket == _socket);

	if (event.events & EPOLLRDHUP || event.events & EPOLLHUP)
		LOG_FATAL("UDP %s socket recv EPOLLRDHUP or EPOLLHUP event.", ipv4Socket ? "IPv4" : "IPv6");
	
	if (event.events & EPOLLERR)
		LOG_FATAL("UDP %s socket recv EPOLLERR event.", ipv4Socket ? "IPv4" : "IPv6");

	//-- send is first than receive.
	if (event.events & EPOLLOUT)
		processSendEvent(socket);

	if (event.events & EPOLLIN)
	{
		if (ipv4Socket)
		{
			while (true)
			{
				UDPReceivedResults results;
				if (recvBuffer.recvIPv4(socket, results) == false)
					return;

				for (auto& answer: results.answerList)
					deliverAnswer(results.connInfo, answer);
				
				for (auto& quest: results.questList)
					deliverQuest(results.connInfo, quest);
			}
		}
		else
		{
			while (true)
			{
				UDPReceivedResults results;
				if (recvBuffer.recvIPv6(socket, results) == false)
					return;
				
				for (auto& answer: results.answerList)
					deliverAnswer(results.connInfo, answer);
				
				for (auto& quest: results.questList)
					deliverQuest(results.connInfo, quest);
			}
		}
	}
}

void UDPEpollServer::processSendEvent(int socket)
{
	bool needWaitSendEvent = false;
	if (socket == _socket)
	{
		_sendBuffer.send(needWaitSendEvent);
		updateSocketStatus(socket, needWaitSendEvent);
	}
	else if (socket == _socket6)
	{
		_sendBuffer6.send(needWaitSendEvent);
		updateSocketStatus(socket, needWaitSendEvent);
	}
	else
		LOG_ERROR("Unknown UDP socket %d is triggered with send event in server process.", socket);
}

void UDPEpollServer::deliverAnswer(ConnectionInfoPtr connInfo, FPAnswerPtr answer)
{
	BasicAnswerCallback* callback = _callbackMap.takeCallback(connInfo, answer->seqNumLE());
	if (!callback)
	{
		LOG_ERROR("Received error answer seq is %u at UDP %s.", answer->seqNumLE(), connInfo->str().c_str());
		return;
	}
	if (callback->syncedCallback())		//-- check first, then fill result.
	{
		SyncedAnswerCallback* sac = (SyncedAnswerCallback*)callback;
		sac->fillResult(answer,  FPNN_EC_OK);
		return;
	}

	if (_answerCallbackPool)
	{
		callback->fillResult(answer, FPNN_EC_OK);

		BasicAnswerCallbackPtr task(callback);
		_answerCallbackPool->wakeUp(task);
	}
	else
	{
		delete callback;
		LOG_ERROR("UDP server received an answer, but process answers is desiabled. Answer will be dropped. %s", connInfo->str().c_str());
	}
}

bool UDPEpollServer::returnServerStoppingAnswer(ConnectionInfoPtr connInfo, FPQuestPtr quest)
{
	try
	{
		FPAnswerPtr answer = FpnnErrorAnswer(quest, FPNN_EC_CORE_SERVER_STOPPING, Config::_sName);
		std::string *raw = answer->raw();
		sendData(connInfo, raw);
		return true;
	}
	catch (const FpnnError& ex)
	{
		LOG_ERROR("Exception when return UDP server stopping answer. %s, exception:(%d)%s", connInfo->str().c_str(), ex.code(), ex.what());
		return false;
	}
	catch (...)
	{
		LOG_ERROR("Exception when return UDP server stopping answer. %s", connInfo->str().c_str());
		return false;
	}

	return true;
}

void UDPEpollServer::deliverQuest(ConnectionInfoPtr connInfo, FPQuestPtr quest)
{
	if (_stopping)
	{
		returnServerStoppingAnswer(connInfo, quest);
		return;
	}

	bool prior = false;
	const std::string& questMethod = quest->method();

	if (questMethod[0] == '*')
	{		
		if (_serverMasterProcessor->isMasterMethod(questMethod))
			prior = true;
	}

	UDPRequestPackage* requestPackage = new(std::nothrow) UDPRequestPackage(connInfo, quest);
	if (!requestPackage)
	{
		LOG_ERROR("Alloc Event package for received quest (%s) of UDP server failed. %s", questMethod.c_str(), connInfo->str().c_str());
		return;
	}

	bool wakeUpResult;
	if (!prior)
		wakeUpResult = _workerPool->wakeUp(requestPackage);
	else
		wakeUpResult = _workerPool->priorWakeUp(requestPackage);

	if (wakeUpResult == false)
	{
		if (!_workerPool->exiting())
		{
			LOG_ERROR("Worker pool wake up for quest (%s) failed, UDP server worker Pool length limitation is caught. %s",
				questMethod.c_str(), connInfo->str().c_str());

			if (quest->isTwoWay())
			{
				try
				{
					FPAnswerPtr answer = FpnnErrorAnswer(quest, FPNN_EC_CORE_WORK_QUEUE_FULL, std::string("Server queue is full!"));
					std::string *raw = answer->raw();
					sendData(connInfo, raw);
				}
				catch (const FpnnError& ex)
				{
					LOG_ERROR("Generate error answer for UDP server worker queue full failed. Quest %s, %s, exception:(%d)%s",
						questMethod.c_str(), connInfo->str().c_str(), ex.code(), ex.what());
				}
				catch (...)
				{
					LOG_ERROR("Generate error answer for UDP server worker queue full failed. Quest %s, %s",
						questMethod.c_str(), connInfo->str().c_str());
				}
			}
		}
		else
			LOG_ERROR("Worker pool wake up for quest (%s) failed, UDP server is exiting. %s", questMethod.c_str(), connInfo->str().c_str());

		delete requestPackage;
	}
}

void UDPEpollServer::stop()
{
	if(!_running) return;

	LOG_INFO("UDP server will go to stop!");

	_stopping = true;

	//call user defined function before server exit
	_serverMasterProcessor->getQuestProcessor()->serverWillStop();

	_running = false;
	ServerController::joinTimeoutCheckThread();

	{
		pipe(_closeNotifyFds);

		nonblockedFd(_closeNotifyFds[0]);
		struct epoll_event      ev;

		ev.data.fd = _closeNotifyFds[1];
		ev.events = EPOLLIN | EPOLLOUT | EPOLLERR | EPOLLHUP | EPOLLET | EPOLLRDHUP | EPOLLONESHOT;

		epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, _closeNotifyFds[1], &ev);
		write(_closeNotifyFds[1], this, 4);
	}

	LOG_INFO("UDP server is stopping ...");

	_stopSignalNotified = true;
}

void UDPEpollServer::clean()
{
	if (_socket > 0)
	{
		close(_socket);
		_socket = 0;
	}

	if (_socket6 > 0)
	{
		close(_socket6);
		_socket6 = 0;
	}

	while (!_stopSignalNotified)
		usleep(20000);

	close(_closeNotifyFds[1]);
	close(_closeNotifyFds[0]);
	_closeNotifyFds[0] = 0;
	_closeNotifyFds[1] = 0;
	
	clearRemnantCallbacks();

	_workerPool->release();
	if (_answerCallbackPool)
		_answerCallbackPool->release();

	close(_epoll_fd);
	_epoll_fd = 0;

	if (_epollEvents)
	{
		delete [] _epollEvents;
		_epollEvents = NULL;
	}

	LOG_INFO("Server stopped.");

	//call user defined function after server exit
	_serverMasterProcessor->getQuestProcessor()->serverStopped();
}

void UDPEpollServer::clearRemnantCallbacks()
{
	std::set<BasicAnswerCallback*> callbacks;
	_callbackMap.clearAndFetchAllRemnantCallbacks(callbacks);

	for (auto callback: callbacks)
	{
		if (callback->syncedCallback())		//-- check first, then fill result.
			callback->fillResult(NULL, FPNN_EC_CORE_SERVER_STOPPING);
		else
		{
			if (_answerCallbackPool)
			{
				callback->fillResult(NULL, FPNN_EC_CORE_SERVER_STOPPING);

				BasicAnswerCallbackPtr task(callback);
				_answerCallbackPool->wakeUp(task);
			}
			else
				delete callback;
		}
	}
}

void UDPEpollServer::sendData(ConnectionInfoPtr connInfo, std::string* data)
{
	bool needWaitSendEvent = false, actualSent = false;
	if (connInfo->socket == _socket)
	{
		_sendBuffer.send(connInfo, needWaitSendEvent, actualSent, data);
		updateSocketStatus(_socket, needWaitSendEvent);
	}
	else if (connInfo->socket == _socket6)
	{
		_sendBuffer6.send(connInfo, needWaitSendEvent, actualSent, data);
		updateSocketStatus(_socket6, needWaitSendEvent);
	}
	else
	{
		delete data;
		LOG_ERROR("Send dat on unknown UDP socket %d is dropped.", connInfo->socket);
	}
}

bool UDPEpollServer::sendQuestWithBasicAnswerCallback(ConnectionInfoPtr connInfo, FPQuestPtr quest, BasicAnswerCallback* callback, int timeout)
{
	if (!quest)
		return false;

	if (quest->isTwoWay() && !callback)
		return false;

	std::string* raw = NULL;
	try
	{
		raw = quest->raw();
	}
	catch (const FpnnError& ex){
		LOG_ERROR("Quest Raw Exception:(%d)%s", ex.code(), ex.what());
		return false;
	}
	catch (...)
	{
		LOG_ERROR("Quest Raw Exception.");
		return false;
	}

	if (callback)
	{
		uint32_t seqNum = quest->seqNumLE();
		callback->updateExpiredTime(slack_real_msec() + timeout);
		_callbackMap.insert(connInfo, seqNum, callback);
	}

	sendData(connInfo, raw);

	return true;
}

/*void UDPEpollServer::sendAnswer(ConnectionInfoPtr connInfo, FPAnswerPtr answer)
{
	std::string* raw = NULL;
	try
	{
		raw = answer->raw();
	}
	catch (const FpnnError& ex){
		answer = FpnnErrorAnswer(quest, ex.code(), std::string(ex.what()) + ", " + connInfo->str());
		raw = answer->raw();
	}
	catch (...)
	{
		answer = FpnnErrorAnswer(quest, FPNN_EC_CORE_UNKNOWN_ERROR, std::string("exception while do answer raw, ") + connInfo->str());
		raw = answer->raw();
	}

	Config::ServerAnswerAndSlowLog(quest, answer, connInfo->ip, connInfo->port);

	sendData(connInfo, raw);
}*/
FPAnswerPtr UDPEpollServer::sendQuest(ConnectionInfoPtr connInfo, FPQuestPtr quest, int timeout)
{
	if (!quest->isTwoWay())
	{
		sendQuestWithBasicAnswerCallback(connInfo, quest, NULL, 0);
		return NULL;
	}

	if (timeout == 0) timeout = _timeoutQuest;

	std::mutex local_mutex;
	std::shared_ptr<SyncedAnswerCallback> s(new SyncedAnswerCallback(&local_mutex, quest));
	if (!sendQuestWithBasicAnswerCallback(connInfo, quest, s.get(), timeout))
	{
		return FpnnErrorAnswer(quest, FPNN_EC_CORE_SEND_ERROR, "unknown sending error.");
	}

	return s->takeAnswer();
}

void UDPEpollServer::checkTimeout()
{
	if (_running)
	{
		//-- Quest Timeout: process as milliseconds
		int64_t current = slack_real_msec();
		std::list<std::map<uint32_t, BasicAnswerCallback*> > timeouted;

		_callbackMap.extractTimeoutedCallback(current, timeouted);
		for (auto& bacMap: timeouted)
		{
			for (auto& bacPair: bacMap)
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
						_answerCallbackPool->wakeUp(task);
					}
				}
			}
		}
	}
}