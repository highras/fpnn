#ifndef Bandwidth_Quest_Processor_h
#define Bandwidth_Quest_Processor_h

#include "IQuestProcessor.h"
#include "../Transfer/Transfer.h"

using namespace fpnn;

class BandwidthQuestProcessor: public IQuestProcessor
{
	QuestProcessorClassPrivateFields(BandwidthQuestProcessor)

	std::mutex _mutex;
	std::map<int, DataReceiverPtr> _receiverMap;
	int _currTaskId;

public:
	BandwidthQuestProcessor(): _currTaskId(0)
	{
		registerMethod("ls", &BandwidthQuestProcessor::ls);
		registerMethod("dl", &BandwidthQuestProcessor::dl);
		registerMethod("up", &BandwidthQuestProcessor::up);
		registerMethod("data", &BandwidthQuestProcessor::data);
		registerMethod("stream", &BandwidthQuestProcessor::stream);
	}

	virtual ~BandwidthQuestProcessor() {}

	//-- External
	FPAnswerPtr dl(const FPReaderPtr args, const FPQuestPtr quest, const ConnectionInfo& ci);
	FPAnswerPtr up(const FPReaderPtr args, const FPQuestPtr quest, const ConnectionInfo& ci);
	FPAnswerPtr ls(const FPReaderPtr args, const FPQuestPtr quest, const ConnectionInfo& ci);
	FPAnswerPtr data(const FPReaderPtr args, const FPQuestPtr quest, const ConnectionInfo& ci);
	FPAnswerPtr stream(const FPReaderPtr args, const FPQuestPtr quest, const ConnectionInfo& ci);

	QuestProcessorClassBasicPublicFuncs
};

#endif
