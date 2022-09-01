//#include "../Config.h"
#include "../ConnectionReclaimer.h"
#include "AutoRelease.h"
#include "RawClientIOWorker.h"
//#include "RawTCPClientIOWorker.h"
#include "RawTCPClient.h"

using namespace fpnn;

RawTCPClient::RawTCPClient(const std::string& host, int port, bool autoReconnect):
	RawClient(host, port, autoReconnect), _ioChunkSize(512)
{
	_rawDataProcessPool = new TaskThreadPool();
	_rawDataProcessPool->init(1, 0, 1, 1);
}

ConnectionInfoPtr RawTCPClient::perpareConnection(int socket)
{
	/*int flag = 1;
	if (-1 == setsockopt(socket, IPPROTO_TCP, TCP_NODELAY, (void*)&flag, sizeof(flag)))
	{
		::close(socket);
		LOG_ERROR("TCP-Nodelay: disable Nagle failed. Socket: %d", socket);
		return nullptr;
	}*/

	ConnectionInfoPtr newConnectionInfo;
	RawTCPClientConnection* connection = NULL;
	{
		std::unique_lock<std::mutex> lck(_mutex);
		newConnectionInfo.reset(new ConnectionInfo(socket, _connectionInfo->port, _connectionInfo->ip, _isIPv4, false));
		connection = new RawTCPClientConnection(shared_from_this(), _ioChunkSize, newConnectionInfo);
	}

	connected(connection);

	bool joined = ClientEngine::nakedInstance()->joinEpoll(connection);
	if (!joined)
	{
		LOG_ERROR("Raw TCP Client join epoll failed after TCP connected event. %s", connection->_connectionInfo->str().c_str());
		errorAndWillBeClosed(connection);

		return nullptr;
	}
	else
		return newConnectionInfo;
}

int RawTCPClient::connectIPv4Address(ConnectionInfoPtr currConnInfo)
{
	struct sockaddr_in serverAddr;
	memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = inet_addr(currConnInfo->ip.c_str()); 
	serverAddr.sin_port = htons(currConnInfo->port);

	if (serverAddr.sin_addr.s_addr == INADDR_NONE)
		return 0;

	int socketfd = ::socket(AF_INET, SOCK_STREAM, 0);
	if (socketfd < 0)
		return 0;

	if (::connect(socketfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) != 0)
	{
		::close(socketfd);
		return 0;
	}

	return socketfd;
}
int RawTCPClient::connectIPv6Address(ConnectionInfoPtr currConnInfo)
{
	struct sockaddr_in6 serverAddr;
	memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin6_family = AF_INET6;  
	serverAddr.sin6_port = htons(currConnInfo->port);

	if (inet_pton(AF_INET6, currConnInfo->ip.c_str(), &serverAddr.sin6_addr) != 1)
		return 0;

	int socketfd = ::socket(AF_INET6, SOCK_STREAM, 0);
	if (socketfd < 0)
		return 0;

	if (::connect(socketfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) != 0)
	{
		::close(socketfd);
		return 0;
	}

	return socketfd;
}

bool RawTCPClient::connect()
{
	if (_connected)
		return true;

	ConnectionInfoPtr currConnInfo;
	{
		std::unique_lock<std::mutex> lck(_mutex);
		while (_connStatus == ConnStatus::Connecting)
			_condition.wait(lck);

		if (_connStatus == ConnStatus::Connected)
			return true;

		currConnInfo = _connectionInfo;

		_connected = false;
		_connStatus = ConnStatus::Connecting;
	}

	RawTCPClient* self = this;

	CannelableFinallyGuard errorGuard([self, currConnInfo](){
		std::unique_lock<std::mutex> lck(self->_mutex);
		if (currConnInfo.get() == self->_connectionInfo.get())
		{
			self->_connected = false;
			self->_connStatus = ConnStatus::NoConnected;
		}
		self->_condition.notify_all();
	});

	int socket = 0;
	if (_isIPv4)
		socket = connectIPv4Address(currConnInfo);
	else
		socket = connectIPv6Address(currConnInfo);

	if (socket == 0)
	{
		LOG_ERROR("Raw TCP client connect remote server %s failed.", currConnInfo->str().c_str());
		return false;
	}

	ConnectionInfoPtr newConnInfo = perpareConnection(socket);
	if (!newConnInfo)
		return false;

	errorGuard.cancel();
	{
		std::unique_lock<std::mutex> lck(_mutex);
		if (_connectionInfo.get() == currConnInfo.get())
		{
			_connectionInfo = newConnInfo;
			_connected = true;
			_connStatus = ConnStatus::Connected;
			_condition.notify_all();

			return true;
		}
	}

	LOG_ERROR("This codes (RawTCPClient::connect dupled) is impossible touched. This is just a safety inspection. If this ERROR triggered, please tell swxlion to fix it.");

	//-- dupled
	RawTCPClientConnection* conn = (RawTCPClientConnection*)_engine->takeConnection(newConnInfo.get());
	if (conn)
	{
		_engine->exitEpoll(conn);
		willClose(conn);
	}

	std::unique_lock<std::mutex> lck(_mutex);

	while (_connStatus == ConnStatus::Connecting)
		_condition.wait(lck);

	_condition.notify_all();
	if (_connStatus == ConnStatus::Connected)
		return true;

	return false;
}

bool RawTCPClient::sendData(std::string* data)
{
	if (!_connected)
	{
		if (!_autoReconnect)
			return false;

		if (!reconnect())
			return false;
	}

	ConnectionInfoPtr connInfo;
	{
		std::unique_lock<std::mutex> lck(_mutex);
		connInfo = _connectionInfo;
	}

	ClientEngine::nakedInstance()->sendData(connInfo->socket, connInfo->token, data);
	return true;
}

RawTCPClientPtr RawClient::createRawTCPClient(const std::string& host, int port, bool autoReconnect)
{
	return RawTCPClient::createClient(host, port, autoReconnect);
}
RawTCPClientPtr RawClient::createRawTCPClient(const std::string& endpoint, bool autoReconnect)
{
	return RawTCPClient::createClient(endpoint, autoReconnect);
}
