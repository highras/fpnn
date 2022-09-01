#include "../../../ClientEngine.h"
#include "FpnnClientCenter.h"

using namespace fpnn;

std::mutex ClientCenter::_mutex;
static std::atomic<bool> _created(false);
static ClientCenterPtr _clientCenter;

ClientCenterPtr ClientCenter::instance()
{
	if (!_created)
	{
		std::unique_lock<std::mutex> lck(_mutex);
		if (!_created)
		{
			_clientCenter.reset(new ClientCenter);
			_created = true;
		}
	}
	return _clientCenter;
}

ClientCenter::ClientCenter(): _running(true), _timeoutQuest(FPNN_DEFAULT_QUEST_TIMEOUT * 1000)
{
	_loopThread = std::thread(&ClientCenter::loopThread, this);
}

ClientCenter::~ClientCenter()
{
	_running = false;
	_loopThread.join();
}

void ClientCenter::loopThread()
{
	while (_running)
	{
		int cyc = 100;
		int udpSendingCheckSyc = 5;
		while (_running && cyc--)
		{
			udpSendingCheckSyc -= 1;
			if (udpSendingCheckSyc == 0)
			{
				udpSendingCheckSyc = 5;
				
				std::unordered_set<UDPFPNNProtocolProcessorPtr> udpProcessors;
				{
					std::unique_lock<std::mutex> lck(_mutex);
					udpProcessors = _udpProcessors;
				}

				for (auto processor: udpProcessors)
				{
					processor->sendCacheData();
					processor->cleanExpiredCallbacks();
				}
			}

			usleep(10000);
		}

		std::list<std::map<uint32_t, BasicAnswerCallback*>> timeouted;
		int64_t current = slack_real_msec();

		{
			std::unique_lock<std::mutex> lck(_mutex);

			for (auto& pp: _callbackMap)
			{
				std::map<uint32_t, BasicAnswerCallback*> currMap;
				timeouted.push_back(currMap);

				for (auto& pp2: pp.second)
				{
					if (pp2.second->expiredTime() <= current)
						currMap[pp2.first] = pp2.second;
				}

				if (currMap.size())
				{
					for (auto& pp2: currMap)
						pp.second.erase(pp2.first);

					timeouted.back().swap(currMap);
				}
				else
					timeouted.pop_back();
			}
		}

		for (auto& cbMap: timeouted)
			for (auto& pp: cbMap)
				runCallback(nullptr, FPNN_EC_CORE_TIMEOUT, pp.second);
	}

	//-- loop thread will exit.
	std::unordered_map<int, std::unordered_map<uint32_t, BasicAnswerCallback*>> allCallbackMap;
	{
		std::unique_lock<std::mutex> lck(_mutex);
		allCallbackMap.swap(_callbackMap);
	}

	for (auto& pp: allCallbackMap)
		cleanCallbacks(pp.second);

	//--TODO
}

void ClientCenter::cleanCallbacks(std::unordered_map<uint32_t, BasicAnswerCallback*>& callbackMap)
{
	for (auto& pp: callbackMap)
	{
		runCallback(nullptr, FPNN_EC_CORE_CONNECTION_CLOSED, pp.second);
	}
	callbackMap.clear();
}

void ClientCenter::runCallback(FPAnswerPtr answer, int errorCode, BasicAnswerCallback* callback)
{
	if (callback->syncedCallback())
		callback->fillResult(answer, errorCode);
	else
	{
		callback->fillResult(answer, errorCode);

		BasicAnswerCallbackPtr task(callback);
		ClientEngine::wakeUpAnswerCallbackThreadPool(task);
	}
}

void ClientCenter::unregisterConnection(int socket)
{
	std::unordered_map<uint32_t, BasicAnswerCallback*> callbackMap;

	ClientCenterPtr self = instance();

	{
		std::unique_lock<std::mutex> lck(_mutex);
		auto iter = self->_callbackMap.find(socket);
		if (iter != self->_callbackMap.end())
		{
			callbackMap.swap(iter->second);
			self->_callbackMap.erase(socket);
		}
		else
			return;
	}

	cleanCallbacks(callbackMap);
}

BasicAnswerCallback* ClientCenter::takeCallback(int socket, uint32_t seqNum)
{
	ClientCenterPtr self = instance();

	std::unique_lock<std::mutex> lck(_mutex);
	auto iter = self->_callbackMap.find(socket);
	if (iter != self->_callbackMap.end())
	{
		auto subIter = iter->second.find(seqNum);
		if (subIter != iter->second.end())
		{
			BasicAnswerCallback* cb = subIter->second;
			iter->second.erase(seqNum);
			//-- Don't processing the case for iter->second empty.
			//-- It can be processed in loopThread, but a perfect implememted
			//-- must triggered in unregisterConnection() function.
			return cb;
		}
	}

	return NULL;
}

void ClientCenter::registerCallback(int socket, uint32_t seqNum, BasicAnswerCallback* callback)
{
	ClientCenterPtr self = instance();

	std::unique_lock<std::mutex> lck(_mutex);
	self->_callbackMap[socket][seqNum] = callback;
}

void ClientCenter::registerUDPProcessor(UDPFPNNProtocolProcessorPtr processor)
{
	ClientCenterPtr self = instance();

	std::unique_lock<std::mutex> lck(_mutex);
	self->_udpProcessors.insert(processor);
}

void ClientCenter::unregisterUDPProcessor(UDPFPNNProtocolProcessorPtr processor)
{
	ClientCenterPtr self = instance();

	std::unique_lock<std::mutex> lck(_mutex);
	self->_udpProcessors.erase(processor);
}
