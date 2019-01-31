#ifndef FPNN_Standard_Stress_Source_h
#define FPNN_Standard_Stress_Source_h

#include <stdint.h>
#include <atomic>
#include <vector>
#include <thread>
#include "ControlCenter.h"

using namespace fpnn;

struct EncryptInfo
{
	bool ssl;
	std::string curveName;
	std::string publicKey;
	bool packageMode;
	bool reinforce;

	EncryptInfo(): ssl(false), packageMode(true), reinforce(false) {}
};

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
	struct EncryptInfo _encryptInfo;

	void processEncrypt(TCPClientPtr client);
	void test_worker(int qps);

protected:
	int _taskId;
	std::string _region;

public:
	StressSource(int taskId, const std::string& region, const std::string& endpoint): _endpoint(endpoint),
		_running(false), _send(0), _recv(0), _sendError(0), _recvError(0), _timecost(0), _taskId(taskId), _region(region)
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

	bool launch(int connections, int totalQPS);
	void reportStatistics(int clientCount);
	void checkEncryptInfo(const FPReaderPtr payload);
};
typedef std::shared_ptr<StressSource> StressSourcePtr;

#endif