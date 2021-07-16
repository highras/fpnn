#ifndef Controller_Quest_Processor_h
#define Controller_Quest_Processor_h

#include "IQuestProcessor.h"
#include "StressRecorder.h"

using namespace fpnn;

struct UploadTaskStatus
{
	int total;
	std::atomic<int> failedCount;
	bool completed;
	bool lastStatus;

	UploadTaskStatus(): total(0), failedCount(0), completed(false), lastStatus(true) {}
};
typedef std::shared_ptr<UploadTaskStatus> UploadTaskStatusPtr;

struct DeployTaskStatus
{
	std::vector<std::string> failedEndpoints;
	bool completed;

	DeployTaskStatus(): completed(false) {}
};
typedef std::shared_ptr<DeployTaskStatus> DeployTaskStatusPtr;

class CtrlQuestProcessor: public IQuestProcessor
{
	QuestProcessorClassPrivateFields(CtrlQuestProcessor)

	UploadTaskStatusPtr _uploadTaskStatus;
	DeployTaskStatusPtr _deployTaskStatus;

	std::atomic<int64_t> _overloadBeginMsec;
	volatile bool _needStop;
	int _timecostThreshold;

public:
	StressRecorder stressRecorder;

public:
	CtrlQuestProcessor(): _overloadBeginMsec(0), _needStop(false)
	{
		registerMethod("uploadFinish", &CtrlQuestProcessor::uploadFinish);
		registerMethod("deployFinish", &CtrlQuestProcessor::deployFinish);
		registerMethod("actorStatus", &CtrlQuestProcessor::actorStatus);
		registerMethod("actorResult", &CtrlQuestProcessor::actorResult);

		_uploadTaskStatus.reset(new UploadTaskStatus());
		_deployTaskStatus.reset(new DeployTaskStatus());
	}

	FPAnswerPtr uploadFinish(const FPReaderPtr args, const FPQuestPtr quest, const ConnectionInfo& ci);
	FPAnswerPtr deployFinish(const FPReaderPtr args, const FPQuestPtr quest, const ConnectionInfo& ci);
	FPAnswerPtr actorStatus(const FPReaderPtr args, const FPQuestPtr quest, const ConnectionInfo& ci);
	FPAnswerPtr actorResult(const FPReaderPtr args, const FPQuestPtr quest, const ConnectionInfo& ci);

	UploadTaskStatusPtr uploadTaskStatus() { return _uploadTaskStatus; }
	DeployTaskStatusPtr deployTaskStatus() { return _deployTaskStatus; }
	bool needStop() { return _needStop; }
	void setTimecostThreshold(int threshold) { _timecostThreshold = threshold; }

	QuestProcessorClassBasicPublicFuncs
};
typedef std::shared_ptr<CtrlQuestProcessor> CtrlQuestProcessorPtr;

#endif