#ifndef FPNN_Stress_Controller_h
#define FPNN_Stress_Controller_h

#include <vector>
#include "TCPClient.h"
#include "../CommonConstant.h"
#include "CtrlQuestProcessor.h"

#define DATS_DEPLOY_MAX_CPU_LOAD 0.80

using namespace fpnn;

extern std::mutex gc_outputMutex;

class StressController
{
	TCPClientPtr _client;
	CtrlQuestProcessorPtr _processor;
	std::string _targetHost;
	volatile bool _running;

private:
	struct LoadStatus
	{
		std::string endpoint;
		std::string host;
		int connCount;
		double load;
		int cpus;
		int64_t freeMemories;

		LoadStatus(): connCount(0), load(0.0), cpus(0), freeMemories(0) {}
	};

	struct MachineActorStatus
	{
		std::string host;
		int connCount;
		double load;
		int cpus;
		int64_t freeMemories;
		std::map<int, std::string> pidEpMap;

		MachineActorStatus(): connCount(0), load(0.0), cpus(0), freeMemories(0) {}
	};

private:
	//-- Utilities part
	std::string actorName() { return FPNN_Stress_Actor_Name; }
	int findIndex(const std::string& field, const std::vector<std::string>& fields);
	void formatMachineStatus(std::vector<std::string>& fields, std::vector<std::vector<std::string>>& rows);
	bool startMonitor();
	bool gathermachineStatus(const std::set<std::string>& allDeployerEndpoints, std::vector<struct LoadStatus>& deployers);
	bool gathermachineStatus(struct MachineActorStatus& deployerStatus, struct LoadStatus& testStatus);
	bool gathermachineStatus(std::map<std::string, struct MachineActorStatus>& deployerStatus, struct LoadStatus& testStatus);
	int monitorTargetServer(const std::string& host, struct MachineStatus& status);
	bool checkActorStatus(const std::string& actorMD5, bool& avaliableCenterCached, std::set<std::string>& allDeployerEndpoints, std::set<std::string>& useableEndpoints);
	std::string prepareUniqueId();
	std::string prepareActorParams(const std::string& uuid);
	void prepareAutoTest(std::string& actorInstanceName, std::string& launchParams, int& perConnCount, int& perQPS);
	void printActionHint(const std::string& hintLineInfo);
	void monitor();
	int interfaceQPS(TCPClientPtr client, const std::string& interface);

private:
	//-- action & follow part
	bool checkActor(std::set<std::string>& actorDeployEps);
	bool uploadActor(const std::string& content);
	bool deployActor(const std::set<std::string>& endpoints);
	int launchActor(const std::string& deployerEndpoint, const std::string& actorInstanceName, const std::string& launchParams, struct MachineActorStatus& status);
	bool sendAction(const std::string& actorInstanceName, const std::string& actorInstanceEndpoint, int pid, const std::string& method, FPWriter& payloadWriter, const std::string& desc);
	bool quitActor(const std::string& endpoint, const std::string& actorInstanceName, int pid);

public:
	bool init();
	void autoTest();
};

#endif