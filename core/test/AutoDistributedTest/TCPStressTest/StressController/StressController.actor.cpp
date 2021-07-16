#include <iostream>
#include "CommandLineUtil.h"
#include "FormattedPrint.h"
#include "FileSystemUtil.h"
#include "NetworkUtility.h"
#include "CtrlQuestProcessor.h"
#include "StressController.h"

using namespace std;
using namespace fpnn;

int StressController::findIndex(const std::string& field, const std::vector<std::string>& fields)
{
	for (size_t i = 0; i < fields.size(); i++)
		if (fields[i] == field)
			return (int)i;

	return -1;
}

void StressController::formatMachineStatus(std::vector<std::string>& fields, std::vector<std::vector<std::string>>& rows)
{
	int cpuIdx = findIndex("cpus", fields);
	int loadIdx = findIndex("load", fields);
	int memIdx = findIndex("memories", fields);
	int freeMemIdx = findIndex("freeMemories", fields);
	int rxIdx = findIndex("RX", fields);
	int txIdx = findIndex("TX", fields);

	fields.push_back("load/cpus");

	for (auto& row: rows)
	{
		int cpus = atoi(row[cpuIdx].c_str());
		double load = atof(row[loadIdx].c_str());

		if (cpus)
			row.push_back(std::to_string(load/cpus));
		else
			row.push_back("N/A");

		row[memIdx] = formatBytesQuantity(atoll(row[memIdx].c_str()), 2);
		row[freeMemIdx] = formatBytesQuantity(atoll(row[freeMemIdx].c_str()), 2);
		row[rxIdx] = formatBytesQuantity(atoll(row[rxIdx].c_str()), 2);
		row[txIdx] = formatBytesQuantity(atoll(row[txIdx].c_str()), 2);
	}
}

bool StressController::startMonitor()
{
	FPQWriter qw(1, "monitorMachineStatus");
	qw.param("monitor", true);

	FPAnswerPtr answer = _client->sendQuest(qw.take());
	FPAReader ar(answer);
	if (ar.status())
	{
		cout<<"[Start machines monitor][Exception] Error code: "<<ar.wantInt("code")<<", ex: "<<ar.wantString("ex")<<endl;
		return false;
	}
	return true;
}

bool StressController::gathermachineStatus(const std::set<std::string>& allDeployerEndpoints, std::vector<struct LoadStatus>& deployers)
{
	std::map<std::string, std::string> hostEpMap;
	for (auto& endpoint: allDeployerEndpoints)
	{
		int port;
		std::string host;

		if (parseAddress(endpoint, host, port))
			hostEpMap[host] = endpoint;
	}

	FPAnswerPtr answer = _client->sendQuest(FPQWriter::emptyQuest("machineStatus"));
	FPAReader ar(answer);
	if (ar.status())
	{
		cout<<"[Query machines status][Exception] Error code: "<<ar.wantInt("code")<<", ex: "<<ar.wantString("ex")<<endl;
		return false;
	}

	std::vector<std::string> fields = ar.want("fields", std::vector<std::string>());
	std::vector<std::vector<std::string>> rows = ar.want("rows", std::vector<std::vector<std::string>>());

	int sourceIdx = findIndex("source", fields);
	int hostIdx = findIndex("host", fields);
	int loadIdx = findIndex("load", fields);
	int memIdx = findIndex("freeMemories", fields);
	int connIdx = findIndex("tcpCount", fields);
	int cpusIdx = findIndex("cpus", fields);

	if (sourceIdx < 0 || hostIdx < 0 || loadIdx < 0 || memIdx < 0 || connIdx < 0 || cpusIdx < 0)
	{
		cout<<"[Exception] Machines status columns error. Cannot find 'source' or 'host' or 'load' or 'freeMemories' or 'tcpCount'."<<endl;
		return false;
	}

	for (auto& row: rows)
	{
		if (row[sourceIdx] != "Deployer")
			continue;

		struct LoadStatus ls;
		ls.host = row[hostIdx];
		ls.endpoint = hostEpMap[row[hostIdx]];

		if (ls.endpoint.empty())
			continue;

		ls.connCount = std::stoi(row[connIdx]);
		ls.load = std::stod(row[loadIdx]);
		ls.freeMemories = std::stoll(row[memIdx]);
		ls.cpus = std::stoi(row[cpusIdx]);

		deployers.push_back(ls);
	}

	return true;
}

bool StressController::gathermachineStatus(struct MachineActorStatus& deployerStatus, struct LoadStatus& testStatus)
{
	FPAnswerPtr answer = _client->sendQuest(FPQWriter::emptyQuest("machineStatus"));
	FPAReader ar(answer);
	if (ar.status())
	{
		cout<<"[Query machines status][Exception] Error code: "<<ar.wantInt("code")<<", ex: "<<ar.wantString("ex")<<endl;
		return false;
	}

	std::vector<std::string> fields = ar.want("fields", std::vector<std::string>());
	std::vector<std::vector<std::string>> rows = ar.want("rows", std::vector<std::vector<std::string>>());

	int sourceIdx = findIndex("source", fields);
	int hostIdx = findIndex("host", fields);
	int loadIdx = findIndex("load", fields);
	int memIdx = findIndex("freeMemories", fields);
	int connIdx = findIndex("tcpCount", fields);
	int cpusIdx = findIndex("cpus", fields);

	for (auto& row: rows)
	{
		if (row[sourceIdx] == "Deployer" && deployerStatus.host == row[hostIdx])
		{
			deployerStatus.connCount = std::stoi(row[connIdx]);
			deployerStatus.load = std::stod(row[loadIdx]);
			deployerStatus.freeMemories = std::stoll(row[memIdx]);
			deployerStatus.cpus = std::stoi(row[cpusIdx]);
		}
		else if (row[sourceIdx] == "Monitor" && testStatus.host == row[hostIdx])
		{
			testStatus.connCount = std::stoi(row[connIdx]);
			testStatus.load = std::stod(row[loadIdx]);
			testStatus.freeMemories = std::stoll(row[memIdx]);
			testStatus.cpus = std::stoi(row[cpusIdx]);
		}
	}

	formatMachineStatus(fields, rows);
	std::unique_lock<std::mutex> lck(gc_outputMutex);
	printTable(fields, rows);

	return true;
}
bool StressController::gathermachineStatus(std::map<std::string, struct MachineActorStatus>& deployerStatus, struct LoadStatus& testStatus)
{
	FPAnswerPtr answer = _client->sendQuest(FPQWriter::emptyQuest("machineStatus"));
	FPAReader ar(answer);
	if (ar.status())
	{
		cout<<"[Query machines status][Exception] Error code: "<<ar.wantInt("code")<<", ex: "<<ar.wantString("ex")<<endl;
		return false;
	}

	std::vector<std::string> fields = ar.want("fields", std::vector<std::string>());
	std::vector<std::vector<std::string>> rows = ar.want("rows", std::vector<std::vector<std::string>>());

	int sourceIdx = findIndex("source", fields);
	int hostIdx = findIndex("host", fields);
	int loadIdx = findIndex("load", fields);
	int memIdx = findIndex("freeMemories", fields);
	int connIdx = findIndex("tcpCount", fields);
	int cpusIdx = findIndex("cpus", fields);

	for (auto& row: rows)
	{
		if (row[sourceIdx] == "Deployer")
		{
			for (auto& pp: deployerStatus)
			{
				if (pp.second.host == row[hostIdx])
				{
					pp.second.connCount = std::stoi(row[connIdx]);
					pp.second.load = std::stod(row[loadIdx]);
					pp.second.freeMemories = std::stoll(row[memIdx]);
					pp.second.cpus = std::stoi(row[cpusIdx]);
					break;
				}
			}		
		}
		else if (row[sourceIdx] == "Monitor" && testStatus.host == row[hostIdx])
		{
			testStatus.connCount = std::stoi(row[connIdx]);
			testStatus.load = std::stod(row[loadIdx]);
			testStatus.freeMemories = std::stoll(row[memIdx]);
			testStatus.cpus = std::stoi(row[cpusIdx]);
		}
	}

	formatMachineStatus(fields, rows);
	std::unique_lock<std::mutex> lck(gc_outputMutex);
	printTable(fields, rows);

	return true;
}

int StressController::monitorTargetServer(const std::string& host, struct MachineStatus& status)
{
	FPAnswerPtr answer = _client->sendQuest(FPQWriter::emptyQuest("machineStatus"));
	FPAReader ar(answer);
	if (ar.status())
	{
		cout<<"[Monitor target server status][Exception] Error code: "<<ar.wantInt("code")<<", ex: "<<ar.wantString("ex")<<endl;
		return 0;
	}

	std::vector<std::string> fields = ar.want("fields", std::vector<std::string>());
	std::vector<std::vector<std::string>> rows = ar.want("rows", std::vector<std::vector<std::string>>());

	int sourceIdx = findIndex("source", fields);
	int hostIdx = findIndex("host", fields);
	int loadIdx = findIndex("load", fields);
	int pingIdx = findIndex("ping/2 (msec)", fields);
	int connIdx = findIndex("tcpCount", fields);
	int cpusIdx = findIndex("cpus", fields);
	int rxIdx = findIndex("RX", fields);
	int txIdx = findIndex("TX", fields);

	for (auto& row: rows)
	{
		if (row[sourceIdx] == "Monitor" && host == row[hostIdx])
		{
			status.load = std::stod(row[loadIdx]);
			status.connCount = std::stoi(row[connIdx]);
			status.halfPingMsec = std::stoi(row[pingIdx]);
			status.RX = std::stoll(row[rxIdx]);
			status.TX = std::stoll(row[txIdx]);

			return std::stoi(row[cpusIdx]);
		}
	}

	return 0;
}

bool StressController::checkActorStatus(const std::string& actorMD5, bool& avaliableCenterCached, std::set<std::string>& allDeployerEndpoints, std::set<std::string>& useableEndpoints)
{
	FPAnswerPtr answer = _client->sendQuest(FPQWriter::emptyQuest("availableActors"));
	FPAReader ar(answer);
	if (ar.status())
	{
		cout<<"[checkActorStatus][Exception] Error code: "<<ar.wantInt("code")<<", ex: "<<ar.wantString("ex")<<endl;
		return false;
	}

	std::string standardMD5(actorMD5);
	avaliableCenterCached = false;
	//-- fetch actors infos on ControlCenter.
	{
		OBJECT availableActors = ar.getObject("availableActors");
		FPReader ar2(availableActors);
		std::vector<std::string> fields = ar2.want("fields", std::vector<std::string>());
		std::vector<std::vector<std::string>> rows = ar2.want("rows", std::vector<std::vector<std::string>>());

		int nameIdx = findIndex("name", fields);
		int md5Idx = findIndex("md5", fields);

		for (auto& row: rows)
		{
			if (row[nameIdx] == actorName())
			{
				if (actorMD5.empty())
				{
					standardMD5 = row[md5Idx];
					avaliableCenterCached = true;
				}
				else if (actorMD5 == row[md5Idx])
					avaliableCenterCached = true;

				break;
			}
		}
	}

	{
		OBJECT deployedActors = ar.getObject("deployedActors");
		FPReader ar2(deployedActors);
		std::vector<std::string> fields = ar2.want("fields", std::vector<std::string>());
		std::vector<std::vector<std::string>> rows = ar2.want("rows", std::vector<std::vector<std::string>>());

		if (rows.empty())
		{
			cout<<"[checkActorStatus][Exception] No endpoint can be used. Please prepare test machines for continue."<<endl;
			return false;
		}

		int epIdx = findIndex("endpoint", fields);
		int nameIdx = findIndex("actorName", fields);
		int md5Idx = findIndex("md5", fields);

		for (auto& row: rows)
		{
			allDeployerEndpoints.insert(row[epIdx]);

			if (row[nameIdx] == actorName())
			{
				if (standardMD5.empty())
				{
					standardMD5 = row[md5Idx];
					useableEndpoints.insert(row[epIdx]);
				}
				else if (standardMD5 != row[md5Idx])
				{
					if (!avaliableCenterCached)
					{
						useableEndpoints.clear();
						cout<<"[checkActorStatus][Exception] No endpoint can be used. Please prepare test machines for continue."<<endl;
						return true;
					}
				}
				else
					useableEndpoints.insert(row[epIdx]);
			}
		}
	}

	return true;
}

bool StressController::checkActor(std::set<std::string>& actorDeployEps)
{
	std::string actorPath = CommandLineParser::getString("actor");

	FileSystemUtil::FileAttrs attrs;
	if (actorPath.size() && !FileSystemUtil::readFileAndAttrs(actorPath, attrs))
	{
		cout<<"Load "<<actorName()<<" at "<<actorPath<<" failed."<<endl;
		return false;
	}

	printActionHint("check actor deploying status");

	bool avaliableCenterCached;
	std::set<std::string> useableEndpoints;
	std::set<std::string> allDeployerEndpoints;
	if (!checkActorStatus(attrs.sign, avaliableCenterCached, allDeployerEndpoints, useableEndpoints))
		return false;

	//-- Does need to upload?
	if (attrs.sign.size() && !avaliableCenterCached)
	{
		printActionHint("begin upload actor to Control Center");
		if (!uploadActor(attrs.content))
			return false;
	}

	//-- Is need to deploy actor?
	printActionHint("gather deployer machines' load status");
	std::vector<struct LoadStatus> deployers;
	if (!gathermachineStatus(allDeployerEndpoints, deployers))
	{
		cout<<"gather deployer machines' load status failed."<<endl;
		return false;
	}

	int minCPUs = CommandLineParser::getInt("minCPUs", 4);
	std::set<std::string> needDeployEndpoints;
	for (auto& ls: deployers)
	{
		if (ls.cpus < minCPUs || ls.load/ls.cpus > DATS_DEPLOY_MAX_CPU_LOAD)
			useableEndpoints.erase(ls.endpoint);
		else
		{
			actorDeployEps.insert(ls.endpoint);

			if (useableEndpoints.find(ls.endpoint) == useableEndpoints.end())
				needDeployEndpoints.insert(ls.endpoint);
		}
	}

	if (needDeployEndpoints.size())
		printActionHint("deploy actor to machines");
		
	return deployActor(needDeployEndpoints);
}

bool StressController::uploadActor(const std::string& content)
{
	const size_t gc_maxTransportLength = 2 * 1024 * 1024;

	size_t parts = content.length() / gc_maxTransportLength;
	if (content.length() % gc_maxTransportLength)
		parts += 1;

	UploadTaskStatusPtr uploadStatus = _processor->uploadTaskStatus();
	uploadStatus->total = (int)parts;

	size_t remain = content.length();
	size_t offset = 0;
	for (size_t i = 0; i < parts; i++)
	{
		int no = i + 1;
		FPQWriter qw(4, "uploadActor");
		qw.param("name", actorName());
		qw.paramBinary("section", content.data() + offset, (remain > gc_maxTransportLength) ? gc_maxTransportLength : remain);
		qw.param("count", parts);
		qw.param("no", no);

		bool status = _client->sendQuest(qw.take(), [uploadStatus](FPAnswerPtr answer, int errorCode){
			if (errorCode != FPNN_EC_OK && errorCode != FPNN_EC_CORE_TIMEOUT)
				uploadStatus->failedCount++;
		}, i * 5);
		if (!status)
			uploadStatus->failedCount++;

		remain -= gc_maxTransportLength;
		offset += gc_maxTransportLength;

		usleep(20 * 1000);
	}

	int sleepTicket = 0;
	while (true)
	{
		sleep(1);
		sleepTicket++;

		if (uploadStatus->completed)
		{
			if (uploadStatus->lastStatus)
				return true;

			cout<<"Upload Actor "<<actorName()<<" failed."<<endl;
			return false;
		}

		if (uploadStatus->total == uploadStatus->failedCount)
		{
			cout<<"Upload Actor "<<actorName()<<" failed."<<endl;
			return false;
		}

		if ((sleepTicket % 5) == 0)
			cout<<"Upload "<<actorName()<<", please wait ..."<<endl;

		if (sleepTicket == 10)
		{
			sleepTicket = 0;
			_client->sendQuest(FPQWriter::emptyQuest("ping"), [](FPAnswerPtr answer, int errorCode){});
		}
	}
}

bool StressController::deployActor(const std::set<std::string>& endpoints)
{
	if (endpoints.empty())
		return true;

	FPQWriter qw(2, "deploy");
	qw.param("endpoints", endpoints);
	qw.param("actor", actorName());

	FPAnswerPtr answer = _client->sendQuest(qw.take());
	if (answer->status())
	{
		FPAReader ar(answer);
		cout<<"[Deploy Actor][Exception] Error code: "<<ar.wantInt("code")<<", ex: "<<ar.wantString("ex")<<endl;
		return false;
	}

	DeployTaskStatusPtr deployTaskStatus = _processor->deployTaskStatus();

	int sleepTicket = 0;
	while (true)
	{
		sleep(1);
		sleepTicket++;

		if (deployTaskStatus->completed)
		{
			if (deployTaskStatus->failedEndpoints.empty())
				return true;

			cout<<"Deploy Actor "<<actorName()<<" failed."<<endl;
			cout<<"Failed endpoint(s):";
			for (auto& ep: deployTaskStatus->failedEndpoints)
				cout<<" "<<ep;

			return false;
		}

		if ((sleepTicket % 5) == 0)
			cout<<"Deploy "<<actorName()<<", please wait ..."<<endl;

		if (sleepTicket == 10)
		{
			sleepTicket = 0;
			_client->sendQuest(FPQWriter::emptyQuest("ping"), [](FPAnswerPtr answer, int errorCode){});
		}
	}
}

int StressController::launchActor(const std::string& deployerEndpoint, const std::string& actorInstanceName, const std::string& launchParams, struct MachineActorStatus& status)
{
	std::string deployerHost;
	int port;

	if (!parseAddress(deployerEndpoint, deployerHost, port))
	{
		cout<<"[Launch Actor][Exception] Invalid deployer endpoint "<<deployerEndpoint<<endl;
		return 0;
	}

	std::string cmdLine(_client->endpoint());
	cmdLine.append(" ").append(launchParams);

	FPQWriter qw(3, "launchActor");
	qw.param("endpoints", std::vector<std::string>{deployerEndpoint});
	qw.param("actor", actorName());
	qw.param("cmdLine", cmdLine);

	FPAnswerPtr answer = _client->sendQuest(qw.take());
	if (answer->status())
	{
		FPAReader ar(answer);
		cout<<"[Launch Actor][Exception] Error code: "<<ar.wantInt("code")<<", ex: "<<ar.wantString("ex")<<endl;
		return 0;
	}

	int sleepTicket = 0;
	while (true)
	{
		sleep(1);
		sleepTicket++;

		FPAnswerPtr answer = _client->sendQuest(FPQWriter::emptyQuest("actorTaskStatus"));
		FPAReader ar(answer);
		if (ar.status())
			continue;
		else
		{
			std::vector<std::string> fields = ar.want("fields", std::vector<std::string>());
			std::vector<std::vector<std::string>> rows = ar.want("rows", std::vector<std::vector<std::string>>());
			
			int nameIdx = findIndex("actorName", fields);
			int epIdx = findIndex("endpoint", fields);
			int pidIdx = findIndex("pid", fields);

			for (auto& row: rows)
			{
				if (row[nameIdx] == actorInstanceName)
				{
					int port;
					std::string actorHost;

					if (!parseAddress(row[epIdx], actorHost, port))
						continue;

					if (actorHost == deployerHost)
					{
						int pid = atoi(row[pidIdx].c_str());
						if (status.pidEpMap.find(pid) == status.pidEpMap.end())
						{
							status.pidEpMap[pid] = row[epIdx];
							return pid;
						}
					}
				}
			}
		}

		if ((sleepTicket % 5) == 0)
			cout<<"Launch actor, please wait ..."<<endl;

		if (sleepTicket == 20)
		{
			cout<<"Launch actor timeouted."<<endl;
			return 0;
		}
	}
}

bool StressController::sendAction(const std::string& actorInstanceName, const std::string& actorInstanceEndpoint, int pid, const std::string& method, FPWriter& payloadWriter, const std::string& desc)
{
	FPQWriter qw(6, "actorAction");
	qw.param("actor", actorInstanceName);
	qw.param("endpoint", actorInstanceEndpoint);
	qw.param("pid", pid);
	qw.param("method", method);
	qw.param("payload", payloadWriter.raw());
	qw.param("taskDesc", desc);

	FPAnswerPtr answer = _client->sendQuest(qw.take());
	FPAReader ar(answer);
	
	return (answer->status() == 0);
}

bool StressController::quitActor(const std::string& endpoint, const std::string& actorInstanceName, int pid)
{
	FPWriter pw((uint32_t)0);

	FPQWriter qw(6, "actorAction");
	qw.param("actor", actorInstanceName);
	qw.param("endpoint", endpoint);
	qw.param("pid", pid);
	qw.param("method", "quit");
	qw.param("payload", pw.raw());
	qw.param("taskDesc", std::string("Quit actor ").append(endpoint).append(" pid ").append(std::to_string(pid)));

	FPAnswerPtr answer = _client->sendQuest(qw.take());
	return (answer->status() == 0);
}