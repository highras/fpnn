#include "UDPSessions.h"

using namespace fpnn;

bool UDPCallbackMap::insert(ConnectionInfoPtr ci, uint32_t seqNum, BasicAnswerCallback* callback)
{
	std::unique_lock<std::mutex> lck(_mutex);
	//_callbackMap[ci->token][seqNum] = callback;

	std::unordered_map<uint32_t, BasicAnswerCallback*>& sub = _callbackMap[ci->token];

	std::pair<uint32_t, BasicAnswerCallback*> data(seqNum, callback);
	std::pair<std::unordered_map<uint32_t, BasicAnswerCallback*>::iterator, bool> res = sub.insert(data);
	return res.second;
}

BasicAnswerCallback* UDPCallbackMap::takeCallback(ConnectionInfoPtr ci, uint32_t seqNum)
{
	std::unique_lock<std::mutex> lck(_mutex);
	auto iter = _callbackMap.find(ci->token);
	if (iter != _callbackMap.end())
	{
		auto subIter = iter->second.find(seqNum);
		if (subIter == iter->second.end())
			return NULL;

		BasicAnswerCallback* rev = subIter->second;
		iter->second.erase(subIter);
		if (iter->second.empty())
			_callbackMap.erase(iter);

		return rev;
	}
	return NULL;
}

void UDPCallbackMap::extractTimeoutedCallback(int64_t now, std::list<std::map<uint32_t, BasicAnswerCallback*> >& timeouted)
{
	std::set<uint64_t> emptyConnInfoSet;

	std::unique_lock<std::mutex> lck(_mutex);
	for (auto& connPair: _callbackMap)
	{
		std::map<uint32_t, BasicAnswerCallback*> emptyMap;
		timeouted.push_back(emptyMap);
		std::map<uint32_t, BasicAnswerCallback*>& currMap = timeouted.back();

		for (auto& cbPair: connPair.second)
		{
			if (cbPair.second->expiredTime() <= now)
				currMap[cbPair.first] = cbPair.second;
		}

		for (auto& ecbPair: currMap)
			connPair.second.erase(ecbPair.first);

		if (connPair.second.empty())
			emptyConnInfoSet.insert(connPair.first);
	}

	for (auto key: emptyConnInfoSet)
		_callbackMap.erase(key);
}

void UDPCallbackMap::clearAndFetchAllRemnantCallbacks(std::set<BasicAnswerCallback*> &callbacks)
{
	std::unique_lock<std::mutex> lck(_mutex);
	for (auto& connPair: _callbackMap)
		for (auto& cbPair: connPair.second)
			callbacks.insert(cbPair.second);

	_callbackMap.clear();
}

