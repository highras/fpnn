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

	

	bool PartitionedConnectionMap::sendQuestWithBasicAnswerCallback(int socket, uint64_t token, FPQuestPtr quest, BasicAnswerCallback* callback, int timeout)
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

		uint32_t seqNum = quest->seqNumLE();

		if (callback)
			callback->updateExpiredTime(slack_real_msec() + timeout);

		int idx = socket % _count;
		bool status = _array[idx]->sendQuest(socket, token, raw, seqNum, callback);
		if (!status)
			delete raw;

		return status;
	}

	FPAnswerPtr PartitionedConnectionMap::sendQuest(int socket, uint64_t token, std::mutex* mutex, FPQuestPtr quest, int timeout)
	{
		if (!quest->isTwoWay())
		{
			sendQuestWithBasicAnswerCallback(socket, token, quest, NULL, 0);
			return NULL;
		}

		std::shared_ptr<SyncedAnswerCallback> s(new SyncedAnswerCallback(mutex, quest));
		if (!sendQuestWithBasicAnswerCallback(socket, token, quest, s.get(), timeout))
		{
			return FpnnErrorAnswer(quest, FPNN_EC_CORE_SEND_ERROR, "unknown sending error.");
		}

		return s->takeAnswer();
	}
}
