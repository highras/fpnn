#include <time.h>
#include <list>
#include <sstream>
#include <iostream>
#include "StringUtil.h"
#include "FileSystemUtil.h"
#include "FormattedPrint.h"
#include "StressRecorder.h"

using namespace std;
using namespace fpnn;

extern std::mutex gc_outputMutex;
const int gc_periodSeconds = 3;

void StressRecorder::archive()	//-- split by minute and seconds section.
{
	int64_t currSec = slack_real_sec();
	int64_t currMinute = currSec / 60;
	int64_t thresholdSec = currMinute * 60;

	std::set<int64_t> archived;
	std::map<int64_t, std::map<int64_t, std::map<int, struct Record>>> cache;	//-- map<minute, map<seconds/section, map<taskId, record>>>

	for (auto it = _rawRecords.begin(); it != _rawRecords.end(); it++)
	{
		if (it->first < thresholdSec)
		{
			int64_t minute = it->first / 60;
			int64_t section = (it->first - minute * 60) / gc_periodSeconds;

			for (auto& pp: it->second)
				cache[minute][section][pp.first] = pp.second;
			
			archived.insert(it->first);
		}
	}

	for (int64_t sec: archived)
		_rawRecords.erase(sec);

	archiveSecondSections(cache);
}

//-- 按照3秒一个间隔，分段合并统计数据
void StressRecorder::archiveSecondSections(std::map<int64_t, std::map<int64_t, std::map<int, struct Record>>>& cache)
{
	std::map<int64_t, std::map<int64_t, struct SecondSectionTotal>> medium;
	for (auto& pp: cache)
	{
		for (auto& pp2: pp.second)
		{
			for (auto& pp3: pp2.second)
			{
				medium[pp.first][pp2.first].clientCount += pp3.second.clientCount;

				medium[pp.first][pp2.first].sendCount += pp3.second.sendCount;
				medium[pp.first][pp2.first].recvCount += pp3.second.recvCount;
				medium[pp.first][pp2.first].sendErrorCount += pp3.second.sendErrorCount;
				medium[pp.first][pp2.first].recvErrorCount += pp3.second.recvErrorCount;
				medium[pp.first][pp2.first].allCostUsec += pp3.second.allCostUsec;

				if (pp3.second.recvCount > 0)
				{
					int64_t cpq = pp3.second.allCostUsec / pp3.second.recvCount;
					
					if (medium[pp.first][pp2.first].minCostUsecPerQuest < 0 || medium[pp.first][pp2.first].minCostUsecPerQuest > cpq)
						medium[pp.first][pp2.first].minCostUsecPerQuest = cpq;
					if (medium[pp.first][pp2.first].maxCostUsecPerQuest < 0 || medium[pp.first][pp2.first].maxCostUsecPerQuest < cpq)
						medium[pp.first][pp2.first].maxCostUsecPerQuest = cpq;
				}
			}
		}
	}

	archiveSetp3(medium);
}

//-- 合并每分钟的数据
void StressRecorder::archiveSetp3(std::map<int64_t, std::map<int64_t, struct SecondSectionTotal>>& medium)	//-- map<minute, map<seconds/section, SecondSectionTotal>>
{
	for (auto& pp: medium)
	{
		auto it = pp.second.begin();
		int64_t sec = it->first;
		int period = 0;

		ArchivedRecord archived;

		for (; it != pp.second.end(); it++)
		{
			SecondSectionTotal& arso = it->second;

			if (arso.clientCount == 0)
			{
				archiveSetp4(pp.first, sec, period, archived);
				archived.reset();
				period = 0;
				continue;
			}

			if (archived.clientCount == 0)
				archived.clientCount = arso.clientCount;
			else if (archived.clientCount != arso.clientCount)
			{
				archiveSetp4(pp.first, sec, period, archived);
				archived.reset();
				period = 0;

				archived.clientCount = arso.clientCount;
			}

			sec = it->first;
			period += gc_periodSeconds;

			archived.sendErrorCount += arso.sendErrorCount;
			archived.recvErrorCount += arso.recvErrorCount;

			if (arso.sendCount > 0)
			{
				archived.sendQPS.average += arso.sendCount;

				int64_t qps = arso.sendCount / gc_periodSeconds;

				if (archived.sendQPS.min < 0 || archived.sendQPS.min > qps)
					archived.sendQPS.min = qps;
				if (archived.sendQPS.max < 0 || archived.sendQPS.max < qps)
					archived.sendQPS.max = qps;
			}

			if (arso.recvCount > 0)
			{
				archived.recvQPS.average += arso.recvCount;

				int64_t qps = arso.recvCount / gc_periodSeconds;

				if (archived.recvQPS.min < 0 || archived.recvQPS.min > qps)
					archived.recvQPS.min = qps;
				if (archived.recvQPS.max < 0 || archived.recvQPS.max < qps)
					archived.recvQPS.max = qps;
			}

			archived.costUsecPerQuest.average += arso.allCostUsec;
			if (archived.costUsecPerQuest.min < 0 || archived.costUsecPerQuest.min > arso.minCostUsecPerQuest)
				archived.costUsecPerQuest.min = arso.minCostUsecPerQuest;
			if (archived.costUsecPerQuest.max < 0 || archived.costUsecPerQuest.max < arso.maxCostUsecPerQuest)
				archived.costUsecPerQuest.max = arso.maxCostUsecPerQuest;
		}

		archiveSetp4(pp.first, sec, period, archived);
	}
}

void StressRecorder::archiveSetp4(int64_t minute, int64_t sec, int period, ArchivedRecord& archived)
{
	if (archived.clientCount == 0 || period == 0)
		return;

	if (archived.recvQPS.average)
		archived.costUsecPerQuest.average /= archived.recvQPS.average;
	else
		archived.costUsecPerQuest.average = 0;
	
	archived.sendQPS.average /= period;
	archived.recvQPS.average /= period;
	
	_archivedRecords[minute][sec] = archived;
}

void StressRecorder::archiveStatus(int64_t second)
{
	int64_t currMinute = second / 60;
	int64_t thresholdSec = currMinute * 60;

	std::set<int64_t> archived;
	std::map<int64_t, std::list<struct MachineStatus>> cache;

	for (auto it = _rawStatus.begin(); it != _rawStatus.end(); it++)
	{
		if (it->first < thresholdSec)
		{
			int64_t minute = it->first / 60;
			cache[minute].push_back(it->second);
			archived.insert(it->first);
		}
	}

	for (int64_t sec: archived)
		_rawStatus.erase(sec);


	//----------

	for (auto it = cache.begin(); it != cache.end(); it++)
	{
		ArchivedMachineStatus ams;
		for (auto& ms: it->second)
		{
			ams.load.add(ms.load);
			ams.connCount.add(ms.connCount);
			ams.halfPingMsec.add(ms.halfPingMsec);

			if (ams.RX < ms.RX)
				ams.RX = ms.RX;

			if (ams.TX < ms.TX)
				ams.TX = ms.TX;
		}

		_archivedStatus[it->first] = ams;
	}
}

void StressRecorder::addQPS(int qps)
{
	std::unique_lock<std::mutex> lck(_mutex);
	int64_t currSec = slack_real_sec();
	int64_t currMinute = currSec / 60;
	_serverQPS[currMinute] = qps;
}

void StressRecorder::addSessions(int count)
{
	std::unique_lock<std::mutex> lck(_mutex);
	int64_t currSec = slack_real_sec();
	int64_t currMinute = currSec / 60;
	_serverSessions[currMinute] = count;
}

void StressRecorder::addARQSessions(int count)
{
	std::unique_lock<std::mutex> lck(_mutex);
	int64_t currSec = slack_real_sec();
	int64_t currMinute = currSec / 60;
	_serverARQSessions[currMinute] = count;
}

const std::vector<std::string> ArchivedRecordsFields{"Record time (UTC)", "Stress Connection Count",
	"Average Send QPS", "Min Send QPS", "Max Send QPS",
	"Average Recv QPS", "Min Recv QPS", "Max Recv QPS",
	"Average Send Error Count", "Average Recv Error Count",
	"Average Time Cost (usec)", "Min Time Cost (usec)", "Max Time Cost (usec)",
	"Interface QPS", "Server UDP Session Count", "Server UDP ARQ Session Count", "load", "load/cpu", "ping/2 (msec)", "Recv Bytes/s", "Send Bytes/s"
};
void StressRecorder::showRecords()
{
	std::vector<std::vector<std::string>> rows;

	{
		std::unique_lock<std::mutex> lck(_mutex);
		archive();
		archiveStatus(slack_real_sec());

		for (auto& pp: _archivedRecords)
		{
			for (auto& pp2: pp.second)
			{
				std::vector<std::string> row;

				char buf[32];
				time_t rtime = (time_t)pp.first * 60 + pp2.first;
				std::string recordTime = ctime_r(&rtime, buf);
				recordTime = StringUtil::trim(recordTime);

				ArchivedRecord& archived = pp2.second;

				row.push_back(recordTime);
				row.push_back(std::to_string(archived.clientCount));

				row.push_back(std::to_string(archived.sendQPS.average));
				row.push_back(std::to_string(archived.sendQPS.min));
				row.push_back(std::to_string(archived.sendQPS.max));

				row.push_back(std::to_string(archived.recvQPS.average));
				row.push_back(std::to_string(archived.recvQPS.min));
				row.push_back(std::to_string(archived.recvQPS.max));

				row.push_back(std::to_string(archived.sendErrorCount));
				row.push_back(std::to_string(archived.recvErrorCount));

				row.push_back(std::to_string(archived.costUsecPerQuest.average));
				row.push_back(std::to_string(archived.costUsecPerQuest.min));
				row.push_back(std::to_string(archived.costUsecPerQuest.max));

				//-- "Interface QPS"
				row.push_back(std::to_string(_serverQPS[pp.first]));

				//-- "Server UDP Session Count"
				row.push_back(std::to_string(_serverSessions[pp.first]));
				row.push_back(std::to_string(_serverARQSessions[pp.first]));

				//-- "load", "load/cpu", "ping/2 (msec)", "Recv Bytes/s", "Send Bytes/s"
				ArchivedMachineStatus& ams = _archivedStatus[pp.first];
				row.push_back(ams.load.str());

				if (_cpu)
				{
					struct Range<double> loadcpu = ams.load;
					loadcpu.min /= _cpu;
					loadcpu.max /= _cpu;
					row.push_back(loadcpu.str());
				}
				else
					row.push_back("N/A");

				row.push_back(ams.halfPingMsec.str());
				row.push_back(formatBytesQuantity(ams.RX, 0));
				row.push_back(formatBytesQuantity(ams.TX, 0));

				rows.push_back(row);
			}
		}
	}

	std::unique_lock<std::mutex> lck(gc_outputMutex);
	cout<<endl<<"* Records will update by per minute *"<<endl<<endl;
	printTable(ArchivedRecordsFields, rows);
}


bool StressRecorder::saveToCSVFormat(const std::string& filename)
{
	std::stringstream ss;
	ss << "Record time (UTC), UTC Minute, Second, Stress Connection Count, ";
	ss << "Average Send QPS, Min Send QPS, Max Send QPS, ";
	ss << "Average Recv QPS, Min Recv QPS, Max Recv QPS, ";
	ss << "Average Send Error Count, Recv Error Count, ";
	ss << "Average Time Cost (usec), Min Time Cost (usec), Max Time Cost (usec), ";
	ss << "Interface QPS, Server UDP Session Count, Server UDP ARQ Session Count, load, load/cpu, ping/2 (msec), Recv Bytes/s, Send Bytes/s";

	{
		std::unique_lock<std::mutex> lck(_mutex);
		archive();
		archiveStatus(slack_real_sec());

		for (auto& pp: _archivedRecords)
		{
			for (auto& pp2: pp.second)
			{
				char buf[32];
				time_t rtime = (time_t)pp.first * 60 + pp2.first;
				std::string recordTime = ctime_r(&rtime, buf);
				recordTime = StringUtil::trim(recordTime);

				ArchivedRecord& archived = pp2.second;

				ss <<"\n";
				ss << recordTime << "," << pp.first << "," << pp2.first << "," << archived.clientCount << ",";

				ss << archived.sendQPS.average << "," << archived.sendQPS.min << "," << archived.sendQPS.max << ",";
				ss << archived.recvQPS.average << "," << archived.recvQPS.min << "," << archived.recvQPS.max << ",";

				ss << archived.sendErrorCount << "," << archived.recvErrorCount << ",";
				ss << archived.costUsecPerQuest.average << "," << archived.costUsecPerQuest.min << "," << archived.costUsecPerQuest.max << ",";

				//-- "Interface QPS"
				ss << _serverQPS[pp.first] << ",";

				//-- "Server UDP Session Count"
				ss << _serverSessions[pp.first] << ",";
				ss << _serverARQSessions[pp.first] << ",";

				//-- "load, load/cpu, ping/2 (msec), Recv Bytes/s, Send Bytes/s"
				ArchivedMachineStatus& ams = _archivedStatus[pp.first];
				ss << ams.load.str() << ",";
	
				if (_cpu)
				{
					struct Range<double> loadcpu = ams.load;
					loadcpu.min /= _cpu;
					loadcpu.max /= _cpu;
					ss << loadcpu.str() <<",";
				}
				else
					ss << "N/A,";

				ss << ams.halfPingMsec.str() << "," << formatBytesQuantity(ams.RX, 0) << "," << formatBytesQuantity(ams.TX, 0);
			}
		}
	}

	std::string content = ss.str();
	return FileSystemUtil::saveFileContent(filename, content);
}