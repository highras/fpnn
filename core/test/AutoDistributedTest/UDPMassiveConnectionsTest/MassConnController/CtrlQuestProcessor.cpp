#include <iostream>
#include "CtrlQuestProcessor.h"

using namespace std;

extern std::mutex gc_outputMutex;
extern std::atomic<bool> gc_showDetail;

FPAnswerPtr CtrlQuestProcessor::uploadFinish(const FPReaderPtr args, const FPQuestPtr quest, const ConnectionInfo& ci)
{
	_uploadTaskStatus->lastStatus = args->wantBool("ok");
	_uploadTaskStatus->completed = true;

	return FPAWriter::emptyAnswer(quest);
}

FPAnswerPtr CtrlQuestProcessor::deployFinish(const FPReaderPtr args, const FPQuestPtr quest, const ConnectionInfo& ci)
{
	_deployTaskStatus->failedEndpoints = args->want("failedEndpoints", std::vector<std::string>());
	_deployTaskStatus->completed = true;
	
	return FPAWriter::emptyAnswer(quest);
}

FPAnswerPtr CtrlQuestProcessor::actorStatus(const FPReaderPtr args, const FPQuestPtr quest, const ConnectionInfo& ci)
{
	int taskId = args->wantInt("taskId");
	//std::string region = args->wantString("region");
	std::string endpoint = args->wantString("endpoint");
	std::string payload = args->wantString("payload");

	FPReader reader(payload);

	struct Record record;

	record.clientCount = reader.wantInt("connections");
	record.periodUsec = reader.wantInt("period");
	
	record.sendCount = reader.wantInt("send");
	record.recvCount = reader.wantInt("recv");
	record.sendErrorCount = reader.wantInt("serror");
	record.recvErrorCount = reader.wantInt("rerror");
	record.allCostUsec = reader.wantInt("allcost");

	stressRecorder.addRecord(taskId, record);

	if (gc_showDetail)
	{
		std::unique_lock<std::mutex> lck(gc_outputMutex);
		std::cout<<" -- endpoint: "<<endpoint<<", task: "<<taskId<<", conn: "<<record.clientCount<<", period usec: "<<record.periodUsec;
		std::cout<<", send: "<<record.sendCount<<" (QPS): "<<(record.sendCount * 1000 * 1000 / record.periodUsec);
		std::cout<<", recv: "<<record.recvCount<<" (QPS): "<<(record.recvCount * 1000 * 1000 / record.periodUsec);
		std::cout<<", sendError: "<<record.sendErrorCount<<", recvError: "<<record.recvErrorCount;
		std::cout<<", avg cost usec: "<<(record.allCostUsec / record.recvCount)<<std::endl;
	}

	if (record.recvCount && record.allCostUsec/record.recvCount > _timecostThreshold)
	{
		int64_t lastOverloadMsec = _overloadBeginMsec;
		
		if (lastOverloadMsec == 0)
			_overloadBeginMsec = slack_real_msec();
		else
		{
			if (slack_real_msec() - lastOverloadMsec > 10 * 1000)		//-- 10 seconds
				_needStop = true;
		}
	}
	else
		_overloadBeginMsec = 0;

	return FPAWriter::emptyAnswer(quest);
}

FPAnswerPtr CtrlQuestProcessor::actorResult(const FPReaderPtr args, const FPQuestPtr quest, const ConnectionInfo& ci)
{
	return FPAWriter::emptyAnswer(quest);
}