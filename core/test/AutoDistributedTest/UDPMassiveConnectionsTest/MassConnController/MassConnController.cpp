#include <iostream>
#include <sys/types.h>
#include <unistd.h>
#include "uuid.h"
#include "FPJson.h"
#include "ignoreSignals.h"
#include "CommandLineUtil.h"
#include "MassConnController.h"

using namespace std;
using namespace fpnn;

std::mutex gc_outputMutex;
std::atomic<bool> gc_showDetail(false);
volatile bool StressController::_running = false;

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
	cout<<"\t--massThreadCount            Threads' count for per massive test task. Default is 1000"<<endl;
	cout<<"\t--clientCount                Clients' count for per massive test task. Default is 20000"<<endl;
	cout<<"\t--perClientQPS               Per client QPS. Default is 0.02"<<endl;
	cout<<"\t--maxClientsPerMachine       Maximum clients count per machine. Default is 60000"<<endl;
	cout<<"\t--stopDelayWhenPeakOccur     Test will be stopped if in this mintues no records caught connection count peak. Default is 5 minutes"<<endl;
	cout<<"\t--connPeakTolerance          Tolerance for recording connection count peak. Default is 2000"<<endl;
	cout<<"\t--extraWaitTime              Extra wait time in minutes when no available deployer/actor machine can be scheduled. Default is 0 minute"<<endl;
	cout<<"\t--enableAutoBoost            Enable auto boost stress mode, no value."<<endl;
	cout<<"\t--autoBoostFirstPeriod       If enable auto boost stress mode, the first boost interval in minute. Default is 10 minute"<<endl;
	cout<<"\t--autoBoostPeriod            If enable auto boost stress mode, the boost interval in minute. Default is 10 minute"<<endl;
	cout<<"\t--autoBoostDecMsec           If enable auto boost stress mode, decrease pointed milliseconds for sleeping after every period. Default is 5 milliseconds"<<endl;
	cout<<"\t--autoBoostMinSleepMsec      If enable auto boost stress mode, this is the mimimum milliseconds for sleeping. Default is 5 milliseconds"<<endl;
	return -1;
}

bool StressController::init()
{
	installStopSignalController();

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

void StressController::installStopSignalController()
{
	signal(SIGINT, &StressController::stopSignalHandler);
	signal(SIGTERM, &StressController::stopSignalHandler);
	signal(SIGQUIT, &StressController::stopSignalHandler);
	signal(SIGUSR1, &StressController::showDetail);
	signal(SIGUSR2, &StressController::showDetail);
}

void StressController::stopSignalHandler(int sig)
{
	_running = false;
}

void StressController::showDetail(int sig)
{
	gc_showDetail = !gc_showDetail;
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

void StressController::prepareAutoTest(std::string& actorInstanceName, std::string& launchParams,
	int& massThreadCount, int& clientCount, double& perClientQPS)
{
	std::string uuid = prepareUniqueId();
	launchParams = prepareActorParams(uuid);

	actorInstanceName = actorName();
	actorInstanceName.append("-").append(uuid);

	massThreadCount = CommandLineParser::getInt("massThreadCount", 1000);
	clientCount = CommandLineParser::getInt("clientCount", 20000);
	perClientQPS = CommandLineParser::getReal("perClientQPS", 0.02);
}

void StressController::targetServerInfos(UDPClientPtr client, const std::string& interface)
{
	FPAnswerPtr answer = client->sendQuest(FPQWriter::emptyQuest("*infos"));
	if (answer)
	{
		if (answer->status())
		{
			std::unique_lock<std::mutex> lck(gc_outputMutex);
			cout<<"[Target Server] fetch infos failed. Info: "<<answer->json()<<endl;
			return;
		}

		std::string payload = answer->json();
		JsonPtr json = Json::parse(payload.c_str());

		std::string path("FPNN.status/server/stat/");
		path.append(interface).append("/QPS");
		int QPS = json->getInt(path, -1, "/");

		int currentSessions = json->getInt("FPNN.status/server/status/udp/currentSessions", -1, "/");
		int currnetARQConnections = json->getInt("FPNN.status/server/status/udp/currnetARQConnections", -1, "/");

		if (QPS > 0)
			_processor->stressRecorder.addQPS(QPS);

		if (currentSessions > 0)
			_processor->stressRecorder.addSessions(currentSessions);

		if (currnetARQConnections > 0)
			_processor->stressRecorder.addARQSessions(currnetARQConnections);
		
		std::unique_lock<std::mutex> lck(gc_outputMutex);
		cout<<"[Target Server] QPS: "<<QPS<<", UDP session: "<<currentSessions<<", ARQ session: "<<currnetARQConnections<<endl;
	}
}

void StressController::monitor()
{
	int cpu;
	struct MachineStatus ms;

	int qpsTicket = 0;
	std::string endpoint = CommandLineParser::getString("testEndpoint");
	UDPClientPtr targetSrvClient = UDPClient::createClient(endpoint);
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
			targetServerInfos(targetSrvClient, "two way demo");
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
			targetServerInfos(targetSrvClient, "two way demo");
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

	systemCmd(actorDeployEps, "echo \"5001 65100\" > /proc/sys/net/ipv4/ip_local_port_range");

	int massThreadCount;
	int clientCount;
	double perClientQPS;
	std::string launchParams;
	std::string actorInstanceName;
	
	prepareAutoTest(actorInstanceName, launchParams, massThreadCount, clientCount, perClientQPS);

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

		targetStatus.connCount.tolerance = CommandLineParser::getInt("connPeakTolerance", 2000);
	}

	std::map<std::string, int> deployerTaskCount;
	std::map<std::string, struct MachineActorStatus> actorStatus;
	{
		for (auto& deployerEndpoint: actorDeployEps)
		{
			struct MachineActorStatus& macStatus = actorStatus[deployerEndpoint];
			int port;
			parseAddress(deployerEndpoint, macStatus.host, port);
			deployerTaskCount[deployerEndpoint] = 0;
		}

		if (!gathermachineStatus(actorStatus, targetStatus))
			return;
	}

	int peakDelay = CommandLineParser::getInt("stopDelayWhenPeakOccur", 5);
	peakDelay *= 60;

	_running = true;
	const char* stopReason = "";
	std::thread monitorThread(&StressController::monitor, this);

	cout<<"============================[ stress test start ]====================================="<<endl;

	int taskCount = 0;
	int overloadCount = 0;
	int newInstanceCount = -1;
	int extraWaitTime = 0;
	int maxDeployerTaskCount = CommandLineParser::getInt("maxClientsPerMachine", 60000) / clientCount;
	double loadThreshold = CommandLineParser::getReal("stopPerCPULoad", 1.5);	//-- 4 core load >= 6, 8 core load >= 12
	while (newInstanceCount && _running)
	{
		newInstanceCount = 0;
		for (auto& deployerEndpoint: actorDeployEps)
		{
			if (deployerTaskCount[deployerEndpoint] >= maxDeployerTaskCount)
				continue;

			struct MachineActorStatus& macStatus = actorStatus[deployerEndpoint];

			if (macStatus.load/macStatus.cpus >= DATS_DEPLOY_MAX_CPU_LOAD)
				continue;

			printActionHint("launch new stress source");

			int pid = launchActor(deployerEndpoint, actorInstanceName, launchParams, macStatus);
			if (pid == 0)
				continue;

			deployerTaskCount[deployerEndpoint] += 1;
			newInstanceCount++;
			taskCount++;

			if (CommandLineParser::exist("enableAutoBoost"))
			{
				int autoBoostFirstPeriod = CommandLineParser::getInt("autoBoostFirstPeriod", 10);
				int autoBoostPeriod = CommandLineParser::getInt("autoBoostPeriod", 10);
				int autoBoostDecMsec = CommandLineParser::getInt("autoBoostDecMsec", 5);
				int autoBoostMinSleepMsec = CommandLineParser::getInt("autoBoostMinSleepMsec", 5);

				FPWriter pw(8);
				pw.param("endpoint", targetStatus.endpoint);
				pw.param("threadCount", massThreadCount);
				pw.param("clientCount", clientCount);
				pw.param("perClientQPS", perClientQPS);

				pw.param("firstWaitMinute", autoBoostFirstPeriod);
				pw.param("intervalMinute", autoBoostPeriod);
				pw.param("decSleepMsec", autoBoostDecMsec);
				pw.param("minSleepMsec", autoBoostMinSleepMsec);

				printActionHint("add new auto boost stress");
				sendAction(actorInstanceName, macStatus.pidEpMap[pid], pid, "autoBoostStress", pw, "Auto boost stress instance");
			}
			else
			{
				FPWriter pw(4);
				pw.param("endpoint", targetStatus.endpoint);
				pw.param("threadCount", massThreadCount);
				pw.param("clientCount", clientCount);
				pw.param("perClientQPS", perClientQPS);

				printActionHint("add new stress");
				sendAction(actorInstanceName, macStatus.pidEpMap[pid], pid, "beginStress", pw, "Stress instance");
			}
			sleep(1);

			waitCatchConnections(clientCount * taskCount, targetStatus, peakDelay, loadThreshold, overloadCount, stopReason);

			if (!_running)
				break;
		}

		if (newInstanceCount == 0)
		{
			extraWaitTime = CommandLineParser::getInt("extraWaitTime") * 60;
			stopReason = "no available deployer/actor machine can be scheduled.";
			cout<<"Test will be stopped, "<<stopReason<<" Wait extra "<<extraWaitTime<<" seconds for recording."<<endl;

			waitIntervalSeconds(extraWaitTime, targetStatus, peakDelay, loadThreshold, overloadCount, stopReason);
			_running = false;
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

void StressController::waitCatchConnections(int connections, struct LoadStatus& targetStatus,
	int peakDelay, double loadThreshold, int &overloadCount, const char* &stopReason)
{
	struct MachineActorStatus machineStatus;

	while (_running)
	{
		sleep(2);
		gathermachineStatus(machineStatus, targetStatus);

		bool needStop = _processor->needStop();
		if (needStop)
		{
			_running = false;
			stopReason = "quest time cost catch threshold.";
		}

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

		if (slack_real_sec() - targetStatus.connCount.occurredSeconds > peakDelay)
		{
			stopReason = "No new peak or peak don't be caught in prescribed time.";
			_running = false;
		}

		if (targetStatus.connCount > connections)
			return;
	}
}

void StressController::waitIntervalSeconds(int intervalSeconds, struct LoadStatus& targetStatus,
	int peakDelay, double loadThreshold, int &overloadCount, const char* &stopReason)
{
	struct MachineActorStatus machineStatus;

	while (intervalSeconds > 0 && _running)
	{
		sleep(2);
		intervalSeconds -= 2;
		gathermachineStatus(machineStatus, targetStatus);

		bool needStop = _processor->needStop();
		if (needStop)
		{
			_running = false;
			stopReason = "quest time cost catch threshold.";
		}

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

		if (slack_real_sec() - targetStatus.connCount.occurredSeconds > peakDelay)
		{
			stopReason = "No new peak or peak don't be caught in prescribed time.";
			_running = false;
		}
	}
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