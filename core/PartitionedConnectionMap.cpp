#include "PartitionedConnectionMap.h"

namespace fpnn
{
	BasicAnswerCallback* ConnectionMap::takeCallback(int socket, uint32_t seqNum)
	{
		std::unique_lock<std::mutex> lck(_mutex);
		HashMap<int, BasicConnection*>::node_type* node = _connections.find(socket);
		if (node)
		{
			BasicConnection* connection = node->data;
			
			auto iter = connection->_callbackMap.find(seqNum);
  			if (iter != connection->_callbackMap.end())
  			{
  				BasicAnswerCallback* cb = iter->second;
  				connection->_callbackMap.erase(seqNum);
  				return cb;
  			}
  			return NULL;
		}
		return NULL;
	}

	void ConnectionMap::extractTimeoutedCallback(int64_t threshold, std::list<std::map<uint32_t, BasicAnswerCallback*> >& timeouted)
	{
		HashMap<int, BasicConnection*>::node_type* node = NULL;
		std::unique_lock<std::mutex> lck(_mutex);
		while ((node = _connections.next_node(node)))
		{
			BasicConnection* connection = node->data;

			std::map<uint32_t, BasicAnswerCallback*> basMap;
			timeouted.push_back(basMap);

			std::map<uint32_t, BasicAnswerCallback*>& currMap = timeouted.back();
			for (auto& cbPair: connection->_callbackMap)
			{
				if (cbPair.second->expiredTime() <= threshold)
					currMap[cbPair.first] = cbPair.second;
			}

			for (auto& bacPair: currMap)
				connection->_callbackMap.erase(bacPair.first);
		}
	}

	void ConnectionMap::extractTimeoutedConnections(int64_t threshold, std::list<BasicConnection*>& timeouted)
	{
		typedef HashMap<int, BasicConnection*>::node_type HashNode;

		HashNode* node = NULL;
		std::list<HashNode*> timeoutedNodes;
		std::unique_lock<std::mutex> lck(_mutex);
		while ((node = _connections.next_node(node)))
		{
			BasicConnection* connection = node->data;

			if (connection->_activeTime <= threshold)
			{
				timeoutedNodes.push_back(node);
				timeouted.push_back(connection);
			}
		}

		for (auto n: timeoutedNodes)
			_connections.remove_node(n);
	}

	void ConnectionMap::TCPClientKeepAlive(TCPClientSharedKeepAlivePingDatas& sharedPing, std::list<TCPClientConnection*>& invalidConnections)
	{
		typedef HashMap<int, BasicConnection*>::node_type HashNode;

		std::list<TCPClientKeepAliveTimeoutInfo> keepAliveList;

		//-- Step 1: pick invalid connections & requiring ping connections
		{
			bool isLost;
			int timeout;
			HashNode* node = NULL;
			std::list<HashNode*> invalidNodes;

			std::unique_lock<std::mutex> lck(_mutex);

			while ((node = _connections.next_node(node)))
			{
				BasicConnection* connection = node->data;
				if (connection->connectionType() == BasicConnection::TCPClientConnectionType)
				{
					TCPClientConnection* tcpClientConn = (TCPClientConnection*)connection;
					timeout = tcpClientConn->isRequireKeepAlive(isLost);
					if (isLost)
					{
						invalidConnections.push_back(tcpClientConn);
						invalidNodes.push_back(node);
					}
					else if (timeout > 0)
					{
						connection->_refCount++;
						keepAliveList.push_back(TCPClientKeepAliveTimeoutInfo{ tcpClientConn, timeout });
					}
				}
			}

			for (auto n: invalidNodes)
				_connections.remove_node(n);
		}

		//--  Step 2: send ping
		if (keepAliveList.size() > 0)
		{
			sharedPing.build();
			sendTCPClientKeepAlivePingQuest(sharedPing, keepAliveList);
		}
	}

	bool PartitionedConnectionMap::sendQuestWithBasicAnswerCallback(int socket, uint64_t token, FPQuestPtr quest, BasicAnswerCallback* callback, int timeout, bool discardableUDPQuest)
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
		catch (const std::exception& ex)
		{
			LOG_ERROR("Quest Raw Exception: %s", ex.what());
			return false;
		}
		catch (...)
		{
			LOG_ERROR("Quest Raw Exception.");
			return false;
		}

		uint32_t seqNum = quest->seqNumLE();

		if (callback)
			callback->updateExpiredTime(slack_real_msec() + timeout);

		int idx = socket % _count;
		bool status = _array[idx]->sendQuest(socket, token, raw, seqNum, callback, timeout, discardableUDPQuest);
		if (!status)
			delete raw;

		return status;
	}

	FPAnswerPtr PartitionedConnectionMap::sendQuest(int socket, uint64_t token, std::mutex* mutex, FPQuestPtr quest, int timeout, bool discardableUDPQuest)
	{
		if (!quest->isTwoWay())
		{
			sendQuestWithBasicAnswerCallback(socket, token, quest, NULL, 0, discardableUDPQuest);
			return NULL;
		}

		std::shared_ptr<SyncedAnswerCallback> s(new SyncedAnswerCallback(mutex, quest));
		if (!sendQuestWithBasicAnswerCallback(socket, token, quest, s.get(), timeout, discardableUDPQuest))
		{
			return FpnnErrorAnswer(quest, FPNN_EC_CORE_SEND_ERROR, "unknown sending error.");
		}

		return s->takeAnswer();
	}
}
