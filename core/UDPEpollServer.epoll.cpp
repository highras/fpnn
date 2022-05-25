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

//-- For unused the returned values of pipe() & write() in UDPEpollServer::stop().
#pragma GCC diagnostic ignored "-Wunused-result"

using namespace fpnn;

UDPServerPtr UDPEpollServer::_server = nullptr;

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

	//-- ECC-AES encryption
	_encryptEnabled = Setting::getBool(std::vector<std::string>{
		"FPNN.server.udp.security.ecdh.enable",
		"FPNN.server.security.ecdh.enable"}, false);
	if (_encryptEnabled)
	{
		if (_keyExchanger.init("udp") == false)
		{
			throw FPNN_ERROR_CODE_FMT(FpnnCoreError, FPNN_EC_CORE_UNKNOWN_ERROR, "Auto config ECC-AES for UDP server failed.");
		}
	}

	_heldLogger = FPLog::instance();

	return true;
}

void UDPEpollServer::enableForceEncryption()
{
	Config::UDP::_server_user_methods_force_encrypted = true;
}

bool UDPEpollServer::prepare()
{
	if (_serverMasterProcessor->checkQuestProcessor() == false)
	{
		LOG_FATAL("Invalide Quest Processor for UDP server.");
		return false;
	}

	//_connectionMap.init(128);
	int cpuCount = getCPUCount();
	_serverMasterProcessor->setServer(this);

	if (_workerPool == nullptr)
	{
		int workThreadMin = Setting::getInt(std::vector<std::string>{
			"FPNN.server.udp.work.thread.min.size", "FPNN.server.work.thread.min.size"}, cpuCount);
		int workThreadMax = Setting::getInt(std::vector<std::string>{
			"FPNN.server.udp.work.thread.max.size", "FPNN.server.work.thread.max.size"}, cpuCount);

		ServerUtils::adjustThreadPoolParams(workThreadMin, workThreadMax, 2, 256);
		configWorkerThreadPool(workThreadMin, 1, workThreadMin, workThreadMax, _maxWorkerPoolQueueLength);
	}

	if (!_ioWorker)
	{
		_ioWorker.reset(new UDPServerIOWorker);
		//_ioWorker->setWorkerPool(_workerPool);
		_ioWorker->setServer(this);
	}
	if (_ioPool == nullptr)
	{
		_ioPool = GlobalIOPool::instance();
		_ioPool->setServerIOWorker(_ioWorker);

		{
			/** 
				Just ensure io Pool is inited. If is inited, it will not reconfig or reinit.
			*/

			int ioThreadMin = Setting::getInt("FPNN.global.io.thread.min.size", cpuCount);
			int ioThreadMax = Setting::getInt("FPNN.global.io.thread.max.size", cpuCount);

			ServerUtils::adjustThreadPoolParams(ioThreadMin, ioThreadMax, 2, 256);
			_ioPool->init(ioThreadMin, 1, ioThreadMin, ioThreadMax);
		}

		/**
			Ensure helodLogger is newset.
			Because ClientEngine maybe startup before server created. If in that case, the ioPool is inited by ClientEngine,
			then, the FPlog is reinited in constructor of server. So, the held logger need to be refreshed.
		*/
		_ioPool->updateHeldLogger();
	}

	{
		int duplexThreadMin = Setting::getInt(std::vector<std::string>{
			"FPNN.server.udp.duplex.thread.min.size", "FPNN.server.duplex.thread.min.size"}, 0);
		int duplexThreadMax = Setting::getInt(std::vector<std::string>{
			"FPNN.server.udp.duplex.thread.max.size", "FPNN.server.duplex.thread.max.size"}, 0);

		if (_answerCallbackPool == nullptr)
		{
			int perfectThreadCount = duplexThreadMin;

			if (duplexThreadMax <= 0)
				duplexThreadMax = getCPUCount();

			if (duplexThreadMin <= 0)
			{
				duplexThreadMin = 0;
				perfectThreadCount = duplexThreadMax;
			}

			enableAnswerCallbackThreadPool(duplexThreadMin, 1, perfectThreadCount, duplexThreadMax);
		}
	}

	if (!_ip.empty())
		_ip = HostLookup::get(_ip);

	//if (!_ipv6.empty())
	//	_ipv6 = HostLookup::get(_ipv6);

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

const char* UDPEpollServer::createSocket(int socketDomain, const struct sockaddr *addr, socklen_t addrlen, int& newSocket)
{
	newSocket = socket(socketDomain, SOCK_DGRAM, 0);
	if (newSocket == -1)
	{
		newSocket = 0;
		return "Create UDP socket failed.";
	}

	int reuse_flag = 1;
	if (-1 == setsockopt(newSocket, SOL_SOCKET, SO_REUSEADDR, (void*)&(reuse_flag), sizeof(reuse_flag)))
	{
		return "Set UDP socket SO_REUSEADDR failed.";
	}

	if (-1 == setsockopt(newSocket, SOL_SOCKET, SO_REUSEPORT, (void*)&(reuse_flag), sizeof(reuse_flag)))
	{
		return "Set UDP socket SO_REUSEPORT failed.";
	}

	if (!nonblockedFd(newSocket))
	{
		return "Change UDP socket to non-blocked failed.";
	}

	if (-1 == bind(newSocket, addr, addrlen)) 
	{
		return "Bind UDP socket failed.";
	}

	return NULL;
}

bool UDPEpollServer::initIPv4()
{
	memset(&_serverAddr, 0, sizeof(_serverAddr));
	_serverAddr.sin_family = AF_INET; 
	_serverAddr.sin_port = htons(_port);

	if (_ip.empty())
		_serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	else
		_serverAddr.sin_addr.s_addr = inet_addr(_ip.c_str());
	
	if (_serverAddr.sin_addr.s_addr == INADDR_NONE)
	{
		std::string errorMsg("Invalid UDP IPv4 address: ");
		errorMsg.append(_ip);
		initFailClean(errorMsg.c_str());
		return false;
	}

	const char* errorMessage = createSocket(AF_INET, (struct sockaddr *)(&_serverAddr), sizeof(_serverAddr), _socket);
	if (errorMessage != NULL)
	{
		std::string info("IPv4 listening socket create failed. Reason: ");
		info += errorMessage;

		initFailClean(info.c_str());
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

	return true;
}

bool UDPEpollServer::initIPv6()
{
	memset(&_serverAddr6, 0, sizeof(_serverAddr6));
	_serverAddr6.sin6_family = AF_INET6; 
	_serverAddr6.sin6_port = htons(_port6);

	if (_ipv6.empty())
		_serverAddr6.sin6_addr = in6addr_any;
	else if (inet_pton(AF_INET6, _ipv6.c_str(), &_serverAddr6.sin6_addr) != 1)
	{
		std::string errorMsg("Invalid UDP IPv6 address: ");
		errorMsg.append(_ipv6);
		initFailClean(errorMsg.c_str());
		return false;
	}

	const char* errorMessage = createSocket(AF_INET6, (struct sockaddr *)(&_serverAddr6), sizeof(_serverAddr6), _socket6);
	if (errorMessage != NULL)
	{
		std::string info("IPv6 listening socket create failed. Reason: ");
		info += errorMessage;

		initFailClean(info.c_str());
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

void UDPEpollServer::exitCheck()
{
	if (_serverMasterProcessor->getQuestProcessor())
	{
		//call user defined function after server exit
		_serverMasterProcessor->getQuestProcessor()->serverWillStop();
		_serverMasterProcessor->getQuestProcessor()->serverStopped();
		// force release business processor
		_serverMasterProcessor->setQuestProcessor(nullptr);
	}
}

void UDPEpollServer::run()
{
	//-- force exit when startup() failed.
	if (_epoll_fd == 0)
	{
		//call user defined function after server exit
		_serverMasterProcessor->getQuestProcessor()->serverWillStop();
		_serverMasterProcessor->getQuestProcessor()->serverStopped();
		// force release business processor
		_serverMasterProcessor->setQuestProcessor(nullptr);
		
		return;
	}

	_running = true;
	_stopping = false;
	_stopSignalNotified = false;

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
		{
			if (_epollEvents[i].data.fd == _socket)
			{
				createConnections(_socket);
			}
			else if (_epollEvents[i].data.fd == _socket6)
			{
				createConnections6(_socket6);
			}
			else
				processEvent(_epollEvents[i]);
		}

		recheckNewSockets();
		scheduleNewConnections();
	}
}

void UDPEpollServer::createConnections(int socket)
{
	while (true)
	{
		if (_receiver.recvIPv4(socket) == false)
			return;

		if (_receiver.udpRawData == NULL)
			continue;

		if (checkSourceAddress() == false)
			continue;

		int newSocket;
		const char* errorMessage = createSocket(AF_INET, (struct sockaddr *)(&_serverAddr), sizeof(_serverAddr), newSocket);
		if (errorMessage != NULL)
		{
			delete _receiver.udpRawData;

			if (newSocket > 0)
				close(newSocket);

			LOG_ERROR("IPv4 socket create for new UDP connection (%s:%d) failed. Reason: %s",
				_receiver.ip.c_str(), _receiver.port, errorMessage);

			continue;
		}

		if (connect(newSocket, (struct sockaddr *)(_receiver.sockaddr), _receiver.requiredAddrLen) != 0)
		{
			close(newSocket);
			delete _receiver.udpRawData;
			LOG_ERROR("Connect new IPv4 socket for new UDP connection (%s:%d) failed.", _receiver.ip.c_str(), _receiver.port);
			continue;
		}

		newConnection(newSocket, true);
		_newSockets.insert(newSocket);
	}
}

void UDPEpollServer::createConnections6(int socket6)
{
	while (true)
	{
		if (_receiver.recvIPv6(socket6) == false)
			return;

		if (_receiver.udpRawData == NULL)
			continue;

		if (checkSourceAddress() == false)
			continue;

		int newSocket;
		const char* errorMessage = createSocket(AF_INET6, (struct sockaddr *)(&_serverAddr6), sizeof(_serverAddr6), newSocket);
		if (errorMessage != NULL)
		{
			delete _receiver.udpRawData;

			if (newSocket > 0)
				close(newSocket);

			LOG_ERROR("IPv6 socket create for new UDP connection (%s:%d) failed. Reason: %s",
				_receiver.ip.c_str(), _receiver.port, errorMessage);

			continue;
		}

		if (connect(newSocket, (struct sockaddr *)(_receiver.sockaddr), _receiver.requiredAddrLen) != 0)
		{
			close(newSocket);
			delete _receiver.udpRawData;
			LOG_ERROR("Connect new IPv6 socket for new UDP connection (%s:%d) failed.", _receiver.ip.c_str(), _receiver.port);
			continue;
		}

		newConnection(newSocket, false);
		_newSockets6.insert(newSocket);
	}
}

bool UDPEpollServer::checkSourceAddress()
{
	UDPServerConnection* connection = _connectionCache.check(_receiver);
	if (connection == NULL)
		return true;

	connection->appendRawData(_receiver.udpRawData);
	return false;
}

void UDPEpollServer::newConnection(int newSocket, bool isIPv4)
{
	ConnectionInfoPtr connInfo(new ConnectionInfo(newSocket, _receiver.port, _receiver.ip, isIPv4, true));
	connInfo->changeToUDP((uint8_t*)_receiver.sockaddr);

	int mtu = connInfo->isPrivateIP() ? Config::UDP::_LAN_MTU : Config::UDP::_internet_MTU;

	UDPServerConnection* connection = new UDPServerConnection(connInfo, mtu);
	connection->epollfd = _epoll_fd;

	if (_encryptEnabled)
		connection->setKeyExchanger(&_keyExchanger);

	_connectionCache.insert(_receiver, connection);
	connection->appendRawData(_receiver.udpRawData);
	connection->_refCount++;
}

void UDPEpollServer::recheckNewSockets()
{
	std::set<int> sockets;

	while (_newSockets.size())
	{
		sockets.swap(_newSockets);

		for (auto socket: sockets)
		{
			createConnections(socket);

		}
		sockets.clear();
	}

	while (_newSockets6.size())
	{
		sockets.swap(_newSockets6);

		for (auto socket: sockets)
		{
			createConnections6(socket);

		}
		sockets.clear();
	}
}

void UDPEpollServer::scheduleNewConnections()
{
	std::unordered_map<std::string, UDPServerConnection*>& cache = _connectionCache.getCache();

	_connectionMap.insert(cache);
	for (auto& pp: cache)
		_ioPool->wakeUp(pp.second);

	_connectionCache.reset();
}

void UDPEpollServer::processEvent(struct epoll_event & event)
{
	int socket = event.data.fd;

	//-- Please care the order.
	if (event.events & EPOLLRDHUP || event.events & EPOLLHUP)
	{
		terminateConnection(socket, UDPSessionEventType::Closed, FPNN_EC_CORE_CONNECTION_CLOSED);
		return;
	}
	else if (event.events & EPOLLERR)
	{
		terminateConnection(socket, UDPSessionEventType::Closed, FPNN_EC_CORE_UNKNOWN_ERROR);
		return;
	}
	
	UDPServerConnection* conn = (UDPServerConnection*)_connectionMap.signConnection(socket, event.events);
	if (conn)
		_ioPool->wakeUp(conn);
}

void UDPEpollServer::connectionConnectedEventCompleted(UDPServerConnection* conn)
{
	bool noTerminationCalled = conn->connectedEventCompleted();

	if (noTerminationCalled && conn->isEncrypted())
	{
		bool needWaitSendEvent = false;
		conn->sendCachedData(needWaitSendEvent, true);
		if (needWaitSendEvent)
			noTerminationCalled = waitForAllEvents(conn->socket());

		if (conn->isRequireClose())
			noTerminationCalled = false;
	}

	if (!noTerminationCalled)
	{
		exitEpoll(conn->socket());

		if (deliverSessionEvent(conn, UDPSessionEventType::Closed) == false)
			conn->_refCount--;

		//-- terminateConnection() is called, just call closed event only.
		// terminateConnection(conn->socket(), UDPSessionEventType::Closed, FPNN_EC_CORE_CONNECTION_CLOSED);
		return;
	}

	//-- TO dispatch quest
	std::list<FPQuestPtr> cachedQuests;
	conn->swapCachedQuests(cachedQuests);

	for (auto& quest: cachedQuests)
		deliverQuest(conn, quest);

	conn->_refCount--;
}

bool UDPEpollServer::deliverSessionEvent(UDPServerConnection* conn, UDPSessionEventType type)
{
	UDPRequestPackage* requestPackage = new(std::nothrow) UDPRequestPackage(conn, type);
	if (!requestPackage)
	{
		LOG_ERROR("Alloc Event package for UDP session %s event failed. %s",
			(type == UDPSessionEventType::Connected) ? "connected" : "closing", conn->_connectionInfo->str().c_str());
		return false;
	}

	bool wakeUpResult;
	if (type == UDPSessionEventType::Connected)
		wakeUpResult = _workerPool->priorWakeUp(requestPackage);
	else
		wakeUpResult = _workerPool->forceWakeUp(requestPackage);

	if (wakeUpResult)
		return true;

	LOG_ERROR("Worker pool wake up for UDP session %s event failed. %s",
		(type == UDPSessionEventType::Connected) ? "connected" : "closing", conn->_connectionInfo->str().c_str());

	delete requestPackage;
	return false;
}

void UDPEpollServer::deliverAnswer(UDPServerConnection* conn, FPAnswerPtr answer)
{
	BasicAnswerCallback* callback = conn->takeCallback(answer->seqNumLE());
	if (!callback)
	{
		LOG_ERROR("Received error answer seq is %u at UDP endpoint %s.", answer->seqNumLE(), conn->_connectionInfo->endpoint().c_str());
		return;
	}
	if (callback->syncedCallback())		//-- check first, then fill result.
	{
		SyncedAnswerCallback* sac = (SyncedAnswerCallback*)callback;
		sac->fillResult(answer,  FPNN_EC_OK);
		return;
	}

	{
		callback->fillResult(answer, FPNN_EC_OK);

		BasicAnswerCallbackPtr task(callback);
		_answerCallbackPool->wakeUp(task);
	}
}

void UDPEpollServer::returnServerStoppingAnswer(UDPServerConnection* conn, FPQuestPtr quest)
{
	try
	{
		FPAnswerPtr answer = FpnnErrorAnswer(quest, FPNN_EC_CORE_SERVER_STOPPING, Config::_sName);
		std::string *raw = answer->raw();
		conn->sendData(raw, slack_real_msec() + _timeoutQuest, false);
	}
	catch (const FpnnError& ex)
	{
		LOG_ERROR("Exception when return UDP server stopping answer. endpoint %s, exception:(%d)%s", conn->_connectionInfo->endpoint().c_str(), ex.code(), ex.what());
	}
	catch (...)
	{
		LOG_ERROR("Exception when return UDP server stopping answer. endpoint %s", conn->_connectionInfo->endpoint().c_str());
	}
}

void UDPEpollServer::deliverQuest(UDPServerConnection* conn, FPQuestPtr quest)
{
	if (_stopping)
	{
		if (quest->isOneWay())
			return;
		
		returnServerStoppingAnswer(conn, quest);
		return;
	}

	bool prior = false;
	const std::string& questMethod = quest->method();

	if (questMethod[0] == '*')
	{		
		if (_serverMasterProcessor->isMasterMethod(questMethod))
			prior = true;
	}

	UDPRequestPackage* requestPackage = new(std::nothrow) UDPRequestPackage(conn->_connectionInfo, quest);
	if (!requestPackage)
	{
		LOG_ERROR("Alloc Event package for received quest (%s) of UDP server failed. %s", questMethod.c_str(), conn->_connectionInfo->str().c_str());
		return;
	}

	bool wakeUpResult;
	if (!prior)
	{
		if (Config::UDP::_server_user_methods_force_encrypted && !conn->isEncrypted())
		{
			LOG_WARN("All user methods reuiqre encrypted. Unencrypted connection will visit %s. Connection will be closed by server. %s",
				questMethod.c_str(), conn->_connectionInfo->str().c_str());

			delete requestPackage;
			return;
		}

		wakeUpResult = _workerPool->wakeUp(requestPackage);
	}
	else
		wakeUpResult = _workerPool->priorWakeUp(requestPackage);

	if (wakeUpResult == false)
	{
		if (!_workerPool->exiting())
		{
			LOG_ERROR("Worker pool wake up for quest (%s) failed, UDP server worker Pool length limitation is caught. %s",
				questMethod.c_str(), conn->_connectionInfo->str().c_str());

			if (quest->isTwoWay())
			{
				try
				{
					FPAnswerPtr answer = FpnnErrorAnswer(quest, FPNN_EC_CORE_WORK_QUEUE_FULL, std::string("Server queue is full!"));
					std::string *raw = answer->raw();
					conn->sendData(raw, slack_real_msec() + _timeoutQuest, false);
				}
				catch (const FpnnError& ex)
				{
					LOG_ERROR("Generate error answer for UDP server worker queue full failed. Quest %s, %s, exception:(%d)%s",
						questMethod.c_str(), conn->_connectionInfo->str().c_str(), ex.code(), ex.what());
				}
				catch (...)
				{
					LOG_ERROR("Generate error answer for UDP server worker queue full failed. Quest %s, %s",
						questMethod.c_str(), conn->_connectionInfo->str().c_str());
				}
			}
		}
		else
			LOG_WARN("Worker pool wake up for quest (%s) failed, UDP server is exiting. %s", questMethod.c_str(), conn->_connectionInfo->str().c_str());

		delete requestPackage;
	}
}

void UDPEpollServer::reclaimeConnection(UDPServerConnection* conn)
{
	IReleaseablePtr autoRelease(conn);
	_reclaimer->reclaim(autoRelease);
}

void UDPEpollServer::stop()
{
	if(!_running) return;

	LOG_INFO("UDP server will go to stop!");

	_stopping = true;
	_ioWorker->serverWillStop();

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
	/*
	//-- 需要在 _workerPool release 之后关闭，否则 ARQ 链接的关闭事件，无法发送。
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
	*/

	while (!_stopSignalNotified)
		usleep(20000);

	close(_closeNotifyFds[1]);
	close(_closeNotifyFds[0]);
	_closeNotifyFds[0] = 0;
	_closeNotifyFds[1] = 0;
	
	clearRemnantConnections();

	_workerPool->release();
	_answerCallbackPool->release();

	//-- 需要在 _workerPool release 之后关闭，否则 ARQ 链接的关闭事件，无法发送。
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
	// force release business processor
	_serverMasterProcessor->setQuestProcessor(nullptr);
}

void UDPEpollServer::clearRemnantConnections()
{
	_connectionMap.markAllConnectionsActiveCloseSignal();
	periodSendingCheck();

	std::list<UDPServerConnection*> connections;
	_connectionMap.removeAllConnections(connections);

	for (auto conn: connections)
		terminateConnection(conn->socket(), UDPSessionEventType::Closed, FPNN_EC_CORE_SERVER_STOPPING);
}

void UDPEpollServer::sendData(int socket, uint64_t token, std::string* data, int64_t expiredMS, bool discardable)
{
	if (expiredMS == 0)
		expiredMS = slack_real_msec() + _timeoutQuest;

	if (!_connectionMap.sendData(socket, token, data, expiredMS, discardable))
	{
		delete data;
		LOG_WARN("UDP data not send at socket %d, address: %s. socket maybe closed.", socket, NetworkUtil::getPeerName(socket).c_str());
	}
}

bool UDPEpollServer::sendQuestWithBasicAnswerCallback(int socket, uint64_t token, FPQuestPtr quest, BasicAnswerCallback* callback, int timeout, bool discardable)
{
	if (_stopping)
	{
		LOG_ERROR("Send duplex quest when server begin stopping.");
		return false;
	}

	return _connectionMap.sendQuestWithBasicAnswerCallback(socket, token, quest, callback, timeout, discardable);
}

FPAnswerPtr UDPEpollServer::sendQuest(int socket, uint64_t token, FPQuestPtr quest, int timeout, bool discardable)
{
	if (!quest->isTwoWay())
	{
		sendQuestWithBasicAnswerCallback(socket, token, quest, NULL, 0, discardable);
		return NULL;
	}

	if (timeout == 0) timeout = _timeoutQuest;

	std::mutex local_mutex;
	std::shared_ptr<SyncedAnswerCallback> s(new SyncedAnswerCallback(&local_mutex, quest));
	if (!sendQuestWithBasicAnswerCallback(socket, token, quest, s.get(), timeout, discardable))
	{
		return FpnnErrorAnswer(quest, FPNN_EC_CORE_SEND_ERROR, "unknown sending error.");
	}

	return s->takeAnswer();
}

bool UDPEpollServer::joinEpoll(int socket)
{
	struct epoll_event	ev;

	ev.data.fd = socket;
	ev.events = EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLET | EPOLLRDHUP | EPOLLONESHOT;

	if (epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, socket, &ev) != 0)
		return false;
	else
		return true;
}

bool UDPEpollServer::waitForAllEvents(int socket)
{
	struct epoll_event	ev;

	ev.data.fd = socket;
	ev.events = EPOLLIN | EPOLLOUT | EPOLLERR | EPOLLHUP | EPOLLET | EPOLLRDHUP | EPOLLONESHOT;

	if (epoll_ctl(_epoll_fd, EPOLL_CTL_MOD, socket, &ev) != 0)
	{
		if (errno == ENOENT)
			return false;
		
		LOG_INFO("UDP Server wait socket event failed. Socket: %d, address: %s, errno: %d", socket, NetworkUtil::getPeerName(socket).c_str(), errno);
		return false;
	}
	else
		return true;
}
bool UDPEpollServer::waitForRecvEvent(int socket)
{
	struct epoll_event	ev;

	ev.data.fd = socket;
	ev.events = EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLET | EPOLLRDHUP | EPOLLONESHOT;

	if (epoll_ctl(_epoll_fd, EPOLL_CTL_MOD, socket, &ev) != 0)
	{
		if (errno == ENOENT)
			return false;

		LOG_INFO("UDP Server wait socket event failed. Socket: %d, address: %s, errno: %d", socket, NetworkUtil::getPeerName(socket).c_str(), errno);
		return false;
	}
	else
		return true;
}

void UDPEpollServer::exitEpoll(int socket)
{
	struct epoll_event	ev;

	ev.data.fd = socket;
	ev.events = 0;

	epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, socket, &ev);
}

void UDPEpollServer::closeConnection(int socket, bool force)
{
	if (force == false)
	{
		_connectionMap.markConnectionActiveCloseSignal(socket);
	}
	else
	{
		terminateConnection(socket, UDPSessionEventType::Closed, FPNN_EC_CORE_CONNECTION_CLOSED);
	}
}

void UDPEpollServer::terminateConnection(int socket, UDPSessionEventType type, int errorCode)
{
	UDPServerConnection* conn = _connectionMap.remove(socket);
	if (!conn)
		return;

	internalTerminateConnection(conn, socket, type, errorCode);
}

void UDPEpollServer::internalTerminateConnection(UDPServerConnection* conn, int socket, UDPSessionEventType type, int errorCode)
{
	exitEpoll(socket);

	clearConnectionQuestCallbacks(conn, errorCode);

	if (conn->isConnectedEventTriggered(true))
	{
		conn->_refCount++;

		if (deliverSessionEvent(conn, type) == false)
			conn->_refCount--;
	}
	
	reclaimeConnection(conn);
}

void UDPEpollServer::clearConnectionQuestCallbacks(UDPServerConnection* conn, int errorCode)
{
	std::unordered_map<uint32_t, BasicAnswerCallback*> callbackMap;
	conn->swapCallbackMap(callbackMap);

	for (auto callbackPair: callbackMap)
	{
		BasicAnswerCallback* callback = callbackPair.second;
		if (callback->syncedCallback())		//-- check first, then fill result.
			callback->fillResult(NULL,  errorCode);
		else
		{
			callback->fillResult(NULL, errorCode);

			BasicAnswerCallbackPtr task(callback);
			_answerCallbackPool->wakeUp(task);
		}
	}
}

void UDPEpollServer::periodSendingCheck()
{
	_connectionMap.periodSending();
}

void UDPEpollServer::checkTimeout()
{
	if (_running)
	{
		std::list<UDPServerConnection*> invalidConnections;
		std::unordered_map<uint32_t, BasicAnswerCallback*> timeoutedCallbacks;
		_connectionMap.extractInvalidConnectionsAndCallbcks(invalidConnections, timeoutedCallbacks);

		for (UDPServerConnection* conn: invalidConnections)
			internalTerminateConnection(conn, conn->socket(), UDPSessionEventType::Closed, FPNN_EC_CORE_INVALID_CONNECTION);

		for (auto& cbPair: timeoutedCallbacks)
		{
			if (cbPair.second)
			{
				BasicAnswerCallback* callback = cbPair.second;
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

void UDPConnectionCache::insert(UDPServerReceiver& receiver, UDPServerConnection* conn)
{
	std::string key((const char*)receiver.sockaddr, (size_t)receiver.requiredAddrLen);

	_cache.emplace(key, conn);
}

UDPServerConnection* UDPConnectionCache::check(UDPServerReceiver& receiver)
{
	std::string key((const char*)receiver.sockaddr, (size_t)receiver.requiredAddrLen);
	auto it = _cache.find(key);
	if (it == _cache.end())
		return NULL;
	else
		return it->second;
}
