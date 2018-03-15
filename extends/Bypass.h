#ifndef FPNN_BYPASS_H
#define FPNN_BYPASS_H

/*
	配置文件约束

	# split by ";, "
	Bypass.FPZK.cluster.carp.list = 
	Bypass.FPZK.cluster.random.list = 
	Bypass.FPZK.cluster.broadcast.list =
*/

#include <list>
#include <memory>
#include <vector>
#include "Setting.h"
#include "StringUtil.h"
#include "FPWriter.h"
#include "FPZKClient.h"
#include "TCPFPZKCarpProxy.hpp"
#include "TCPFPZKRandomProxy.hpp"
#include "TCPFPZKConsistencyProxy.hpp"

using namespace fpnn;

class Bypass
{
	FPZKClientPtr _fpzk;
	std::list<TCPFPZKCarpProxyPtr> _carpList;
	std::list<TCPFPZKRandomProxyPtr> _randomList;
	std::list<TCPFPZKConsistencyProxyPtr> _broadcastList;

public:
	Bypass(FPZKClientPtr fpzkClient): _fpzk(fpzkClient)
	{
		std::string carpNames = Setting::getString("Bypass.FPZK.cluster.carp.list");
		std::string randomNames = Setting::getString("Bypass.FPZK.cluster.random.list");
		std::string broadcastNames = Setting::getString("Bypass.FPZK.cluster.broadcast.list");

		std::vector<std::string> carpList, randomList, broadcastList;
		StringUtil::split(carpNames, ";, ", carpList);
		StringUtil::split(randomNames, ";, ", randomList);
		StringUtil::split(broadcastNames, ";, ", broadcastList);

		for (auto& name: carpList)
		{
			TCPFPZKCarpProxyPtr proxy(new TCPFPZKCarpProxy(_fpzk, name));
			_carpList.push_back(proxy);
		}

		for (auto& name: randomList)
		{
			TCPFPZKRandomProxyPtr proxy(new TCPFPZKRandomProxy(_fpzk, name));
			_randomList.push_back(proxy);
		}

		for (auto& name: broadcastList)
		{
			TCPFPZKConsistencyProxyPtr proxy(new TCPFPZKConsistencyProxy(_fpzk, name, ConsistencySuccessCondition::AllQuestsSuccess));
			_broadcastList.push_back(proxy);
		}
	}

	void bypass(int64_t hintId, const FPQuestPtr quest)
	{
		//std::string jsonBody = quest->json();
		//FPQWriter qw(quest->method(), jsonBody);
		//FPQuestPtr actQuest = qw.take();
		FPQuestPtr actQuest = FPQWriter::CloneQuest(quest->method(), quest);

		for (auto proxy: _carpList)
		{
			//if (proxy->empty())
			//	continue;
			
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

		for (auto proxy: _randomList)
		{
			//if (proxy->empty())
			//	continue;
			
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
			//if (proxy->empty())
			//	continue;

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