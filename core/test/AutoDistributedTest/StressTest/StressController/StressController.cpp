#include <iostream>
#include <sys/types.h>
#include <unistd.h>
#include "uuid.h"
#include "FPJson.h"
#include "ignoreSignals.h"
#include "CommandLineUtil.h"
#include "StressController.h"

using namespace std;
using namespace fpnn;

std::mutex gc_outputMutex;

int showUsage(const char* appName)
{
	cout<<"Usgae:"<<endl;
	cout<<"\t"<<appName<<"<required-params> [optional-params]"<<endl;
	cout<<endl;
	cout<<"  Required-params:"<<endl;
	cout<<"\t--contorlCenterEndpoint      DATS Control Center endpoint"<<endl;
	cout<<"\t--testEndpoint               Tested target server endpoint"<<endl;
	cout<<endl;
	cout<<"  Optional-params:"<<endl;
	cout<<"\t--actor                      Actor binary file path"<<endl;
	cout<<"\t--output                     CSV file name"<<endl;
	cout<<"\t--minCPUs                    Minimum required CPU cores' count. Default is 4"<<endl;
	cout<<"\t--stopPerCPULoad             Test will be stopped when load/cpu > this value consecutive 9 times. Default is 1.5"<<endl;
	cout<<"\t--stopTimeCostMsec           Test will be stopped when quest time cost > this value consecutive 9 times. Default is 600"<<endl;
	cout<<"\t--stressTimeout              Stress quest timeout in seconds. Default 300 seconds"<<endl;
	cout<<"\t--answerPoolThread           Client work threads count of actor. Default is CPU count"<<endl;
	cout<<"\t--perStressConnections       Total Connections for a stress task. Default is 100"<<endl;
	cout<<"\t--perStressQPS               Total QPS for a stress task. Default is 50000"<<endl;
	return -1;
}

bool StressController::init()
{
	int timecostThreshold = CommandLineParser::getInt("stopTimeCostMsec", 600);
	std::string endpoint = CommandLineParser::getString("contorlCenterEndpoint");
	_client = TCPClient::createClient(endpoint);
	if (!_client)
		return false;

	_processor = std::make_shared<CtrlQuestProcessor>();
	_processor->setTimecostThreshold(timecostThreshold * 1000);
	_client->setQuestProcessor(_processor);
	_client->connect();

	return true;
}

std::string StressController::prepareUniqueId()
{
	uuid_t uuidBuf;
	uuid_generate(uuidBuf);

	char strbuf[64];
	uuid_string(uuidBuf, strbuf, 64);

	return std::string(strbuf);
}

std::string StressController::prepareActorParams(const std::string& uuid)
{
	std::string actorParams;

	int timeout = CommandLineParser::getInt("stressTimeout", 300);
	int clientWorkThreads = CommandLineParser::getInt("answerPoolThread", 0);

	if (timeout > 0)
		actorParams.append(" --timeout ").append(std::to_string(timeout));

	if (clientWorkThreads > 0)
		actorParams.append(" --answerPoolThread ").append(std::to_string(clientWorkThreads));

	actorParams.append(" --uniqueId ").append(uuid);

	return actorParams;
}

void StressController::printActionHint(const std::string& hintLineInfo)
{
	cout<<"  .. "<<hintLineInfo<<" ..."<<endl;
}

void StressController::prepareAutoTest(std::string& actorInstanceName, std::string& launchParams, int& perConnCount, int& perQPS)
{
	std::string uuid = prepareUniqueId();
	launchParams = prepareActorParams(uuid);

	actorInstanceName = actorName();
	actorInstanceName.append("-").append(uuid);

	perConnCount = CommandLineParser::getInt("perStressConnections", 100);
	perQPS = CommandLineParser::getInt("perStressQPS", 50000);
}

int StressController::interfaceQPS(TCPClientPtr client, const std::string& interface)
{
	FPAnswerPtr answer = client->sendQuest(FPQWriter::emptyQuest("*infos"));
	if (answer)
	{
		std::string payload = answer->json();
		JsonPtr json = Json::parse(payload.c_str());

		std::string path("FPNN.status/server/stat/");
		path.append(interface).append("/QPS");
		return json->getInt(path, -1, "/");
	}
	else
		return -1;
}

void StressController::monitor()
{
	int cpu;
	struct MachineStatus ms;

	int qpsTicket = 0;
	std::string endpoint = CommandLineParser::getString("testEndpoint");
	TCPClientPtr targetSrvClient = TCPClient::createClient(endpoint);
	targetSrvClient->connect();

	while (_running)
	{
		cpu = monitorTargetServer(_targetHost, ms);
		if (cpu > 0)
		{
			_processor->stressRecorder.addMachineStatus(cpu, ms);
			std::unique_lock<std::mutex> lck(gc_outputMutex);
			cout<<"[Target Server] load: "<<ms.load<<", connCount: "<<ms.connCount<<endl;
		}

		sleep(2);

		qpsTicket += 2;
		if (qpsTicket >= 20)
		{
			qpsTicket = 0;
			int QPS = interfaceQPS(targetSrvClient, "two way demo");
			if (QPS > 0)
			{
				_processor->stressRecorder.addQPS(QPS);
				std::unique_lock<std::mutex> lck(gc_outputMutex);
				cout<<"[Target Server] QPS: "<<QPS<<endl;
			}
		}
	}

	int delayForThisMintueAchived = 62;
	while (delayForThisMintueAchived > 0)
	{
		cpu = monitorTargetServer(_targetHost, ms);
		if (cpu > 0)
		{
			_processor->stressRecorder.addMachineStatus(cpu, ms);
			std::unique_lock<std::mutex> lck(gc_outputMutex);
			cout<<"[Target Server] load: "<<ms.load<<", connCount: "<<ms.connCount<<endl;
		}

		sleep(2);
		delayForThisMintueAchived -= 2;

		qpsTicket += 2;
		if (qpsTicket >= 20)
		{
			qpsTicket = 0;
			int QPS = interfaceQPS(targetSrvClient, "two way demo");
			if (QPS > 0)
			{
				_processor->stressRecorder.addQPS(QPS);
				std::unique_lock<std::mutex> lck(gc_outputMutex);
				cout<<"[Target Server] QPS: "<<QPS<<endl;
			}
		}
	}
}

void StressController::autoTest()
{
	if (!startMonitor())
		return;

	std::set<std::string> actorDeployEps;
	if (!checkActor(actorDeployEps))
		return;

	int perQPS;
	int perConnCount;
	std::string launchParams;
	std::string actorInstanceName;
	
	prepareAutoTest(actorInstanceName, launchParams, perConnCount, perQPS);

	struct LoadStatus targetStatus;
	{
		int port;
		targetStatus.endpoint = CommandLineParser::getString("testEndpoint");
		if (!parseAddress(targetStatus.endpoint, targetStatus.host, port))
		{
			cout<<"Prepare target server info error. Endpoint "<<targetStatus.endpoint<<" is invalid."<<endl;
			return;
		}
		_targetHost = targetStatus.host;
	}

	std::map<std::string, struct MachineActorStatus> actorStatus;
	{
		for (auto& deployerEndpoint: actorDeployEps)
		{
			struct MachineActorStatus& macStatus = actorStatus[deployerEndpoint];
			int port;
			parseAddress(deployerEndpoint, macStatus.host, port);
		}

		if (!gathermachineStatus(actorStatus, targetStatus))
			return;
	}

	_running = true;
	const char* stopReason = "";
	std::thread monitorThread(&StressController::monitor, this);

	cout<<"============================[ stress test start ]====================================="<<endl;

	int overloadCount = 0;
	int newInstanceCount = -1;
	double loadThreshold = CommandLineParser::getReal("stopPerCPULoad", 1.5);	//-- 4 core load >= 6, 8 core load >= 12
	while (newInstanceCount && _running)
	{
		newInstanceCount = 0;
		for (auto& deployerEndpoint: actorDeployEps)
		{
			struct MachineActorStatus& macStatus = actorStatus[deployerEndpoint];

			if (macStatus.load/macStatus.cpus >= DATS_DEPLOY_MAX_CPU_LOAD)
				continue;
		
			printActionHint("launch new stress source");

			int pid = launchActor(deployerEndpoint, actorInstanceName, launchParams, macStatus);
			if (pid == 0)
				continue;

			newInstanceCount++;

			FPWriter pw(3);
			pw.param("endpoint", targetStatus.endpoint);
			pw.param("connections", perConnCount);
			pw.param("totalQPS", perQPS);

			printActionHint("add new stress");
			sendAction(actorInstanceName, macStatus.pidEpMap[pid], pid, "beginStress", pw, "Stress instance");
			sleep(1);

			int intervalSeconds = 5 * 60;
			while (intervalSeconds > 0 && _running)
			{
				sleep(2);
				intervalSeconds -= 2;
				gathermachineStatus(macStatus, targetStatus);

				_running = !_processor->needStop();
				if (!_running)
					stopReason = "quest time cost catch threshold.";

				if (targetStatus.load/targetStatus.cpus > loadThreshold)
				{
					overloadCount += 1;
					if (overloadCount > 9)
					{
						stopReason = "target server per CPU load catch threshold.";
						_running = false;
					}
				}
				else
					overloadCount = 0;

				if (targetStatus.freeMemories < 1024 * 1024 * 100)	//-- 100 MB free
				{
					stopReason = "target server free memories catch threshold.";
					_running = false;
				}
			}

			if (!_running)
				break;
		}

		if (newInstanceCount == 0)
		{
			_running = false;
			stopReason = "no available deployer/actor machine can be scheduled.";
		}
	}

	{
		std::unique_lock<std::mutex> lck(gc_outputMutex);
		cout<<"Test will be stopped, "<<stopReason<<" Wait 60 seconds for gathering and archiving the last minute data ..."<<endl;
	}
	//-- wait 60 second for merge last minute data.
	monitorThread.join();

	std::string csvFilename = CommandLineParser::getString("output");
	if (csvFilename.size())
		_processor->stressRecorder.saveToCSVFormat(csvFilename);

	//-- stop all actor
	//-- std::map<std::string, struct MachineActorStatus> actorStatus;
	for (auto& pp: actorStatus)
	{
		for (auto& pp2: pp.second.pidEpMap)
			quitActor(pp2.second, actorInstanceName, pp2.first);
	}

	_processor->stressRecorder.showRecords();
}

int main(int argc, const char* argv[])
{
	ignoreSignals();
	ClientEngine::configAnswerCallbackThreadPool(2, 1, 2, 4);
	ClientEngine::configQuestProcessThreadPool(0, 1, 2, 10, 0);

	CommandLineParser::init(argc, argv);
	
	StressController controller;
	if (!controller.init())
		return showUsage(argv[0]);

	controller.autoTest();

	return 0;
}