#ifndef FPNN_BYPASS_H
#define FPNN_BYPASS_H

/*
	配置文件约束

	# split by ";, "
	Bypass.<tagName>.FPZK.cluster.carp.list = 
	Bypass.<tagName>.FPZK.cluster.oldest.list = 
	Bypass.<tagName>.FPZK.cluster.random.list = 
	Bypass.<tagName>.FPZK.cluster.broadcast.list =
*/

#include <list>
#include <memory>
#include <vector>
#include "Setting.h"
#include "StringUtil.h"
#include "FPWriter.h"
#include "FPZKClient.h"
#include "TCPFPZKCarpProxy.hpp"
#include "TCPFPZKOldestProxy.hpp"
#include "TCPFPZKRandomProxy.hpp"
#include "TCPFPZKBroadcastProxy.hpp"

using namespace fpnn;

class Bypass
{
	bool _empty;
	FPZKClientPtr _fpzk;
	std::list<TCPFPZKCarpProxyPtr> _carpList;
	std::list<TCPFPZKOldestProxyPtr> _oldestList;
	std::list<TCPFPZKRandomProxyPtr> _randomList;
	std::list<TCPFPZKBroadcastProxyPtr> _broadcastList;

public:
	Bypass(FPZKClientPtr fpzkClient, const char* tagName): _fpzk(fpzkClient)
	{
		std::string prefix("Bypass.");
		prefix.append(tagName);

		std::string carpNames = Setting::getString(std::string(prefix).append(".FPZK.cluster.carp.list"));
		std::string oldestNames = Setting::getString(std::string(prefix).append(".FPZK.cluster.oldest.list"));
		std::string randomNames = Setting::getString(std::string(prefix).append(".FPZK.cluster.random.list"));
		std::string broadcastNames = Setting::getString(std::string(prefix).append(".FPZK.cluster.broadcast.list"));

		std::vector<std::string> carpList, oldestList, randomList, broadcastList;
		StringUtil::split(carpNames, ";, ", carpList);
		StringUtil::split(oldestNames, ";, ", oldestList);
		StringUtil::split(randomNames, ";, ", randomList);
		StringUtil::split(broadcastNames, ";, ", broadcastList);

		for (auto& name: carpList)
		{
			TCPFPZKCarpProxyPtr proxy(new TCPFPZKCarpProxy(_fpzk, name));
			_carpList.push_back(proxy);
		}

		for (auto& name: oldestList)
		{
			TCPFPZKOldestProxyPtr proxy(new TCPFPZKOldestProxy(_fpzk, name));
			_oldestList.push_back(proxy);
		}

		for (auto& name: randomList)
		{
			TCPFPZKRandomProxyPtr proxy(new TCPFPZKRandomProxy(_fpzk, name));
			_randomList.push_back(proxy);
		}

		for (auto& name: broadcastList)
		{
			TCPFPZKBroadcastProxyPtr proxy(new TCPFPZKBroadcastProxy(_fpzk, name));
			_broadcastList.push_back(proxy);
		}

		_empty = (_carpList.size() + _oldestList.size() + _randomList.size() + _broadcastList.size()) == 0;
	}

	void bypass(int64_t hintId, const FPQuestPtr quest)
	{
		if (_empty)
			return;
		
		FPQuestPtr actQuest = FPQWriter::CloneQuest(quest->method(), quest);

		for (auto proxy: _carpList)
		{
			if (proxy->empty())
				continue;
			
			bool status = proxy->sendQuest(hintId, actQuest, [hintId, actQuest, proxy](FPAnswerPtr answer, int errorCode)
				{
					if (errorCode != fpnn::FPNN_EC_OK)
					{
						std::string method = actQuest->method();
						bool status = proxy->sendQuest(hintId, actQuest, [hintId, method](FPAnswerPtr answer, int errorCode)
						{
							if (errorCode != fpnn::FPNN_EC_OK)
								LOG_ERROR("Bypass event %s for hintId %lld (broadcast, secondly) return exception.", method.c_str(), hintId);
						});

						if (status == false)//&& proxy->empty() != true)
							LOG_ERROR("Bypass event %s for hintId %lld (broadcast, secondly) failed.", actQuest->method().c_str(), hintId);
					}
				});
			if (status == false)// && proxy->empty() != true)
			{
				status = proxy->sendQuest(hintId, actQuest, [hintId, actQuest](FPAnswerPtr answer, int errorCode)
					{
						if (errorCode != fpnn::FPNN_EC_OK)
							LOG_ERROR("Bypass event %s for hintId %lld (broadcast, secondly) return exception.", actQuest->method().c_str(), hintId);
					});
				if (status == false)// && proxy->empty() != true)
					LOG_ERROR("Bypass event %s for hintId %lld (broadcast, secondly) failed.", quest->method().c_str(), hintId);
			}
		}

		for (auto proxy: _oldestList)
		{
			if (proxy->empty())
				continue;
			
			bool status = proxy->sendQuest(actQuest, [actQuest, proxy](FPAnswerPtr answer, int errorCode)
				{
					if (errorCode != fpnn::FPNN_EC_OK)
					{
						std::string method = actQuest->method();
						bool status = proxy->sendQuest(actQuest, [method](FPAnswerPtr answer, int errorCode)
						{
							if (errorCode != fpnn::FPNN_EC_OK)
								LOG_ERROR("Bypass event %s (oldest, secondly) return exception.", method.c_str());
						});

						if (status == false)//&& proxy->empty() != true)
							LOG_ERROR("Bypass event %s (oldest, secondly) failed.", actQuest->method().c_str());
					}
				});
			if (status == false)// && proxy->empty() != true)
			{
				status = proxy->sendQuest(actQuest, [actQuest](FPAnswerPtr answer, int errorCode)
					{
						if (errorCode != fpnn::FPNN_EC_OK)
							LOG_ERROR("Bypass event %s (oldest, secondly) return exception.", actQuest->method().c_str());
					});
				if (status == false)// && proxy->empty() != true)
					LOG_ERROR("Bypass event %s (oldest, secondly) failed.", quest->method().c_str());
			}
		}

		for (auto proxy: _randomList)
		{
			if (proxy->empty())
				continue;
			
			bool status = proxy->sendQuest(actQuest, [actQuest, proxy](FPAnswerPtr answer, int errorCode)
				{
					if (errorCode != fpnn::FPNN_EC_OK)
					{
						std::string method = actQuest->method();
						bool status = proxy->sendQuest(actQuest, [method](FPAnswerPtr answer, int errorCode)
						{
							if (errorCode != fpnn::FPNN_EC_OK)
								LOG_ERROR("Bypass event %s (random, secondly) return exception.", method.c_str());
						});

						if (status == false)//&& proxy->empty() != true)
							LOG_ERROR("Bypass event %s (random, secondly) failed.", actQuest->method().c_str());
					}
				});
			if (status == false)// && proxy->empty() != true)
			{
				status = proxy->sendQuest(actQuest, [actQuest](FPAnswerPtr answer, int errorCode)
					{
						if (errorCode != fpnn::FPNN_EC_OK)
							LOG_ERROR("Bypass event %s (random, secondly) return exception.", actQuest->method().c_str());
					});
				if (status == false)// && proxy->empty() != true)
					LOG_ERROR("Bypass event %s (random, secondly) failed.", quest->method().c_str());
			}
		}
		
		for (auto proxy: _broadcastList)
		{
			if (proxy->empty())
				continue;

			bool status = proxy->sendQuest(actQuest, [actQuest, proxy](FPAnswerPtr answer, int errorCode)
			{
				if (errorCode != fpnn::FPNN_EC_OK)
				{
					std::string method = actQuest->method();
					bool status = proxy->sendQuest(actQuest, [method](FPAnswerPtr answer, int errorCode)
					{
						if (errorCode != fpnn::FPNN_EC_OK)
							LOG_ERROR("Bypass event %s (broadcast, secondly) return exception.", method.c_str());
					});

					if (status == false)// && proxy->empty() != true)
						LOG_ERROR("Bypass event %s (broadcast, secondly) failed.", actQuest->method().c_str());
				}
			});
			if (status == false)// && proxy->empty() != true)
			{
				status = proxy->sendQuest(actQuest, [actQuest](FPAnswerPtr answer, int errorCode)
					{
						if (errorCode != fpnn::FPNN_EC_OK)
							LOG_ERROR("Bypass event %s (broadcast, secondly) return exception.", actQuest->method().c_str());
					});
				if (status == false)// && proxy->empty() != true)
					LOG_ERROR("Bypass event %s (broadcast, secondly) failed.", quest->method().c_str());
			}
		}
	}
};
typedef std::shared_ptr<Bypass> BypassPtr;

#endif