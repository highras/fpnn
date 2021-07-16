#ifndef Stress_Machine_Info_h
#define Stress_Machine_Info_h

#include <mutex>
#include <map>
#include <set>
#include <string>
#include "msec.h"
#include "NetworkUtility.h"

struct Record
{
	int clientCount;
	int64_t periodUsec;
	int64_t sendCount;
	int64_t recvCount;
	int64_t sendErrorCount;
	int64_t recvErrorCount;
	int64_t allCostUsec;
};

struct MachineStatus
{
	int halfPingMsec;
	double load;
	int connCount;
	int64_t RX;
	int64_t TX;
};

struct StatUnit
{
	int64_t average;
	int64_t min;
	int64_t max;

	StatUnit(): average(0), min(-1), max(-1) {}
	void reset()
	{
		average = 0;
		min = -1;
		max = -1;
	}
};

struct ArchivedRecord
{
	int clientCount;
	struct StatUnit sendQPS;
	struct StatUnit recvQPS;
	int64_t sendErrorCount;
	int64_t recvErrorCount;
	struct StatUnit costUsecPerQuest;

	ArchivedRecord(): clientCount(0), sendErrorCount(0), recvErrorCount(0) {}
	void reset()
	{
		clientCount = 0;
		sendQPS.reset();
		recvQPS.reset();
		sendErrorCount = 0;
		recvErrorCount = 0;
		costUsecPerQuest.reset();
	}
};

template<typename N>
struct Range
{
	N min;
	N max;

	void add(N n)
	{
		if (min < 0)
		{
			min = n;
			max = n;
		}
		else if (n > max)
			max = n;
		else if (n < min)
			min = n;
	}

	std::string str()
	{
		if (min == max)
			return std::to_string(min);
		else
			return std::to_string(min).append(" ~ ").append(std::to_string(max));
	}
};

struct ArchivedMachineStatus
{
	struct Range<int> connCount;
	struct Range<double> load;
	struct Range<int> halfPingMsec;
	int64_t RX;
	int64_t TX;

	ArchivedMachineStatus(): connCount{-1, -1}, load{-1., -1.}, halfPingMsec{-1, -1}, RX(0), TX(0) {}
};

struct SecondSectionTotal
{
	int clientCount;
	int64_t sendCount;
	int64_t recvCount;
	int64_t sendErrorCount;
	int64_t recvErrorCount;
	int64_t allCostUsec;

	//int64_t minSendQPS;
	//int64_t maxSendQPS;

	//int64_t minRecvQPS;
	//int64_t maxRecvQPS;

	int64_t minCostUsecPerQuest;
	int64_t maxCostUsecPerQuest;

	SecondSectionTotal(): clientCount(0), sendCount(0), recvCount(0), sendErrorCount(0), recvErrorCount(0), allCostUsec(0)
	{
		//minSendQPS = 0;
		//maxSendQPS = 0;

		//minRecvQPS = 0;
		//maxRecvQPS = 0;

		minCostUsecPerQuest = -1;
		maxCostUsecPerQuest = -1;
	}
};

class StressRecorder
{
	std::mutex _mutex;
	std::map<int64_t, std::map<int, struct Record>> _rawRecords;	//-- map<sec, map<taskId, record>>
	std::map<int64_t, std::map<int64_t, struct ArchivedRecord>> _archivedRecords;	//-- map<minute, map<sec, archived>>

	int _cpu;
	std::map<int64_t, struct MachineStatus> _rawStatus;	//-- map<sec, status>
	std::map<int64_t, struct ArchivedMachineStatus> _archivedStatus;	//-- map<minute, archived>

	std::map<int64_t, int>	_serverQPS;
	std::map<int64_t, int>	_serverSessions;
	std::map<int64_t, int>	_serverARQSessions;

	void archive();
	void archiveSecondSections(std::map<int64_t, std::map<int64_t, std::map<int, struct Record>>>& cache);
	void archiveSetp3(std::map<int64_t, std::map<int64_t, struct SecondSectionTotal>>& medium);
	void archiveSetp4(int64_t minute, int64_t sec, int period, ArchivedRecord& archived);

	void archiveStatus(int64_t second);

public:
	StressRecorder(): _cpu(0) {}
	
	void addRecord(int taskId, struct Record& record)
	{
		std::unique_lock<std::mutex> lck(_mutex);
		_rawRecords[slack_real_sec()][taskId] = record;
		archive();
	}
	void addMachineStatus(int cpu, struct MachineStatus& status)
	{
		std::unique_lock<std::mutex> lck(_mutex);

		if (_cpu == 0)
			_cpu = cpu;

		int64_t second = slack_real_sec();
		_rawStatus[second] = status;
		archiveStatus(second);
	}
	void addQPS(int qps);
	void addSessions(int count);
	void addARQSessions(int count);
	void clear()
	{
		std::unique_lock<std::mutex> lck(_mutex);
		_rawRecords.clear();
		_archivedRecords.clear();

		_rawStatus.clear();
		_archivedStatus.clear();
	}
	void showRecords();
	bool saveToCSVFormat(const std::string& filename);
};

#endif