#ifndef FPNN_Quest_Sender_Implements_H
#define FPNN_Quest_Sender_Implements_H

#include "IQuestProcessor.h"

namespace fpnn
{
	class TCPQuestSender: public QuestSender
	{
		int _socket;
		uint64_t _token;
		std::mutex* _mutex;
		IConcurrentSender* _concurrentSender;

	public:
		TCPQuestSender(IConcurrentSender* concurrentSender, const ConnectionInfo& ci, std::mutex* mutex):
			_socket(ci.socket), _token(ci.token), _mutex(mutex), _concurrentSender(concurrentSender) {}

		virtual ~TCPQuestSender() {}
		/**
			All SendQuest():
				If return false, caller must free quest & callback.
				If return true, don't free quest & callback.
		*/
		virtual FPAnswerPtr sendQuest(FPQuestPtr quest, int timeout = 0)
		{
			return _concurrentSender->sendQuest(_socket, _token, _mutex, quest, timeout * 1000);
		}
	
		virtual bool sendQuest(FPQuestPtr quest, AnswerCallback* callback, int timeout = 0)
		{
			return _concurrentSender->sendQuest(_socket, _token, quest, callback, timeout * 1000);
		}
	
		virtual bool sendQuest(FPQuestPtr quest, std::function<void (FPAnswerPtr answer, int errorCode)> task, int timeout = 0)
		{
			return _concurrentSender->sendQuest(_socket, _token, quest, std::move(task), timeout * 1000);
		}
	};

	class UDPQuestSender: public QuestSender
	{
		ConnectionInfoPtr _connInfo;
		IConcurrentUDPSender* _concurrentSender;

	public:
		UDPQuestSender(IConcurrentUDPSender* concurrentSender, ConnectionInfoPtr ci):
			_connInfo(ci), _concurrentSender(concurrentSender) {}

		virtual ~UDPQuestSender() {}
		/**
			All SendQuest():
				If return false, caller must free quest & callback.
				If return true, don't free quest & callback.
		*/
		virtual FPAnswerPtr sendQuest(FPQuestPtr quest, int timeout = 0)
		{
			return _concurrentSender->sendQuest(_connInfo, quest, timeout * 1000);
		}
	
		virtual bool sendQuest(FPQuestPtr quest, AnswerCallback* callback, int timeout = 0)
		{
			return _concurrentSender->sendQuest(_connInfo, quest, callback, timeout * 1000);
		}
	
		virtual bool sendQuest(FPQuestPtr quest, std::function<void (FPAnswerPtr answer, int errorCode)> task, int timeout = 0)
		{
			return _concurrentSender->sendQuest(_connInfo, quest, std::move(task), timeout * 1000);
		}
	};
}

#endif