#include <iostream>
#include "UDPTestProcessor.h"
#include "TCPClient.h"
#include "UDPClient.h"

using namespace fpnn;

FPAnswerPtr UDPTestProcessor::startTest(const FPReaderPtr args, const FPQuestPtr quest, const ConnectionInfo& ci)
{
	if (ci.isUDP())
		return FpnnErrorAnswer(quest, 500000, "Muust called by TCP protocol.");

	int64_t taskId = slack_real_msec();

	std::thread(&UDPTestProcessor::startX, this, args, taskId).detach();
	return FPAWriter(1, quest)("taskId", taskId);
}

FPAnswerPtr UDPTestProcessor::test(const FPReaderPtr args, const FPQuestPtr quest, const ConnectionInfo& ci)
{
	int64_t taskId = args->wantInt("taskId");
	int64_t count = args->wantInt("count");
	int64_t seq = args->wantInt("seq");
	int64_t msec = args->wantInt("msec");
	int kind = args->wantInt("kind");

	int64_t msec2 = slack_real_msec();

	sendAnswer(FPAWriter::emptyAnswer(quest));

	BaseStatPtr stat;
	{
		std::lock_guard<std::mutex> lck (_mutex);
		auto it = _taskCache.find(taskId);
		if (it != _taskCache.end())
			stat = it->second;
		else
		{
			stat = std::make_shared<BaseStat>(count, 0, 0);
			_taskCache[taskId] = stat;
		}
	}

	if (kind == 0)
		stat->tcpCostMs[seq] = msec2 - msec;
	else if (kind == 1)
		stat->udpARQCostMs[seq] = msec2 - msec;
	else if (kind == 2)
		stat->udpNakeCostMs[seq] = msec2 - msec;
	else if (kind == 3)
		stat->udpMixCostMs[seq] = msec2 - msec;
	
	return nullptr;
}

FPAnswerPtr ReadTestRecord(FPQuestPtr quest, BaseStatPtr stat, bool detail);
FPAnswerPtr UDPTestProcessor::query(const FPReaderPtr args, const FPQuestPtr quest, const ConnectionInfo& ci)
{
	if (ci.isUDP())
		return FpnnErrorAnswer(quest, 500000, "Muust called by TCP protocol.");

	int64_t taskId = args->wantInt("taskId");
	bool detail = args->getBool("detail");

	BaseStatPtr stat = nullptr;
	{
		std::lock_guard<std::mutex> lck (_mutex);
		auto it = _taskCache.find(taskId);
		if (it != _taskCache.end())
			stat = it->second;
	}

	if (stat)
	{
		return ReadTestRecord(quest, stat, detail);
	}
	else
	{
		return FpnnErrorAnswer(quest, 500000, "Cannot find task record.");
	}
}

void CalcuateAverage(std::vector<int>& vec, int& avg, int& min, int& max, int& count)
{
	count = 0;
	max = -1;
	min = 99999999;
	int64_t total = 0;
	for (int cost: vec)
	{
		if (cost >= 0)
		{
			count += 1;
			total += cost;
			if (cost > max)
				max = cost;
			else if (cost < min)
				min = cost;
		}
	}

	if (count == 0)
		avg = -1;
	else
		avg = (int)(total / count);
}

FPAnswerPtr ReadTestRecord(FPQuestPtr quest, BaseStatPtr stat, bool detail)
{
	std::map<std::string, int> tcpStat;
	std::map<std::string, int> udpARQStat;
	std::map<std::string, int> udpNakeStat;
	std::map<std::string, int> udpMixStat;

	FPAWriter aw(7 + (detail ? 4 : 0), quest);
	aw.param("count", stat->count);
	aw.param("qps", stat->QPS);
	aw.param("mixRate", stat->mixRate);

	

	int avg, min, max, count;

	CalcuateAverage(stat->tcpCostMs, avg, min, max, count);
	tcpStat["avg"] = avg;
	tcpStat["min"] = min;
	tcpStat["max"] = max;
	tcpStat["timeout"] = stat->count - count;

	CalcuateAverage(stat->udpARQCostMs, avg, min, max, count);
	udpARQStat["avg"] = avg;
	udpARQStat["min"] = min;
	udpARQStat["max"] = max;
	udpARQStat["timeout"] = stat->count - count;

	CalcuateAverage(stat->udpNakeCostMs, avg, min, max, count);
	udpNakeStat["avg"] = avg;
	udpNakeStat["min"] = min;
	udpNakeStat["max"] = max;
	udpNakeStat["timeout"] = stat->count - count;

	CalcuateAverage(stat->udpMixCostMs, avg, min, max, count);
	udpMixStat["avg"] = avg;
	udpMixStat["min"] = min;
	udpMixStat["max"] = max;
	udpMixStat["timeout"] = stat->count - count;

	aw.param("tcp", tcpStat);
	aw.param("udp(ARQ)", udpARQStat);
	aw.param("udp(Nake)", udpNakeStat);
	aw.param("udp(mix)", udpMixStat);

	if (detail)
	{
		aw.param("tcp", stat->tcpCostMs);
		aw.param("udp(ARQ)", stat->udpARQCostMs);
		aw.param("udp(Nake)", stat->udpNakeCostMs);
		aw.param("udp(mix)", stat->udpMixCostMs);
	}

	return aw.take();
}

FPQuestPtr BuildQuest(int64_t taskId, int count, int seq, int64_t msec, int kind)
{
	FPQWriter qw(5, "test");
	qw.param("taskId", taskId);
	qw.param("count", count);
	qw.param("seq", seq);
	qw.param("msec", msec);
	qw.param("kind", kind);

	return qw.take();
}

void DoTest(BaseStatPtr stat, ClientPtr client, bool tcp, int count, int qps, int64_t taskId)
{
	int interval = 1000*1000/qps;

	client->connect();

	for (int i = 0; i < count; i++)
	{
		int seq = i;
		int64_t msec = slack_real_msec();
		client->sendQuest(BuildQuest(taskId, count, seq, msec, tcp ? 0 : 1), [stat, tcp, msec, seq](FPAnswerPtr answer, int errorCode){
			if (errorCode != 0)
				return;

			int64_t msec2 = slack_real_msec();
			int cost = (int)(msec2 - msec)/2;
			if (tcp)
				stat->tcpCostMs[seq] = cost;
			else
				stat->udpARQCostMs[seq] = cost;
		});
		if (qps == 1)
			sleep(1);
		else
			usleep(interval);
	}
}

void DoNakeTest(BaseStatPtr stat, UDPClientPtr client, int count, int qps, int64_t taskId)
{
	int interval = 1000*1000/qps;

	client->connect();

	for (int i = 0; i < count; i++)
	{
		int seq = i;
		int64_t msec = slack_real_msec();
		client->sendQuestEx(BuildQuest(taskId, count, seq, msec, 2), [stat, msec, seq](FPAnswerPtr answer, int errorCode){
			if (errorCode != 0)
				return;

			int64_t msec2 = slack_real_msec();
			int cost = (int)(msec2 - msec)/2;
			stat->udpNakeCostMs[seq] = cost;
		}, true);
		if (qps == 1)
			sleep(1);
		else
			usleep(interval);
	}
}

void DoMixTest(BaseStatPtr stat, UDPClientPtr client, int rate, int count, int qps, int64_t taskId)
{
	int interval = 1000*1000/qps;
	int rateStep = rate;
	bool discardable;

	client->connect();

	for (int i = 0; i < count; i++)
	{
		if (--rateStep > 0)
			discardable = true;
		else
		{
			discardable = false;
			rateStep = rate;
		}

		int seq = i;
		int64_t msec = slack_real_msec();
		client->sendQuestEx(BuildQuest(taskId, count, seq, msec, 3), [stat, msec, seq](FPAnswerPtr answer, int errorCode){
			if (errorCode != 0)
				return;

			int64_t msec2 = slack_real_msec();
			int cost = (int)(msec2 - msec)/2;
			stat->udpMixCostMs[seq] = cost;
		}, discardable);
		if (qps == 1)
			sleep(1);
		else
			usleep(interval);
	}
}

void UDPTestProcessor::startX(const FPReaderPtr args, int64_t taskId)
{
	try
	{
		std::string tcp = args->wantString("tcp");
		std::string udp = args->wantString("udp");
		int count = args->wantInt("count");
		int qps = args->wantInt("qps");
		int rate = args->wantInt("rate");

		int waitSecond = ClientEngine::getQuestTimeout();
		if (waitSecond == 0)
			waitSecond = 5;

		BaseStatPtr stat(new BaseStat(count, qps, rate));

		TCPClientPtr tcpClient = TCPClient::createClient(tcp);
		UDPClientPtr udpARQClient = UDPClient::createClient(udp);
		UDPClientPtr udpNakeClient = UDPClient::createClient(udp);
		UDPClientPtr udpMixClient = UDPClient::createClient(udp);

		DoTest(stat, tcpClient, true , count, qps, taskId);
		sleep(waitSecond);

		DoTest(stat, udpARQClient, false, count, qps, taskId);
		sleep(waitSecond);

		DoNakeTest(stat, udpNakeClient, count, qps, taskId);
		sleep(waitSecond);

		DoMixTest(stat, udpMixClient, rate, count, qps, taskId);
		sleep(waitSecond);

		std::lock_guard<std::mutex> lck (_mutex);
		_taskCache[taskId] = stat;

		std::cout<<"----[Task "<<taskId<<"] completed."<<std::endl;
	}
	catch (const std::exception& ex)
	{
	}
}