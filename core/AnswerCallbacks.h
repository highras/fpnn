#ifndef FPNN_Answer_Callbacks_H
#define FPNN_Answer_Callbacks_H

#include <mutex>
#include <memory>
#include <functional>
#include <unordered_map>
#include <condition_variable>
#include "ITaskThreadPool.h"
#include "FPMessage.h"
#include "FPWriter.h"
#include "FPReader.h"
#include "FPLog.h"

namespace fpnn
{
	class BasicAnswerCallback;
	typedef std::shared_ptr<BasicAnswerCallback> BasicAnswerCallbackPtr;

	//=================================================================//
	//- Basic Answer Callback:
	//=================================================================//
	class BasicAnswerCallback: public ITaskThreadPool::ITask
	{
		int64_t _sendTime;

	public:
		virtual ~BasicAnswerCallback() {}
		/** If error set, answer will be NULL. This is mean a fatal error occurred, connection will be colsed. */
		virtual void fillResult(FPAnswerPtr answer, int errorCode) = 0;
		virtual bool syncedCallback() { return false; }
		
		void updateSendTime(int64_t sendTime) { _sendTime = sendTime; }
		int64_t sendTime() { return _sendTime; }
	};

	//=================================================================//
	//- Synced Answer Callback:
	//=================================================================//
	class SyncedAnswerCallback: public BasicAnswerCallback
	{
		FPQuestPtr _quest;
		std::mutex* _mutex;
		std::condition_variable _condition;
		FPAnswerPtr _answer;

	public:
		SyncedAnswerCallback(std::mutex* mutex, FPQuestPtr quest):
			_quest(quest), _mutex(mutex), _answer(nullptr) {}

		virtual ~SyncedAnswerCallback() {}
		virtual void run() final {}
		virtual void fillResult(FPAnswerPtr answer, int errorCode) final
		{
			if (!answer)
				answer = FpnnErrorAnswer(_quest, errorCode, "no msg, please refer to log.:)");

			std::unique_lock<std::mutex> lck(*_mutex);
			_answer = answer;
			_condition.notify_one();
		}
		virtual bool syncedCallback() { return true; }

		FPAnswerPtr takeAnswer()
		{
			std::unique_lock<std::mutex> lck(*_mutex);
  			while (!_answer)
  				_condition.wait(lck);
  			
  			return _answer;
		}
	};

	//=================================================================//
	//- (Standard) Answer Callback:
	//=================================================================//
	class AnswerCallback: public BasicAnswerCallback
	{
		int _errorCode;
		FPAnswerPtr _answer;

	public:
		AnswerCallback(): _errorCode(FPNN_EC_OK), _answer(0) {}
		virtual ~AnswerCallback()
		{
		}
		virtual void run() final
		{
			if (_errorCode == FPNN_EC_OK)
				onAnswer(_answer);
			else
				onException(_answer, _errorCode);
		}

		virtual void fillResult(FPAnswerPtr answer, int errorCode) final
		{
			_answer = answer;
			_errorCode = errorCode;

			if (errorCode == FPNN_EC_OK && answer->status())
			{
				FPAReader ar(answer);
				_errorCode = ar.get("code", (int)FPNN_EC_CORE_UNKNOWN_ERROR);
			}
		}

		virtual void onAnswer(FPAnswerPtr) = 0;
		virtual void onException(FPAnswerPtr answer, int errorCode) = 0;
	};

	//=================================================================//
	//- Function Answer Callback:
	//=================================================================//
	class FunctionAnswerCallback: public BasicAnswerCallback
	{
	private:
		int _errorCode;
		FPAnswerPtr _answer;
		std::function<void (FPAnswerPtr answer, int errorCode)> _function;

	public:
		explicit FunctionAnswerCallback(std::function<void (FPAnswerPtr answer, int errorCode)> function):
			_errorCode(FPNN_EC_OK), _answer(0), _function(function) {}
		virtual ~FunctionAnswerCallback()
		{
		}

  		virtual void run() final
  		{
  			_function(_answer, _errorCode);
  		}

  		virtual void fillResult(FPAnswerPtr answer, int errorCode) final
		{
			_answer = answer;
			_errorCode = errorCode;

			if (errorCode == FPNN_EC_OK && answer->status())
			{
				FPAReader ar(answer);
				_errorCode = ar.get("code", (int)FPNN_EC_CORE_UNKNOWN_ERROR);
			}
		}
	};
}

#endif
