#ifndef FPNN_Standard_Stress_Source_h
#define FPNN_Standard_Stress_Source_h

#include <stdint.h>
#include <atomic>
#include <vector>
#include <thread>
#include "UDPClient.h"
#include "ControlCenter.h"

using namespace fpnn;

class StressSource
{
	std::string _endpoint;
	std::atomic<bool> _running;
	std::atomic<int64_t> _send;
	std::atomic<int64_t> _recv;
	std::atomic<int64_t> _sendError;
	std::atomic<int64_t> _recvError;
	std::atomic<int64_t> _timecost;

	std::vector<std::thread> _threads;
	std::vector<std::vector<UDPClientPtr>> _clients;

	int _sleepSec;
	int _sleepUsec;
	bool _sleepBySecond;

	int _firstWaitMinute;
	int _intervalMinute;
	int _decSleepMsec;
	int _minSleepMsec;

	void adjustStress(int64_t &waitingMsec);
	void test_worker(int idx);

protected:
	int _taskId;
	std::string _region;

public:
	StressSource(int taskId, const std::string& region, const std::string& endpoint,
		int firstWaitMinute = 0, int intervalMinute = 0, int decSleepMsec = 0, int minSleepMsec = 0):
		_endpoint(endpoint), _running(false), _send(0), _recv(0), _sendError(0), _recvError(0), _timecost(0),
		_sleepSec(1), _sleepUsec(1000), _sleepBySecond(false),
		_firstWaitMinute(firstWaitMinute), _intervalMinute(intervalMinute), _decSleepMsec(decSleepMsec), _minSleepMsec(minSleepMsec),
		_taskId(taskId), _region(region)
	{}
	virtual ~StressSource()
	{
		stop();
	}

	void stop()
	{
		_running = false;
		for(size_t i = 0; i < _threads.size(); i++)
			_threads[i].join();
	}

	inline void incRecv() { _recv++; }
	inline void incRecvError() { _recvError++; }
	inline void addTimecost( int64_t cost) { _timecost.fetch_add(cost); }

	bool launch(int threadCount, int clientCount, double perClientQPS);
	void reportStatistics(int clientCount);
};
typedef std::shared_ptr<StressSource> StressSourcePtr;

#endif