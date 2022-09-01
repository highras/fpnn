#include "AutoRelease.h"
#include "../ConnectionReclaimer.h"
#include "RawClientIOWorker.h"
//#include "RawUDPClientIOWorker.h"
#include "RawUDPClient.h"

using namespace fpnn;

RawUDPClient::RawUDPClient(const std::string& host, int port, bool autoReconnect): RawClient(host, port, autoReconnect)
{
}

bool RawUDPClient::perpareConnection(ConnectionInfoPtr currConnInfo)
{
	RawUDPClientConnection* connection = new RawUDPClientConnection(shared_from_this(), currConnInfo);

	connected(connection);

	bool joined = ClientEngine::nakedInstance()->joinEpoll(connection);
	if (!joined)
	{
		LOG_ERROR("Raw UDP Client join epoll failed after UDP connected event. %s", currConnInfo->str().c_str());
		errorAndWillBeClosed(connection);
		return false;
	}
	
	return true;
}

int RawUDPClient::connectIPv4Address(ConnectionInfoPtr currConnInfo)
{
	int socketfd = ::socket(AF_INET, SOCK_DGRAM, 0);
	if (socketfd < 0)
		return 0;

	size_t addrlen = sizeof(struct sockaddr_in);
	struct sockaddr_in* serverAddr = (struct sockaddr_in*)malloc(addrlen);

	memset(serverAddr, 0, addrlen);
	serverAddr->sin_family = AF_INET;
	serverAddr->sin_addr.s_addr = inet_addr(currConnInfo->ip.c_str()); 
	serverAddr->sin_port = htons(currConnInfo->port);

	if (serverAddr->sin_addr.s_addr == INADDR_NONE)
	{
		::close(socketfd);
		free(serverAddr);
		return 0;
	}

	if (::connect(socketfd, (struct sockaddr *)serverAddr, addrlen) != 0)
	{
		::close(socketfd);
		free(serverAddr);
		return 0;
	}

	currConnInfo->changeToUDP(socketfd, (uint8_t*)serverAddr);

	return socketfd;
}
int RawUDPClient::connectIPv6Address(ConnectionInfoPtr currConnInfo)
{
	int socketfd = ::socket(AF_INET6, SOCK_DGRAM, 0);
	if (socketfd < 0)
		return 0;

	size_t addrlen = sizeof(struct sockaddr_in6);
	struct sockaddr_in6* serverAddr = (struct sockaddr_in6*)malloc(addrlen);

	memset(serverAddr, 0, addrlen);
	serverAddr->sin6_family = AF_INET6;  
	serverAddr->sin6_port = htons(currConnInfo->port);

	if (inet_pton(AF_INET6, currConnInfo->ip.c_str(), &serverAddr->sin6_addr) != 1)
	{
		::close(socketfd);
		free(serverAddr);
		return 0;
	}

	if (::connect(socketfd, (struct sockaddr *)serverAddr, addrlen) != 0)
	{
		::close(socketfd);
		free(serverAddr);
		return 0;
	}

	currConnInfo->changeToUDP(socketfd, (uint8_t*)serverAddr);

	return socketfd;
}

bool RawUDPClient::connect()
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

	RawUDPClient* self = this;

	CannelableFinallyGuard errorGuard([self, currConnInfo](){
		std::unique_lock<std::mutex> lck(self->_mutex);
		if (currConnInfo.get() == self->_connectionInfo.get())
		{
			if (self->_connectionInfo->socket)
			{
				ConnectionInfoPtr newConnectionInfo(new ConnectionInfo(0, self->_connectionInfo->port, self->_connectionInfo->ip, self->_isIPv4, false));
				newConnectionInfo->changeToUDP();
				self->_connectionInfo = newConnectionInfo;
			}

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
		LOG_ERROR("Raw UDP client connect remote server %s failed.", currConnInfo->str().c_str());
		return false;
	}

	if (!perpareConnection(currConnInfo))
		return false;

	errorGuard.cancel();
	{
		std::unique_lock<std::mutex> lck(_mutex);
		if (_connectionInfo.get() == currConnInfo.get())
		{
			_connected = true;
			_connStatus = ConnStatus::Connected;
			_condition.notify_all();

			return true;
		}
	}

	LOG_ERROR("This codes (RawUDPClient::connect dupled) is impossible touched. This is just a safety inspection. If this ERROR triggered, please tell swxlion to fix it.");

	//-- dupled
	RawUDPClientConnection* conn = (RawUDPClientConnection*)_engine->takeConnection(currConnInfo.get());
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

bool RawUDPClient::sendData(std::string* data)
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

void RawUDPClient::close()
{
	if (!_connected)
		return;

	ConnectionInfoPtr oldConnInfo;
	{
		std::unique_lock<std::mutex> lck(_mutex);
		while (_connStatus == ConnStatus::Connecting)
			_condition.wait(lck);

		if (_connStatus == ConnStatus::NoConnected)
			return;

		oldConnInfo = _connectionInfo;

		ConnectionInfoPtr newConnectionInfo(new ConnectionInfo(0, _connectionInfo->port, _connectionInfo->ip, _isIPv4, false));
		newConnectionInfo->changeToUDP();
		_connectionInfo = newConnectionInfo;
		_connected = false;
		_connStatus = ConnStatus::NoConnected;
	}

	BasicConnection* conn = _engine->takeConnection(oldConnInfo.get());
	if (conn == NULL)
		return;

	_engine->exitEpoll(conn);
	willClose(conn);
}

RawUDPClientPtr RawClient::createRawUDPClient(const std::string& host, int port, bool autoReconnect)
{
	return RawUDPClient::createClient(host, port, autoReconnect);
}
RawUDPClientPtr RawClient::createRawUDPClient(const std::string& endpoint, bool autoReconnect)
{
	return RawUDPClient::createClient(endpoint, autoReconnect);
}
