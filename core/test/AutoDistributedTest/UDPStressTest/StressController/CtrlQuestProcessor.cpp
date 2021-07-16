#include <iostream>
#include "CtrlQuestProcessor.h"

using namespace std;

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
	//std::string endpoint = args->wantString("endpoint");
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

	if (record.recvCount && record.allCostUsec/record.recvCount > _timecostThreshold)
	{
		int v = ++_overloadCount;
		if (v > 5)
			_needStop = true;
	}
	else
		_overloadCount = 0;

	return FPAWriter::emptyAnswer(quest);
}

FPAnswerPtr CtrlQuestProcessor::actorResult(const FPReaderPtr args, const FPQuestPtr quest, const ConnectionInfo& ci)
{
	return FPAWriter::emptyAnswer(quest);
}