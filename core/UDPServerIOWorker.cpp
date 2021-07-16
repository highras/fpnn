#include "hex.h"
#include "jenkins.h"
#include "Config.h"
#include "UDPEpollServer.h"
#include "UDPServerIOWorker.h"

using namespace fpnn;

//=================================================================//
//- UDP Server Receiver
//=================================================================//
bool UDPServerReceiver::recvIPv4(int socket)
{
	udpRawData = NULL;
	requiredAddrLen = _addrlen;
	ssize_t readBytes = recvfrom(socket, _buffer, _UDPMaxDataLen, 0, (struct sockaddr *)&_addr, &requiredAddrLen);
	if (requiredAddrLen > _addrlen)
	{
		LOG_ERROR("UDP recvfrom IPv4 address buffer truncated. required %d, offered is %d.", (int)requiredAddrLen, (int)_addrlen);
		return true;
	}

	if (readBytes > 0)
	{
		port = ntohs(_addr.sin_port);
		ip = IPV4ToString(_addr.sin_addr.s_addr);
		
		udpRawData = new UDPRawBuffer((int)readBytes);
		memcpy(udpRawData->buffer, _buffer, (int)readBytes);

		isIPv4 = true;
		sockaddr = &_addr;
		return true;
	}
	else
	{
		if (readBytes == 0)
			return false;

		if (errno == 0 || errno == EINTR)
			return true;

		if (errno == EAGAIN || errno == EWOULDBLOCK)
			return false;

		return false;
	}
}

bool UDPServerReceiver::recvIPv6(int socket)
{
	udpRawData = NULL;
	requiredAddrLen = _addrlen6;
	ssize_t readBytes = recvfrom(socket, _buffer, _UDPMaxDataLen, 0, (struct sockaddr *)&_addr6, &requiredAddrLen);
	if (requiredAddrLen > _addrlen6)
	{
		LOG_ERROR("UDP recvfrom IPv6 address buffer truncated. required %d, offered is %d.", (int)requiredAddrLen, (int)_addrlen6);
		return true;
	}
	if (readBytes > 0)
	{
		char buf[INET6_ADDRSTRLEN + 4];
		const char *rev = inet_ntop(AF_INET6, &(_addr6.sin6_addr), buf, sizeof(buf));
		if (rev == NULL)
		{
			char hex[32 + 1];
			hexlify(hex, &(_addr6.sin6_addr), 16);
			LOG_ERROR("Format IPv6 address for UDP socket: %d, address: %s failed. clientAddr.sin6_addr: %s", socket, NetworkUtil::getPeerName(socket).c_str(), hex);
			return true;
		}

		port = ntohs(_addr6.sin6_port);
		ip = buf;

		udpRawData = new UDPRawBuffer((int)readBytes);
		memcpy(udpRawData->buffer, _buffer, (int)readBytes);

		isIPv4 = false;
		sockaddr = &_addr6;
		return true;
	}
	else
	{
		if (readBytes == 0)
			return false;

		if (errno == 0 || errno == EINTR)
			return true;

		if (errno == EAGAIN || errno == EWOULDBLOCK)
			return false;

		return false;
	}
}

//=================================================================//
//- UDP Server Connection
//=================================================================//
bool UDPServerConnection::waitForAllEvents()
{
	struct epoll_event	ev;

	ev.data.fd = _connectionInfo->socket;
	ev.events = EPOLLIN | EPOLLOUT | EPOLLERR | EPOLLHUP | EPOLLET | EPOLLRDHUP | EPOLLONESHOT;

	if (epoll_ctl(epollfd, EPOLL_CTL_MOD, _connectionInfo->socket, &ev) != 0)
	{
		if (errno == ENOENT)
			return false;
		
		LOG_INFO("UDP connection wait socket event failed. Socket: %d, address: %s, errno: %d", _connectionInfo->socket, NetworkUtil::getPeerName(_connectionInfo->socket).c_str(), errno);
		return false;
	}
	else
		return true;
}

int UDPServerConnection::send(bool& needWaitSendEvent, std::string* data)
{
	//_activeTime = slack_real_sec();
	//_ioBuffer.sendData(needWaitSendEvent, data);
	LOG_ERROR("Unused interface/method for UDPServerConnection. If this ERROR triggered, please tell swxlion to fix it.");
	//-- Unused
	return -1;
}

void UDPServerConnection::appendQuest(std::list<FPQuestPtr>& questList)
{
	{
		std::unique_lock<std::mutex> lck(_mutex);
		_questCache.insert(_questCache.end(), questList.begin(), questList.end());
	}
	
	questList.clear();
}

void UDPServerConnection::parseCachedRawData(bool& requireClose, bool& requireConnectedEvent)
{
	requireClose = false;
	requireConnectedEvent = false;

	UDPIOReceivedResult sectionResult;

	for (auto it = _rawData.begin(); it != _rawData.end(); it++)
	{
		sectionResult.reset();
		UDPRawBuffer* rawData = *it;
		if (_ioBuffer.parseReceivedData(rawData->buffer, rawData->len, sectionResult))
		{
			if (sectionResult.requireClose)
			{
				for (; it != _rawData.end(); it++)
					delete *it;

				_rawData.clear();
				requireClose = true;
				return;
			}

			_questCache.insert(_questCache.end(), sectionResult.questList.begin(), sectionResult.questList.end());
			if (sectionResult.answerList.size() > 0)
			{
				LOG_ERROR("Invalid answers, count %d from uninited UDP connection: %s",
					(int)sectionResult.answerList.size(), _connectionInfo->str().c_str());
			}
		}
		else
		{
			if (sectionResult.requireClose)
			{
				for (; it != _rawData.end(); it++)
					delete *it;

				_rawData.clear();
				requireClose = true;
				return;
			}
		}

		delete rawData;
	}

	_rawData.clear();

	if (_questCache.size() > 0)
	{
		requireConnectedEvent = true;
		_connectedEventStatus = UDPConnectionEventStatus::Calling;
	}

	return;
}

BasicAnswerCallback* UDPServerConnection::takeCallback(uint32_t seqNum)
{
	std::unique_lock<std::mutex> lck(_mutex);

	auto iter = _callbackMap.find(seqNum);
  	if (iter != _callbackMap.end())
  	{
  		BasicAnswerCallback* cb = iter->second;
  		_callbackMap.erase(seqNum);
  		return cb;
  	}
  	return NULL;
}

void UDPServerConnection::initRecvFinishCheck(std::list<FPQuestPtr>& deliverableQuests, bool& requireConnectedEvent)
{
	std::unique_lock<std::mutex> lck(_mutex);
	switch (_connectedEventStatus)
	{
		case UDPConnectionEventStatus::UnCalling:
			_connectedEventStatus = UDPConnectionEventStatus::Calling;
			requireConnectedEvent = true;
			return;

		case UDPConnectionEventStatus::Calling:
			requireConnectedEvent = false;
			return;

		case UDPConnectionEventStatus::Called:
			requireConnectedEvent = false;
			_questCache.swap(deliverableQuests);
			return;
	}
}

void UDPServerConnection::extractTimeoutedCallback(int64_t now, std::unordered_map<uint32_t, BasicAnswerCallback*>& timeoutedCallbacks)
{
	std::list<uint32_t> expiredSeqNum;

	std::unique_lock<std::mutex> lck(_mutex);

	for (auto& cbPair: _callbackMap)
	{
		if (cbPair.second->expiredTime() <= now)
		{
			expiredSeqNum.push_back(cbPair.first);
			timeoutedCallbacks[cbPair.first] = cbPair.second;
		}
	}

	for (uint32_t seqNum: expiredSeqNum)
		_callbackMap.erase(seqNum);
}

bool UDPServerConnection::sendQuestWithBasicAnswerCallback(FPQuestPtr quest, BasicAnswerCallback* callback, int timeout, bool discardable)
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
	catch (const FpnnError& ex) {
		LOG_ERROR("Quest Raw Exception:(%d)%s", ex.code(), ex.what());
		return false;
	}
	catch (...)
	{
		LOG_ERROR("Quest Raw Exception.");
		return false;
	}

	{
		std::unique_lock<std::mutex> lck(_mutex);
		if (callback)
		{
			uint32_t seqNum = quest->seqNumLE();
			callback->updateExpiredTime(slack_real_msec() + timeout);
			_callbackMap[seqNum] = callback;
		}
	}
	
	bool needWaitSendEvent = false;
	sendData(needWaitSendEvent, raw, slack_real_msec() + timeout, discardable);
	if (needWaitSendEvent)
		waitForAllEvents();

	return true;
}

void UDPServerConnection::sendCachedData(bool& needWaitSendEvent, bool socketReady)
{
	bool blockByFlowControl = false;
	_ioBuffer.sendCachedData(needWaitSendEvent, blockByFlowControl, socketReady);
	//_activeTime = slack_real_sec();
}
void UDPServerConnection::sendData(bool& needWaitSendEvent, std::string* data, int64_t expiredMS, bool discardable)
{
	bool blockByFlowControl = false;
	_ioBuffer.sendData(needWaitSendEvent, blockByFlowControl, data, expiredMS, discardable);
	//_activeTime = slack_real_sec();
}

bool UDPServerConnection::recvData(std::list<FPQuestPtr>& questList, std::list<FPAnswerPtr>& answerList)
{
	bool status = _ioBuffer.recvData();
	questList.swap(_ioBuffer.getReceivedQuestList());
	answerList.swap(_ioBuffer.getReceivedAnswerList());

	//if (questList.size() || answerList.size())
	//	_activeTime = slack_real_sec();

	return status;
}

//=================================================================//
//- UDP Server Connection Map
//=================================================================//
void UDPServerConnectionMap::removeFromScheduleMap(UDPServerConnection* conn)
{
	auto it = _scheduleMap.find(conn->sendingTurn);
	if (it != _scheduleMap.end())
	{
		it->second.erase(conn);

		if (it->second.empty())
			_scheduleMap.erase(it);
	}
}

void UDPServerConnectionMap::changeSchedule(UDPServerConnection* conn, int64_t sendingTurn)
{
	removeFromScheduleMap(conn);
	_scheduleMap[sendingTurn].insert(conn);
	conn->sendingTurn = sendingTurn;
}

/*
void UDPServerConnectionMap::insert(int socket, UDPServerConnection* conn)
{
	std::unique_lock<std::mutex> lck(_mutex);
	_connections.emplace(socket, conn);

	int64_t sendingTurn = slack_real_msec() + Config::UDP::_heartbeat_interval_seconds * 1000;
	conn->sendingTurn = sendingTurn;

	_scheduleMap[sendingTurn].insert(conn);
}*/

void UDPServerConnectionMap::insert(const std::unordered_map<std::string, UDPServerConnection*>& cache)
{
	int64_t sendingTurn = slack_real_msec() + Config::UDP::_heartbeat_interval_seconds * 1000;

	std::unique_lock<std::mutex> lck(_mutex);
	for (auto& pp: cache)
	{
		_connections.emplace(pp.second->socket(), pp.second);
		pp.second->sendingTurn = sendingTurn;

		_scheduleMap[sendingTurn].insert(pp.second);
	}
}

UDPServerConnection* UDPServerConnectionMap::remove(int socket)
{
	std::unique_lock<std::mutex> lck(_mutex);

	auto it = _connections.find(socket);
	if (it != _connections.end())
	{
		UDPServerConnection* connection = it->second;
		_connections.erase(it);

		removeFromScheduleMap(connection);

		return connection;
	}
	else
		return NULL;
}

UDPServerConnection* UDPServerConnectionMap::signConnection(int socket, uint32_t events)
{
	std::unique_lock<std::mutex> lck(_mutex);
	auto it = _connections.find(socket);
	if (it != _connections.end())
	{
		if (events & EPOLLIN)
			it->second->setNeedRecvFlag();
		if (events & EPOLLOUT)
			it->second->setNeedSendFlag();

		it->second->_refCount++;
		return it->second;
	}
	else
		return NULL;
}

void UDPServerConnectionMap::markAllConnectionsActiveCloseSignal()
{
	int64_t sendingTurn = slack_real_msec();

	std::unique_lock<std::mutex> lck(_mutex);
	for (auto& pp: _connections)
	{
		pp.second->markActiveCloseSignal();
		changeSchedule(pp.second, sendingTurn);
	}
}

bool UDPServerConnectionMap::markConnectionActiveCloseSignal(int socket)
{
	UDPServerConnection* conn;
	{
		std::unique_lock<std::mutex> lck(_mutex);

		auto it = _connections.find(socket);
		if (it != _connections.end())
		{
			conn = it->second;
			conn->_refCount++;
		}
		else
			return false;
	}

	conn->markActiveCloseSignal();

	bool needWaitSendEvent = false;
	conn->sendCachedData(needWaitSendEvent, false);
	if (needWaitSendEvent)
		conn->waitForAllEvents();

	conn->_refCount--;
	return true;
}

void UDPServerConnectionMap::removeAllConnections(std::list<UDPServerConnection*>& connections)
{
	std::unique_lock<std::mutex> lck(_mutex);
	for (auto& pp: _connections)
		connections.push_back(pp.second);

	_connections.clear();
	_scheduleMap.clear();
}

void UDPServerConnectionMap::extractInvalidConnectionsAndCallbcks(std::list<UDPServerConnection*>& invalidConnections,
			std::unordered_map<uint32_t, BasicAnswerCallback*>& timeoutedCallbacks)
{
	std::list<int> invalidSockets;
	std::list<UDPServerConnection*> invalidConns;

	std::unique_lock<std::mutex> lck(_mutex);

	int64_t current = slack_real_msec();

	for (auto& pp: _connections)
	{
		if (pp.second->invalid() || pp.second->isRequireClose())
		{
			invalidSockets.push_back(pp.first);
			invalidConns.push_back(pp.second);
			invalidConnections.push_back(pp.second);
		}
		else
			pp.second->extractTimeoutedCallback(current, timeoutedCallbacks);
	}

	for (int socket: invalidSockets)
		_connections.erase(socket);

	for (auto conn: invalidConns)
		removeFromScheduleMap(conn);
}

void UDPServerConnectionMap::putRescheduleMapBack(std::unordered_map<int64_t, std::unordered_set<UDPServerConnection*>>& rescheduleMap)
{
	for (auto& pp: rescheduleMap)
	{
		std::unordered_set<UDPServerConnection*>* connSet = NULL;
		for (auto conn: pp.second)
		{
			if (_connections.find(conn->socket()) != _connections.end())
			{
				if (!connSet)
					connSet = &(_scheduleMap[pp.first]);

				if (conn->sendingTurn != pp.first)
					removeFromScheduleMap(conn);

				conn->sendingTurn = pp.first;
				connSet->insert(conn);
				conn->waitForAllEvents();
			}

			conn->_refCount--;
		}
	}
}

void UDPServerConnectionMap::periodSending()
{
	std::unordered_set<UDPServerConnection*> sendingConns;
	std::unordered_map<int64_t, std::unordered_set<UDPServerConnection*>> rescheduleMap;

	while (true)
	{
		int64_t oldTurn;
		int64_t nextTurn;
		{
			//-- check & fetch
			std::unique_lock<std::mutex> lck(_mutex);
			if (_scheduleMap.empty())
			{
				putRescheduleMapBack(rescheduleMap);
				return;
			}

			//-- fetch
			int64_t current = slack_real_msec();
			auto it = _scheduleMap.begin();
			if (it->first > current)
			{
				putRescheduleMapBack(rescheduleMap);
				return;
			}

			oldTurn = it->first;
			sendingConns.swap(it->second);
			_scheduleMap.erase(it);

			nextTurn = current + Config::UDP::_heartbeat_interval_seconds * 1000;
			std::unordered_set<UDPServerConnection*>& nextSet = _scheduleMap[nextTurn];
			for (auto conn: sendingConns)
			{
				conn->_refCount++;

				if (conn->sendingTurn != oldTurn && conn->sendingTurn != nextTurn)
					removeFromScheduleMap(conn);

				conn->sendingTurn = nextTurn;
				nextSet.insert(conn);
			}
		}

		//-- sending
		for (auto conn: sendingConns)
		{
			bool needWaitSendEvent = false;
			conn->sendCachedData(needWaitSendEvent, false);
			if (needWaitSendEvent)
			{
				rescheduleMap[oldTurn].insert(conn);
				conn->_refCount++;
			}

			conn->_refCount--;
		}
	}
}

bool UDPServerConnectionMap::sendData(int socket, uint64_t token, std::string* data, int64_t expiredMS, bool discardable)
{
	UDPServerConnection* conn;

	{
		std::unique_lock<std::mutex> lck(_mutex);
		auto it = _connections.find(socket);
		if (it != _connections.end() && it->second->_connectionInfo->token == token)
		{
			conn = it->second;
			conn->_refCount++;
		}
		else
			return false;
	}
	
	bool needWaitSendEvent = false;
	conn->sendData(needWaitSendEvent, data, expiredMS, discardable);
	if (needWaitSendEvent)
		conn->waitForAllEvents();
	
	conn->_refCount--;
	return true;
}

bool UDPServerConnectionMap::sendQuestWithBasicAnswerCallback(int socket, uint64_t token, FPQuestPtr quest, BasicAnswerCallback* callback, int timeout, bool discardable)
{
	UDPServerConnection* conn;

	{
		std::unique_lock<std::mutex> lck(_mutex);
		auto it = _connections.find(socket);
		if (it != _connections.end() && it->second->_connectionInfo->token == token)
		{
			conn = it->second;
			conn->_refCount++;
		}
		else
			return false;
	}

	bool status = conn->sendQuestWithBasicAnswerCallback(quest, callback, timeout, discardable);	
	conn->_refCount--;
	return status;
}

//=================================================================//
//- UDP Server IO Worker
//=================================================================//
void UDPServerIOWorker::read(UDPServerConnection * connection)
{
	if (!connection->getRecvToken())
		return;
	
	std::list<FPQuestPtr> questList;
	std::list<FPAnswerPtr> answerList;
	
	bool goon = true;
	while (goon && connection->isRequireClose() == false)
	{
		goon = connection->recvData(questList, answerList);

		for (auto& answer: answerList)
			_server->deliverAnswer(connection, answer);
		
		for (auto& quest: questList)
			_server->deliverQuest(connection, quest);

		questList.clear();
		answerList.clear();
	}

	connection->returnRecvToken();
}

void UDPServerIOWorker::initRead(UDPServerConnection * connection)
{
	if (!connection->getRecvToken())
		return;
	
	std::list<FPQuestPtr> questList;
	std::list<FPAnswerPtr> answerList;
	
	bool goon = true;
	while (goon && connection->isRequireClose() == false)
	{
		goon = connection->recvData(questList, answerList);

		for (auto& answer: answerList)
			_server->deliverAnswer(connection, answer);
		
		connection->appendQuest(questList);
	}

	connection->returnRecvToken();

	if (connection->isRequireClose() == false)
	{
		questList.clear();
		bool requireConnectedEvent;
		connection->initRecvFinishCheck(questList, requireConnectedEvent);

		if (requireConnectedEvent)
		{
			if (_server->deliverSessionEvent(connection, UDPSessionEventType::Connected))
				connection->_refCount++;
			else
				connection->requireClose();

			return;
		}

		for (auto& quest: questList)
			_server->deliverQuest(connection, quest);
	}
}

void UDPServerIOWorker::initConnection(UDPServerConnection * conn)
{
	bool requireClose;
	bool requireConnectedEvent;
	conn->parseCachedRawData(requireClose, requireConnectedEvent);

	if (requireClose)
	{
		conn->_refCount--;
		_server->terminateConnection(conn->socket(), UDPSessionEventType::Closed, FPNN_EC_CORE_CONNECTION_CLOSED);
		return;
	}

	conn->firstSchedule = false;
	_server->joinEpoll(conn->socket());

	if (requireConnectedEvent)
	{
		if (_server->deliverSessionEvent(conn, UDPSessionEventType::Connected))
			return;
		else
		{
			conn->_refCount--;
			_server->terminateConnection(conn->socket(), UDPSessionEventType::Closed, FPNN_EC_CORE_INVALID_CONNECTION);
			return;
		}
	}
	else
	{
		conn->_refCount--;
	}
}

void UDPServerIOWorker::run(UDPServerConnection * conn)
{
	if (conn->firstSchedule)
	{
		initConnection(conn);
		return;
	}

	if (conn->_needRecv)
	{
		if (conn->connectedEventCalled)
			read(conn);
		else
			initRead(conn);

		conn->_needRecv = false;
		conn->_needSend = true;		//-- receive UDP 后，强制调用 send 检查是否有数据需要发送（大概率有ARQ应答包需要发送）。
	}

	bool needWaitSendEvent = false;
	if (conn->_needSend && conn->connectedEventCalled && conn->isRequireClose() == false)
	{
		conn->sendCachedData(needWaitSendEvent, true);
		conn->_needSend = false;
	}

	if (conn->isRequireClose())
	{
		conn->_refCount--;
		_server->terminateConnection(conn->socket(), UDPSessionEventType::Closed, FPNN_EC_CORE_CONNECTION_CLOSED);
		return;
	}

	{
		if (needWaitSendEvent)
		{
			if (_server->waitForAllEvents(conn->socket()))
			{
				conn->_refCount--;
				return;
			}
		}
		else
		{
			if (_server->waitForRecvEvent(conn->socket()))
			{
				conn->_refCount--;
				return;
			}
		}

		LOG_INFO("UDP server connection wait event failed. System memory maybe run out. Connection maybe unusable. %s", conn->_connectionInfo->str().c_str());
		
		conn->_refCount--;
		_server->terminateConnection(conn->socket(), UDPSessionEventType::Closed, FPNN_EC_CORE_CONNECTION_CLOSED);
	}	
}
