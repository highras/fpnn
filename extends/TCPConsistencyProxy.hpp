#ifndef FPNN_TCP_Consistency_Proxy_H
#define FPNN_TCP_Consistency_Proxy_H

#include "ExtendsException.hpp"
#include "TCPProxyCore.hpp"
#include "ConsistencyAnswerCallback.hpp"

namespace fpnn
{
	/*
		If using Quest Processor, please set Quest Processor before call updateEndpoints().
	*/
	class TCPConsistencyProxy: virtual public TCPProxyCore
	{
	private:
		ConsistencySuccessCondition _condition;
		ConsistencyCallbackMapSuitePtr _callbackMapSuite;
		int _requiredCount;

		bool broadcastQuest(FPQuestPtr quest, BasicAnswerCallback* callback, ConsistencySuccessCondition condition, int requiredCount, int timeout)
		{
			if (quest->isOneWay())
				return false;

			std::map<std::string, TCPClientPtr> clients;
			{
				std::unique_lock<std::mutex> lck(_mutex);
				beforeBorcastQuest();

				if (_clients.empty())
					return false;

				clients = _clients;
			}

			int total = (int)clients.size();
			ConsistencyCallbackStatSuitePtr ccss;
			if (condition == ConsistencySuccessCondition::AllQuestsSuccess)
			{
				ccss.reset(new ConsistencyCallbackStatSuite(quest, total, total));
			}
			else if (condition == ConsistencySuccessCondition::HalfQuestsSuccess)
			{
				if (total & 0x1)
					ccss.reset(new ConsistencyCallbackStatSuite(quest, (total >> 1) + 1, total));
				else
					ccss.reset(new ConsistencyCallbackStatSuite(quest, total >> 1, total));
			}
			else if (condition == ConsistencySuccessCondition::CountedQuestsSuccess)
			{
				if (total < requiredCount)
					return false;

				ccss.reset(new ConsistencyCallbackStatSuite(quest, requiredCount, total));
			}
			else if (condition == ConsistencySuccessCondition::OneQuestSuccess)
			{
				ccss.reset(new ConsistencyCallbackStatSuite(quest, 1, total));
			}
			else
				return false;

			_callbackMapSuite->insertCallback(quest, callback);
			
			broadcast(quest, clients, ccss, timeout);
			return true;
		}

		void broadcast(FPQuestPtr quest, std::map<std::string, TCPClientPtr>& clients, ConsistencyCallbackStatSuitePtr ccss, int timeout)
		{
			for (auto& clientPair: clients)
			{
				int errorCode = FPNN_EC_OK;
				TCPClientPtr client = clientPair.second;
				ConsistencyAnswerCallback* callback = new ConsistencyAnswerCallback(_callbackMapSuite, ccss);

				if (!client->connected() && !client->reconnect())
				{
					errorCode = FPNN_EC_CORE_INVALID_CONNECTION;
				}
				else
				{
					if (!client->sendQuest(quest, callback, timeout))
						errorCode = FPNN_EC_CORE_UNKNOWN_ERROR;
				}

				if (errorCode != FPNN_EC_OK)
				{
					callback->fillResult(nullptr, errorCode);
					BasicAnswerCallbackPtr task(callback);

					if (!ClientEngine::wakeUpAnswerCallbackThreadPool(task))
					{
						LOG_ERROR("[Error]Process maybe exiting. Wake up answer callback thread pool of client engine to process consistency proxy failed callback failed. Run it in current thread.");
						task->run();
					}
				}
			}
		}

	protected:
		virtual void beforeBorcastQuest() {}

		virtual void afterUpdate()
		{
			for (auto& endpoint: _endpoints)
			{
				auto iter = _clients.find(endpoint);
				if (iter == _clients.end())
				{
					createTCPClient(endpoint, false);
				}
			}
		}

	public:
		/*
		 *	If questTimeoutSeconds less then zero, mean using global settings.
		 *  If SuccessCondition isn't CountedSuccess, parameter requiredCount is ignored.
		 */
		TCPConsistencyProxy(ConsistencySuccessCondition condition, int requiredCount = 0, int64_t questTimeoutSeconds = -1):
			TCPProxyCore(questTimeoutSeconds), _condition(condition), _requiredCount(requiredCount)
		{
			_callbackMapSuite = std::make_shared<ConsistencyCallbackMapSuite>();
		}
		virtual ~TCPConsistencyProxy()
		{
			/* don't need to do anythings.
			 * The _callbackMapSuite will be holding in each callback of each client in each connection callback map of them.
			 * If all clients are destroyed after this function, each client will clear their connection callback map.
			 * When the callback map of client clearing, the callback in callback map will be given an error, and the action will
			 * trigger the ConsistencyAnswerCallback to increase the failed count. When the failed count is triggered the failed
			 * condition, orginial callback in map of _callbackMapSuite will be triggered, and the original callback will be cleared
			 * from _callbackMapSuite. When the last ConsistencyAnswerCallback destroyed, _callbackMapSuite's reference count will
			 * come to zero, and _callbackMapSuite will be destroyed. 
			 */
		}

		/**
			All SendQuest():
				If return false, caller must free quest & callback.
				If return true, don't free quest & callback.

			timeout in seconds.
		*/
		FPAnswerPtr sendQuest(FPQuestPtr quest, int timeout = 0)
		{
			std::shared_ptr<SyncedAnswerCallback> s(new SyncedAnswerCallback(&_mutex, quest));
			if (!broadcastQuest(quest, s.get(), _condition, _requiredCount, timeout))
				return FPAWriter::errorAnswer(quest, FPNN_EC_CORE_SEND_ERROR, "unknown sending error.");

			return s->takeAnswer();
		}
		bool sendQuest(FPQuestPtr quest, AnswerCallback* callback, int timeout = 0)
		{
			return broadcastQuest(quest, callback, _condition, _requiredCount, timeout);
		}
		bool sendQuest(FPQuestPtr quest, std::function<void (FPAnswerPtr answer, int errorCode)> task, int timeout = 0)
		{
			BasicAnswerCallback* callback = new FunctionAnswerCallback(std::move(task));
			if (broadcastQuest(quest, callback, _condition, _requiredCount, timeout))
				return true;
			else
			{
				delete callback;
				return false;
			}
		}

		FPAnswerPtr sendQuest(FPQuestPtr quest, ConsistencySuccessCondition condition, int requiredCount = 0, int timeout = 0)
		{
			std::shared_ptr<SyncedAnswerCallback> s(new SyncedAnswerCallback(&_mutex, quest));
			if (!broadcastQuest(quest, s.get(), condition, requiredCount, timeout))
				return FPAWriter::errorAnswer(quest, FPNN_EC_CORE_SEND_ERROR, "unknown sending error.");

			return s->takeAnswer();
		}
		bool sendQuest(FPQuestPtr quest, AnswerCallback* callback, ConsistencySuccessCondition condition, int requiredCount = 0, int timeout = 0)
		{
			return broadcastQuest(quest, callback, condition, requiredCount, timeout);
		}
		bool sendQuest(FPQuestPtr quest, std::function<void (FPAnswerPtr answer, int errorCode)> task, ConsistencySuccessCondition condition, int requiredCount = 0, int timeout = 0)
		{
			BasicAnswerCallback* callback = new FunctionAnswerCallback(std::move(task));
			if (broadcastQuest(quest, callback, condition, requiredCount, timeout))
				return true;
			else
			{
				delete callback;
				return false;
			}
		}
	};
	typedef std::shared_ptr<TCPConsistencyProxy> TCPConsistencyProxyPtr;
}

#endif

