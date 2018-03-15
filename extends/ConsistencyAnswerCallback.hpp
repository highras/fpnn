#ifndef FPNN_Consistency_Answer_Callback_H
#define FPNN_Consistency_Answer_Callback_H

#include <atomic>
#include <memory>
#include <unordered_map>
#include "FpnnError.h"
#include "ClientEngine.h"
#include "AnswerCallbacks.h"

namespace fpnn
{
	enum class ConsistencySuccessCondition
	{
		AllQuestsSuccess,
		HalfQuestsSuccess,
		CountedQuestsSuccess,
		OneQuestSuccess,
	};

	struct ConsistencyCallbackStatSuite
	{
		FPQuestPtr quest;
		int successCount;
		int failedCount;
		int requiredCount;
		int totalCount;

		ConsistencyCallbackStatSuite(FPQuestPtr orginialQuest, int required_count, int total_count):
			quest(orginialQuest), successCount(0), failedCount(0), requiredCount(required_count), totalCount(total_count) {}
	};
	typedef std::shared_ptr<ConsistencyCallbackStatSuite> ConsistencyCallbackStatSuitePtr;

	struct ConsistencyCallbackMapSuite
	{
	private:
		std::unordered_map<void *, BasicAnswerCallback*> _callbackMap;

	public:
		std::mutex mutex;
		
	public:
		ConsistencyCallbackMapSuite() {}

		void insertCallback(FPQuestPtr quest, BasicAnswerCallback* callback)
		{
			std::unique_lock<std::mutex> lck(mutex);
			_callbackMap[quest.get()] = callback;
		}
		BasicAnswerCallback* checkCondition(ConsistencyCallbackStatSuitePtr consistencyCallbackStatSuite, bool onSuccessed)
		{
			void *target = NULL;

			if (onSuccessed)
			{
				if (consistencyCallbackStatSuite->successCount == consistencyCallbackStatSuite->requiredCount)
					target = consistencyCallbackStatSuite->quest.get();
			}
			else
			{
				if (consistencyCallbackStatSuite->failedCount == (consistencyCallbackStatSuite->totalCount - consistencyCallbackStatSuite->requiredCount + 1))
					target = consistencyCallbackStatSuite->quest.get();
			}

			if (target)
			{
				auto iter = _callbackMap.find(target);
				if (iter != _callbackMap.end())
				{
					BasicAnswerCallback* callback = iter->second;
					_callbackMap.erase(iter);
					return callback;
				}
			}

			return NULL;
		}
	};
	typedef std::shared_ptr<ConsistencyCallbackMapSuite> ConsistencyCallbackMapSuitePtr;

	class ConsistencyAnswerCallback: public AnswerCallback
	{
		ConsistencyCallbackMapSuitePtr _callbackMapSuite;
		ConsistencyCallbackStatSuitePtr _callbackStatSuite;

		void launchRealCallback(BasicAnswerCallback* callback, bool successed, FPAnswerPtr answer, int errorCode = FPNN_EC_OK)
		{
			if (callback->syncedCallback())		//-- check first, then fill result.
			{
				SyncedAnswerCallback* sac = (SyncedAnswerCallback*)callback;
				sac->fillResult(answer, errorCode);
				return;
			}
			
			callback->fillResult(answer, errorCode);
			BasicAnswerCallbackPtr task(callback);

			if (!ClientEngine::wakeUpAnswerCallbackThreadPool(task))
			{
				LOG_ERROR("[Error]Process maybe exiting. Wake up answer callback thread pool of client engine to process consistency proxy answer callback failed. Run it in current thread.");
				task->run();
			}
		}

	public:
		ConsistencyAnswerCallback(ConsistencyCallbackMapSuitePtr callbackMapSuite, ConsistencyCallbackStatSuitePtr callbackStatSuite):
			_callbackMapSuite(callbackMapSuite), _callbackStatSuite(callbackStatSuite) {}

		virtual void onAnswer(FPAnswerPtr answer)
		{
			if (answer->status())
			{
				FPAReader ar(answer);
				int errorCode = ar.get("code", (int)FPNN_EC_CORE_UNKNOWN_ERROR);
				onException(answer, errorCode);
				return;
			}

			BasicAnswerCallback* callback = NULL;
			{
				std::unique_lock<std::mutex> lck(_callbackMapSuite->mutex);
				_callbackStatSuite->successCount++;

				callback = _callbackMapSuite->checkCondition(_callbackStatSuite, true);
			}
			if (callback)
				launchRealCallback(callback, true, answer);
		}
		virtual void onException(FPAnswerPtr answer, int errorCode)
		{
			BasicAnswerCallback* callback = NULL;
			{
				std::unique_lock<std::mutex> lck(_callbackMapSuite->mutex);
				_callbackStatSuite->failedCount++;
				
				callback = _callbackMapSuite->checkCondition(_callbackStatSuite, false);
			}
			if (callback)
				launchRealCallback(callback, false, answer, errorCode);
		}
	};
}

#endif