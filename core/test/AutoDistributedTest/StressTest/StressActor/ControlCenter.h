#ifndef Control_Center_h
#define Control_Center_h

#include "TCPClient.h"

using namespace fpnn;

/*
	Class ControlCenter is assistant class. Just use it directly.
*/
class ControlCenter
{
public:
	static void beginTask(int taskId, const std::string& method, const std::string& desc);
	static void finishTask(int taskId);

	static FPAnswerPtr sendQuest(FPQuestPtr quest, int timeout = 0);
	static bool sendQuest(FPQuestPtr quest, AnswerCallback* callback, int timeout = 0);
	static bool sendQuest(FPQuestPtr quest, std::function<void (FPAnswerPtr answer, int errorCode)> task, int timeout = 0);
};

#endif