#include <netinet/in.h>
#include <sys/resource.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/sysinfo.h>
#include <sys/types.h>
//#include <netinet/tcp.h>  //-- for TCP_NODELAY
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <set>
#include "hex.h"
#include "HostLookup.h"
#include "FPLog.h"
#include "NetworkUtility.h"
#include "TCPEpollServer.h"
#include "msec.h"
#include "Config.h"
#include "TimeUtil.h"
#include "StringUtil.h"
#include "ServerController.h"
#include "OpenSSLModule.h"
#include "NetworkUtility.h"

//-- For unused the returned values of pipe() & write() in TCPEpollServer::stop().
#pragma GCC diagnostic ignored "-Wunused-result"

using namespace fpnn;

std::mutex TCPEpollServer::_sendQueueMutex[FPNN_SEND_QUEUE_MUTEX_COUNT];
TCPServerPtr TCPEpollServer::_server = nullptr;

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

int TCPEpollServer::getCPUCount()
{
	int cpuCount = get_nprocs();
	return (cpuCount < 2) ? 2 : cpuCount;
}

bool TCPEpollServer::initSystemVaribles(){
	Config::initSystemVaribles();
    return true;
}

void TCPEpollServer::SocketInfo::init(const std::vector<std::string>& ipItems, const std::vector<std::string>& portItems)
{
	int value = Setting::getInt(portItems, 0);
	if (value)
	{
		if(value <= 1024 || value > 65535){
			throw FPNN_ERROR_CODE_FMT(FpnnCoreError, FPNN_EC_CORE_UNKNOWN_ERROR, "Invalide TCP port(%d), should between(1024~~65535)", value);
		}
		port = value;

		ip = Setting::getString(ipItems, "");
		if(ip == "*") ip = "";	//-- listen all ip
	}
}

void TCPEpollServer::SocketInfo::close()
{
	if (socket > 0)
	{
		::close(socket);
		socket = 0;
	}
}

bool TCPEpollServer::initServerVaribles(){
	Config::initServerVaribles();

	_backlog = Setting::getInt("FPNN.server.backlog.size", FPNN_DEFAULT_SOCKET_BACKLOG);

	_ipv4.init(std::vector<std::string>{
			"FPNN.server.tcp.ipv4.listening.ip",
			"FPNN.server.ipv4.listening.ip",
			"FPNN.server.listening.ip",},
		std::vector<std::string>{
			"FPNN.server.tcp.ipv4.listening.port",
			"FPNN.server.ipv4.listening.port",
			"FPNN.server.listening.port"});

	_ipv6.init(std::vector<std::string>{
			"FPNN.server.tcp.ipv6.listening.ip",
			"FPNN.server.ipv6.listening.ip",},
		std::vector<std::string>{
			"FPNN.server.tcp.ipv6.listening.port",
			"FPNN.server.ipv6.listening.port",});

	_sslIPv4.init(
		std::vector<std::string>{"FPNN.server.tcp.ipv4.ssl.listening.ip",},
		std::vector<std::string>{"FPNN.server.tcp.ipv4.ssl.listening.port"});

	_sslIPv6.init(
		std::vector<std::string>{"FPNN.server.tcp.ipv6.ssl.listening.ip",},
		std::vector<std::string>{"FPNN.server.tcp.ipv6.ssl.listening.port",});

	if (_ipv4.port == 0 && _ipv6.port == 0 && _sslIPv4.port == 0 && _sslIPv6.port == 0)
		throw FPNN_ERROR_CODE_MSG(FpnnCoreError, FPNN_EC_CORE_UNKNOWN_ERROR, "Invalide TCP port. normal & SSL and IPv4 & IPv6 port are all unconfigurated.");

	_timeoutQuest = Setting::getInt(std::vector<std::string>{
		"FPNN.server.tcp.quest.timeout", "FPNN.server.quest.timeout"}, FPNN_DEFAULT_QUEST_TIMEOUT) * 1000;

	_timeoutIdle = Setting::getInt("FPNN.server.idle.timeout", FPNN_DEFAULT_IDLE_TIMEOUT) * 1000;

	_ioBufferChunkSize = Setting::getInt("FPNN.server.iobuffer.chunk.size", FPNN_DEFAULT_IO_BUFFER_CHUNK_SIZE);

	_max_events = Setting::getInt("FPNN.server.max.event", FPNN_DEFAULT_MAX_EVENT);

	_maxWorkerPoolQueueLength = Setting::getInt(std::vector<std::string>{
		"FPNN.server.tcp.work.queue.max.size", "FPNN.server.work.queue.max.size"}, FPNN_DEFAULT_WORK_POOL_QUEUE_SIZE);

	_duplexThreadMin = Setting::getInt(std::vector<std::string>{"FPNN.server.tcp.duplex.thread.min.size", "FPNN.server.duplex.thread.min.size"}, 0);
	_duplexThreadMax = Setting::getInt(std::vector<std::string>{"FPNN.server.tcp.duplex.thread.max.size", "FPNN.server.duplex.thread.max.size"}, 0);

	if (_duplexThreadMin > _duplexThreadMax) _duplexThreadMin = _duplexThreadMax;

	//-- ECC-AES encryption
	_encryptEnabled = Setting::getBool("FPNN.server.security.ecdh.enable", false);
	if (_encryptEnabled)
	{
		if (_keyExchanger.init() == false)
		{
			throw FPNN_ERROR_CODE_FMT(FpnnCoreError, FPNN_EC_CORE_UNKNOWN_ERROR, "Auto config ECC-AES failed.");
		}
	}

	return true;
}

void TCPEpollServer::enableForceEncryption()
{
	Config::_server_user_methods_force_encrypted = true;
}

bool TCPEpollServer::prepare()
{
	if (_serverMasterProcessor->checkQuestProcessor() == false)
	{
		LOG_FATAL("Invalide Quest Processor for TCP server.");
		return false;
	}

	const size_t minIOBufferChunkSize = 64;
	if (_ioBufferChunkSize < minIOBufferChunkSize)
		_ioBufferChunkSize = minIOBufferChunkSize;
	
	if (!_connectionMap.inited())
		setEstimateMaxConnections(Config::_server_perfect_connections);

	_serverMasterProcessor->setServer(this);

	if (_workerPool == nullptr)
	{
		int cpuCount = getCPUCount();
		int workThreadMin = Setting::getInt(std::vector<std::string>{"FPNN.server.tcp.work.thread.min.size", "FPNN.server.work.thread.min.size"}, cpuCount);
		int workThreadMax = Setting::getInt(std::vector<std::string>{"FPNN.server.tcp.work.thread.max.size", "FPNN.server.work.thread.max.size"}, cpuCount);

		adjustThreadPoolParams(workThreadMin, workThreadMax, 2, 256);
		configWorkerThreadPool(workThreadMin, 1, workThreadMin, workThreadMax, _maxWorkerPoolQueueLength);
	}

	if (!_ioWorker)
	{
		_ioWorker.reset(new TCPServerIOWorker);
		_ioWorker->setWorkerPool(_workerPool);
		_ioWorker->setServer(this);
	}
	if (_ioPool == nullptr)
	{
		_ioPool = GlobalIOPool::instance();
		_ioPool->setServerIOWorker(_ioWorker);

		{
			/** 
				Just ensure io Pool is inited. If is inited, it will not reconfig or reinit. */

			int cpuCount = getCPUCount();
			int ioThreadMin = Setting::getInt("FPNN.global.io.thread.min.size", cpuCount);
			int ioThreadMax = Setting::getInt("FPNN.global.io.thread.max.size", cpuCount);

			adjustThreadPoolParams(ioThreadMin, ioThreadMax, 2, 256);
			_ioPool->init(ioThreadMin, 1, ioThreadMin, ioThreadMax);
		}

		/**
			Ensure helodLogger is newset.
			Because ClientEngine maybe startup before server created. If in that case, the ioPool is inited by ClientEngine,
			then, the FPlog is reinited in constructor of server. So, the held logger need to be refreshed.
		*/
		_ioPool->updateHeldLogger();
	}

	if (_answerCallbackPool == nullptr)
	{
		if (_duplexThreadMin > 0 && _duplexThreadMax > 0)
			enableAnswerCallbackThreadPool(_duplexThreadMin, 1, _duplexThreadMin, _duplexThreadMax);
		else
		{
			int cpuCount = getCPUCount();
			enableAnswerCallbackThreadPool(0, 1, cpuCount, cpuCount);
		}
	}

	if (!_ipv4.ip.empty())
		_ipv4.ip = HostLookup::get(_ipv4.ip);
	//if (!_ipv6.ip.empty())
	//	_ipv6.ip = HostLookup::get(_ipv6.ip);
	if (!_sslIPv4.ip.empty())
		_sslIPv4.ip = HostLookup::get(_sslIPv4.ip);
	//if (!_sslIPv6.ip.empty())
	//	_sslIPv6.ip = HostLookup::get(_sslIPv6.ip);

	if (_sslIPv4.port || _sslIPv6.port)
		return OpenSSLModule::serverModuleInit();

	return true;
}

void TCPEpollServer::initFailClean(const char* fail_info)
{
	_ipv4.close();
	_ipv6.close();
	_sslIPv4.close();
	_sslIPv6.close();

	close(_epoll_fd);
	_epoll_fd = 0;

	if (_epollEvents)
	{
		delete [] _epollEvents;
		_epollEvents = NULL;
	}

	if (fail_info)
		LOG_FATAL(fail_info);
}

bool TCPEpollServer::init()
{
	if (!initEpoll())
		return false;

	if (_ipv4.port)
		if (!initIPv4(_ipv4))
			return false;

	if (_ipv6.port)
		if (!initIPv6(_ipv6))
			return false;

	if (_sslIPv4.port)
		if (!initIPv4(_sslIPv4))
			return false;

	if (_sslIPv6.port)
		if (!initIPv6(_sslIPv6))
			return false;

	_running = true;
	return true;
}

bool TCPEpollServer::initEpoll()
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
		initFailClean("Create epoll events matrix for TCP server failed.");
		return false;
	}

	return true;
}

bool TCPEpollServer::initIPv4(struct SocketInfo& info)
{
	struct sockaddr_in serverAddr;

	memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET; 
	serverAddr.sin_port = htons(info.port);

	if (info.ip.empty())
		serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	else
		serverAddr.sin_addr.s_addr = inet_addr(info.ip.c_str());
	
	if (serverAddr.sin_addr.s_addr == INADDR_NONE)
	{
		std::string errorMsg("Invalid TCP IPv4 address: ");
		errorMsg.append(info.ip);
		initFailClean(errorMsg.c_str());
		return false;
	}

	info.socket = socket(AF_INET, SOCK_STREAM, 0);
	if (info.socket == -1)
	{
		info.socket = 0;
		initFailClean("Create IPv4 socket failed.");
		return false;
	}

	int reuse_addr = 1;
	if (-1 == setsockopt(info.socket, SOL_SOCKET, SO_REUSEADDR, (void*)&(reuse_addr), sizeof(reuse_addr)))
	{
		initFailClean("Resue IPv4 socket failed.");
		return false;
	}

	if (!nonblockedFd(info.socket))
	{
		initFailClean("Change IPv4 socket to non-blocked failed.");
		return false;
	}

	//-- Add Listening Socket to epoll
	struct epoll_event ev;
	ev.data.fd = info.socket;
	ev.events = EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLET | EPOLLRDHUP;

	if (-1 == epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, info.socket, &ev))
	{
		initFailClean("Add IPv4 listening socket to epoll failed.");
		return false;
	}

	//if (-1 == bind(info.socket, (struct sockaddr *)(&serverAddr), sizeof(struct sockaddr))) 
	if (-1 == bind(info.socket, (struct sockaddr *)(&serverAddr), sizeof(serverAddr))) 
	{
		initFailClean("Bind listening socket failed.");
		return false;
	} 

	if (-1 == listen(info.socket, _backlog)) 
	{
		initFailClean("Listening failed.");
		return false;
	}

	return true;
}

bool TCPEpollServer::initIPv6(struct SocketInfo& info)
{
	struct sockaddr_in6 serverAddr;

	memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin6_family = AF_INET6; 
	serverAddr.sin6_port = htons(info.port);

	if (info.ip.empty())
		serverAddr.sin6_addr = in6addr_any;
	else if (inet_pton(AF_INET6, info.ip.c_str(), &serverAddr.sin6_addr) != 1)
	{
		std::string errorMsg("Invalid TCP IPv6 address: ");
		errorMsg.append(info.ip);
		initFailClean(errorMsg.c_str());
		return false;
	}

	info.socket = socket(AF_INET6, SOCK_STREAM, 0);
	if (info.socket == -1)
	{
		info.socket = 0;
		initFailClean("Create PIv6 sokcet failed.");
		return false;
	}

	int reuse_addr = 1;
	if (-1 == setsockopt(info.socket, SOL_SOCKET, SO_REUSEADDR, (void*)&(reuse_addr), sizeof(reuse_addr)))
	{
		initFailClean("Resuing IPv6 socket failed.");
		return false;
	}

	if (!nonblockedFd(info.socket))
	{
		initFailClean("Change IPv6 socket to non-blocked failed.");
		return false;
	}

	//-- Add Listening Socket to epoll
	struct epoll_event ev;
	ev.data.fd = info.socket;
	ev.events = EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLET | EPOLLRDHUP;

	if (-1 == epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, info.socket, &ev))
	{
		initFailClean("Add IPv6 listening socket to epoll failed.");
		return false;
	}

	if (-1 == bind(info.socket, (struct sockaddr *)(&serverAddr), sizeof(serverAddr))) 
	{
		initFailClean("Bind IPv6 listening socket failed.");
		return false;
	} 

	if (-1 == listen(info.socket, _backlog)) 
	{
		initFailClean("IPv6 listening failed.");
		return false;
	}

	return true;
}

void TCPEpollServer::exitCheck()
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

void TCPEpollServer::run()
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
			if (_epollEvents[i].data.fd == _ipv4.socket)
			{
				acceptIPv4Connection(_ipv4.socket, false);
			}
			else if (_epollEvents[i].data.fd == _sslIPv4.socket)
			{
				acceptIPv4Connection(_sslIPv4.socket, true);
			}
			else if (_epollEvents[i].data.fd == _ipv6.socket)
			{
				acceptIPv6Connection(_ipv6.socket, false);
			}
			else if (_epollEvents[i].data.fd == _sslIPv6.socket)
			{
				acceptIPv6Connection(_sslIPv6.socket, true);
			}
			else
				processEvent(_epollEvents[i]);
		}
	}
}

void TCPEpollServer::acceptIPv4Connection(int socket, bool ssl)
{
	while (true)
	{
		struct sockaddr_in	clientAddr;
		size_t addrlen = sizeof(struct sockaddr_in);
		int newSocket = accept(socket, (struct sockaddr *)(&clientAddr), (socklen_t *)&addrlen);
		if (newSocket == -1)
		{
			if (errno == EAGAIN || errno == EWOULDBLOCK)
				return;
			else
				continue;
		}
		if (_stopping)
		{
			close(newSocket);
			continue;
		}
		if (_enableIPWhiteList && !_ipWhiteList.exist(clientAddr.sin_addr.s_addr))
		{
			close(newSocket);
			LOG_ERROR("Refuse connection from %s:%d", IPV4ToString(clientAddr.sin_addr.s_addr).c_str(), (int)ntohs(clientAddr.sin_port));
			continue;
		}
		ConnectionInfoPtr ci(new ConnectionInfo(newSocket, ntohs(clientAddr.sin_port), IPV4ToString(clientAddr.sin_addr.s_addr), true, true));
		ci->_isSSL = ssl;
		newConnection(newSocket, ci);
	}
}

void TCPEpollServer::acceptIPv6Connection(int socket, bool ssl)
{
	while (true)
	{
		struct sockaddr_in6	clientAddr;
		size_t addrlen = sizeof(struct sockaddr_in6);
		int newSocket = accept(socket, (struct sockaddr *)(&clientAddr), (socklen_t *)&addrlen);
		if (newSocket == -1)
		{
			if (errno == EAGAIN || errno == EWOULDBLOCK)
				return;
			else
				continue;
		}
		if (_stopping)
		{
			close(newSocket);
			continue;
		}

		char buf[INET6_ADDRSTRLEN + 4];
		const char *rev = inet_ntop(AF_INET6, &(clientAddr.sin6_addr), buf, sizeof(buf));
		if (rev == NULL)
		{
			close(newSocket);
			char hex[32 + 1];
			hexlify(hex, &(clientAddr.sin6_addr), 16);
			LOG_ERROR("Format IPv6 address for socket %d failed. clientAddr.sin6_addr: %s", newSocket, hex);
			continue;
		}
		if (_enableIPWhiteList && !_ipWhiteList.exist(clientAddr.sin6_addr))
		{
			close(newSocket);
			LOG_ERROR("Refuse connection from [%s]:%d", buf, (int)ntohs(clientAddr.sin6_port));
			continue;
		}
		ConnectionInfoPtr ci(new ConnectionInfo(newSocket, ntohs(clientAddr.sin6_port), buf, false, true));
		ci->_isSSL = ssl;
		newConnection(newSocket, ci);
	}
}

void TCPEpollServer::newConnection(int newSocket, ConnectionInfoPtr ci)
{
	if (_connectionCount >= _maxConnections)
	{
		close(newSocket);
		LOG_ERROR("New connection is closed cause by max connection limitation caught. Socket: %d, address: %s:%d. Current connections count: %d, Max connections: %d",
			newSocket, ci->ip.c_str(), ci->port, _connectionCount.load(), _maxConnections);
		return;
	}
	else
		_connectionCount++;

	//-- non-blocked
	if (!nonblockedFd(newSocket))
	{
		close(newSocket);
		_connectionCount--;
		LOG_ERROR("Change new accepted fd to non-blocked failed. Socket: %d, address: %s:%d",
			newSocket, ci->ip.c_str(), ci->port);
		return;
	}

	/*int flag = 1;
	if (-1 == setsockopt(ci->socket, IPPROTO_TCP, TCP_NODELAY, (void*)&flag, sizeof(flag)))
	{
		close(newSocket);
		_connectionCount--;
		LOG_ERROR("TCP-Nodelay: disable Nagle failed. Socket: %d, address: %s:%d",
			newSocket, ci->ip.c_str(), ci->port);
		return;
	}*/

	//-- Server Connection
	int idx = newSocket % FPNN_SEND_QUEUE_MUTEX_COUNT;
	TCPServerConnection* connection = new(std::nothrow) TCPServerConnection(_epoll_fd, &_sendQueueMutex[idx], _ioBufferChunkSize, ci);
	if (!connection)
	{
		close(newSocket);
		_connectionCount--;
		LOG_ERROR("Alloc TCPServerConnection for new connection failed. Socket: %d, address: %s:%d", newSocket, ci->ip.c_str(), ci->port);
		return;
	}

	//-- Request Package
	RequestPackage* requestPackage = new(std::nothrow) RequestPackage(IOEventType::Connected, connection->_connectionInfo, connection);
	if (!requestPackage)
	{
		cleanForNewConnectionError(connection, NULL, "Alloc Event package for new connection failed.", false);
		return;
	}
	
	if (!_connectionMap.insert(newSocket, connection))
	{
		cleanForNewConnectionError(connection, requestPackage, "Insert TCPServerConnection to cache failed.", false);
		return;
	}

	if (_workerPool->priorWakeUp(requestPackage) == false) //-- for internal cmd canbe prior executed, connecting event must priored.
	{
		cleanForNewConnectionError(connection, requestPackage, "Worker pool wake up for new connection event failed. Server is exiting.", true);
		return;
	}
}

void TCPEpollServer::cleanForNewConnectionError(TCPServerConnection* connection, RequestPackage* requestPackage, const char* log_info, bool connectionInCache)
{
	if (connectionInCache)
		takeConnection(connection->_connectionInfo->socket);
	
	close(connection->_connectionInfo->socket);
	_connectionCount--;
	LOG_ERROR("%s %s",log_info, connection->_connectionInfo->str().c_str());

	if (requestPackage)
		delete requestPackage;

	delete connection;
}

void TCPEpollServer::cleanForExistConnectionError(TCPServerConnection* connection, RequestPackage* requestPackage, const char* log_info)
{
	LOG_ERROR("%s %s",log_info, connection->_connectionInfo->str().c_str());

	if (requestPackage)
		delete requestPackage;

	TCPServerConnectionPtr autoRelease(connection);
	_connectionCount--;
	_reclaimer->reclaim(autoRelease);
}

void TCPEpollServer::processEvent(struct epoll_event & event)
{
	int socket = event.data.fd;

	int errorCode = FPNN_EC_OK;

	TCPServerConnection* connection;
	RequestPackage* requestPackage;
	//-- Please care the order.
	if (event.events & EPOLLRDHUP || event.events & EPOLLHUP)
	{
		//-- close event processor
		connection = takeConnection(socket);
		if (connection == NULL)
			return;

		connection->exitEpoll();
		errorCode = FPNN_EC_CORE_CONNECTION_CLOSED;
		requestPackage = new(std::nothrow) RequestPackage(IOEventType::Closed, connection->_connectionInfo, connection);
		if (!requestPackage)
		{
			cleanForExistConnectionError(connection, NULL, "Alloc Event package for closing connection failed.");
			return;
		}
	}
	else if (event.events & EPOLLERR)
	{
		//-- error event processor
		connection = takeConnection(socket);
		if (connection == NULL)
			return;

		connection->exitEpoll();
		errorCode = FPNN_EC_CORE_UNKNOWN_ERROR;
		requestPackage = new(std::nothrow) RequestPackage(IOEventType::Error, connection->_connectionInfo, connection);
		if (!requestPackage)
		{
			cleanForExistConnectionError(connection, NULL, "Alloc Event package for connection error failed.");
			return;
		}
	}

	if (errorCode != FPNN_EC_OK)
	{
		clearConnectionQuestCallbacks(connection, errorCode);
		if (_workerPool->forceWakeUp(requestPackage) == false)
			cleanForExistConnectionError(connection, requestPackage, "Worker pool wake up for error or close event failed. Server is exiting.");

		return;
	}
	
	TCPServerConnection* conn = (TCPServerConnection*)_connectionMap.signConnection(socket, event.events);
	if (conn)
		_ioPool->wakeUp(conn);
}

void TCPEpollServer::clearConnectionQuestCallbacks(TCPServerConnection* connection, int errorCode)
{
	for (auto callbackPair: connection->_callbackMap)
	{
		BasicAnswerCallback* callback = callbackPair.second;
		if (callback->syncedCallback())		//-- check first, then fill result.
			callback->fillResult(NULL,  errorCode);
		else
		{
			if (_answerCallbackPool)
			{
				callback->fillResult(NULL, errorCode);

				BasicAnswerCallbackPtr task(callback);
				_answerCallbackPool->wakeUp(task);
			}
			else
			{
				delete callback;
				LOG_ERROR("CallbackMap of server connection is enabled, but process answers is disabled. Answer will be dropped.");
			}
		}
	}
	// connection->_callbackMap.clear(); //-- If necessary.
}

void TCPEpollServer::stop()
{
	if(!_running) return;

	LOG_INFO("Server will go to stop!");

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

	LOG_INFO("Server is stopping ...");

	_stopSignalNotified = true;
}

void TCPEpollServer::clean()
{
	_ipv4.close();
	_ipv6.close();
	_sslIPv4.close();
	_sslIPv6.close();

	while (!_stopSignalNotified)
		usleep(20000);

	close(_closeNotifyFds[1]);
	close(_closeNotifyFds[0]);
	_closeNotifyFds[0] = 0;
	_closeNotifyFds[1] = 0;
	
	sendCloseEvent();

	_connectionMap.waitForEmpty();
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
	// force release business processor
	_serverMasterProcessor->setQuestProcessor(nullptr);
}

void TCPEpollServer::sendCloseEvent()
{
	std::set<int> fdSet;
	_connectionMap.getAllSocket(fdSet);
	
	for (int socket: fdSet)
	{
		struct epoll_event	ev;
		ev.data.fd = socket;
		ev.events = EPOLLHUP | EPOLLRDHUP;
		processEvent(ev);
	}
}

void TCPEpollServer::sendData(int socket, uint64_t token, std::string* data)
{
	if (!_connectionMap.sendData(socket, token, data))
	{
		delete data;
		LOG_WARN("Data not send at socket %d, address: %s. socket maybe closed.", socket, NetworkUtil::getPeerName(socket).c_str());
	}
}

void TCPEpollServer::dealAnswer(int socket, FPAnswerPtr answer)
{
	BasicAnswerCallback* callback = _connectionMap.takeCallback(socket, answer->seqNumLE());
	if (!callback)
	{
		LOG_WARN("Received error answer seq is %u at socket %d, address: %s, %s", answer->seqNumLE(), socket, NetworkUtil::getPeerName(socket).c_str(), answer->json().c_str());
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
		LOG_ERROR("Server received an answer, but process answers is disabled. Answer will be dropped. socket: %d, address: %s", socket, NetworkUtil::getPeerName(socket).c_str());
	}
}

void TCPEpollServer::closeConnection(const ConnectionInfo* ci)
{
	TCPServerConnection* connection = takeConnection(ci);
	if (connection == NULL)
		return;

	connection->exitEpoll();
	RequestPackage* requestPackage = new(std::nothrow) RequestPackage(IOEventType::Closed, connection->_connectionInfo, connection);
	if (!requestPackage)
	{
		cleanForExistConnectionError(connection, NULL, "Alloc Event package for closing connection failed.");
		return;
	}

	clearConnectionQuestCallbacks(connection, FPNN_EC_CORE_CONNECTION_CLOSED);
	if (_workerPool->forceWakeUp(requestPackage) == false)
		cleanForExistConnectionError(connection, requestPackage, "Worker pool wake up for error or close event failed. Server is exiting.");
}

void TCPEpollServer::closeConnectionAfterSent(const ConnectionInfo* ci)
{
	_connectionMap.closeAfterSent(ci);
}

void TCPEpollServer::checkTimeout()
{
	if (_running)
	{
		if (_checkQuestTimeout)
		{
			//-- Quest Timeout: process as milliseconds
			int64_t current = slack_real_msec();
			std::list<std::map<uint32_t, BasicAnswerCallback*> > timeouted;

			_connectionMap.extractTimeoutedCallback(current, timeouted);
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

		if (_timeoutIdle)
		{
			//-- Connection idle Timeout: process as seconds, not milliseconds.
			int64_t threshold = slack_real_sec() - _timeoutIdle / 1000;
			std::list<BasicConnection*> timeouted;

			_connectionMap.extractTimeoutedConnections(threshold, timeouted);
			for (BasicConnection* conn: timeouted)
			{
				TCPServerConnection* connection = (TCPServerConnection*)conn;
				
				LOG_INFO("[Idle Timeout] Connection will be closed by server. %s", connection->_connectionInfo->str().c_str());

				clearConnectionQuestCallbacks(connection, FPNN_EC_CORE_CONNECTION_CLOSED);
				RequestPackage* requestPackage = new(std::nothrow) RequestPackage(IOEventType::Closed, connection->_connectionInfo, connection);
				if (!requestPackage)
				{
					cleanForExistConnectionError(connection, NULL, "Alloc Event package for closing idle connection failed.");
					continue;
				}
				if (_workerPool->forceWakeUp(requestPackage) == false)
				{
					cleanForExistConnectionError(connection, requestPackage, "Worker pool wake up for idle connection failed. Server is exiting.");
				}
			}
		}
	}
}
