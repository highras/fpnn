#ifndef FPNN_Broadcast_Answer_Callback_H
#define FPNN_Broadcast_Answer_Callback_H

#include <unordered_set>
#include "ClientEngine.h"

namespace fpnn
{
	struct BroadcastAnswerCallback
	{
		virtual bool autoDelete() { return true; }		//-- Only used in FPNN internal.
		virtual ~BroadcastAnswerCallback() {}

		virtual void onAnswer(const std::string& endpoint, FPAnswerPtr) {}
		virtual void onException(const std::string& endpoint, FPAnswerPtr, int errorCode) {}
		virtual void onCompleted(std::map<std::string, FPAnswerPtr>& answerMap) {}
	};

	struct BroadcastEveryAnswerCallback final: public BroadcastAnswerCallback
	{
		std::function<void (const std::string& endpoint, FPAnswerPtr answer, int errorCode)> _function;

		virtual ~BroadcastEveryAnswerCallback() {}

		virtual void onAnswer(const std::string& endpoint, FPAnswerPtr answer)
		{
			_function(endpoint, answer, FPNN_EC_OK);
		}
		virtual void onException(const std::string& endpoint, FPAnswerPtr answer, int errorCode)
		{
			_function(endpoint, answer, errorCode);
		}
	};

	struct BroadcastCompletedAnswerCallback final: public BroadcastAnswerCallback
	{
		std::function<void (std::map<std::string, FPAnswerPtr>& answerMap)> _function;

		virtual ~BroadcastCompletedAnswerCallback() {}

		virtual void onCompleted(std::map<std::string, FPAnswerPtr>& answerMap)
		{
			_function(answerMap);
		}
	};

	struct SyncedBroadcastAnswerCallback final: public BroadcastAnswerCallback
	{
	private:
		bool _done;
		std::mutex _mutex;
		std::condition_variable _condition;

	public:
		std::map<std::string, FPAnswerPtr> answerMap;

		SyncedBroadcastAnswerCallback(): _done(false) {}
		virtual ~SyncedBroadcastAnswerCallback() {}
		virtual bool autoDelete() { return false; }

		virtual void onCompleted(std::map<std::string, FPAnswerPtr>& answerMap_)
		{
			std::unique_lock<std::mutex> lck(_mutex);
			answerMap.swap(answerMap_);
			_done = true;
			_condition.notify_one();
		}

		void takeAnswer()
		{
			std::unique_lock<std::mutex> lck(_mutex);
			while (!_done)
				_condition.wait(lck);
		}
	};

	struct BroadcastCallbackMonitor
	{
		std::mutex mutex;
		FPQuestPtr quest;
		BroadcastAnswerCallback *callback;
		std::map<std::string, FPAnswerPtr> answerMap;

		BroadcastCallbackMonitor(FPQuestPtr quest_, BroadcastAnswerCallback* cb): quest(quest_), callback(cb) {}
		~BroadcastCallbackMonitor()
		{
			if (callback)
			{
				bool autoDelete = callback->autoDelete();	//-- For thread safety, checking whether auto delete must before call callback->onCompleted().
				callback->onCompleted(answerMap);
				if (autoDelete)
					delete callback;
			}
		}
	};
	typedef std::shared_ptr<BroadcastCallbackMonitor> BroadcastCallbackMonitorPtr;

	class BroadcastProxyAnswerCallback: public AnswerCallback
	{
		std::string _endpoint;
		BroadcastCallbackMonitorPtr _callbackMonitor;

	public:
		BroadcastProxyAnswerCallback(const std::string& endpoint, BroadcastCallbackMonitorPtr callbackMonitor):
			_endpoint(endpoint), _callbackMonitor(callbackMonitor) {}
		virtual ~BroadcastProxyAnswerCallback() {}

		virtual void onAnswer(FPAnswerPtr answer)
		{
			{
				std::unique_lock<std::mutex> lck(_callbackMonitor->mutex);
				_callbackMonitor->answerMap[_endpoint] = answer;
			}

			if (_callbackMonitor->callback)
				_callbackMonitor->callback->onAnswer(_endpoint, answer);
		}

		virtual void onException(FPAnswerPtr answer, int errorCode)
		{
			FPAnswerPtr ans = answer;
			if (!answer)
				ans = FPAWriter::errorAnswer(_callbackMonitor->quest, errorCode, "");
			
			{
				std::unique_lock<std::mutex> lck(_callbackMonitor->mutex);
				_callbackMonitor->answerMap[_endpoint] = ans;
			}

			if (_callbackMonitor->callback)
				_callbackMonitor->callback->onException(_endpoint, answer, errorCode);
		}
	};
}

#endif