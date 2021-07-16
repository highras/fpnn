#ifndef UDP_Test_Processor_H
#define UDP_Test_Processor_H

#include <map>
#include "IQuestProcessor.h"

using namespace fpnn;

struct BaseStat
{
	int count;
	int QPS;
	int mixRate;
	std::vector<int> tcpCostMs;
	std::vector<int> udpARQCostMs;
	std::vector<int> udpNakeCostMs;
	std::vector<int> udpMixCostMs;

	BaseStat(int count_, int qps, int rate)
	{
		count = count_;
		QPS = qps;
		mixRate = rate;

		for (int i = 0; i < count; i++)
		{
			tcpCostMs.push_back(-1);
			udpARQCostMs.push_back(-1);
			udpNakeCostMs.push_back(-1);
			udpMixCostMs.push_back(-1);
		}
	}
};
typedef std::shared_ptr<BaseStat> BaseStatPtr;

class UDPTestProcessor: public IQuestProcessor
{
	QuestProcessorClassPrivateFields(UDPTestProcessor)

	std::mutex _mutex;
	std::map<int64_t, BaseStatPtr> _taskCache;

	void startX(const FPReaderPtr args, int64_t taskId);

	FPAnswerPtr start(const FPReaderPtr args, const FPQuestPtr quest, const ConnectionInfo& ci);
	FPAnswerPtr test(const FPReaderPtr args, const FPQuestPtr quest, const ConnectionInfo& ci);
	FPAnswerPtr query(const FPReaderPtr args, const FPQuestPtr quest, const ConnectionInfo& ci);

public:

	UDPTestProcessor()
	{
		registerMethod("start", &UDPTestProcessor::start);
		registerMethod("test", &UDPTestProcessor::test);
		registerMethod("query", &UDPTestProcessor::query);
	}
	QuestProcessorClassBasicPublicFuncs
};

#endif