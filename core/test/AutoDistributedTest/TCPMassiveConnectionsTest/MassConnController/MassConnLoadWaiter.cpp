#include <iostream>
#include "FormattedPrint.h"
#include "TCPClient.h"

using namespace std;
using namespace fpnn;

int showUsage(const char* appName)
{
	cout<<"Usgae:"<<endl;
	cout<<"\t"<<appName<<" endpoint monitor-host load-threshold conn-threshold"<<endl;
	cout<<"\t"<<appName<<" host port monitor-host load-threshold conn-threshold"<<endl;
	return -1;
}

int findIndex(const std::string& field, const std::vector<std::string>& fields)
{
	for (size_t i = 0; i < fields.size(); i++)
		if (fields[i] == field)
			return (int)i;

	return -1;
}

void formatStatus(std::vector<std::string>& fields, std::vector<std::vector<std::string>>& rows)
{
	int cpuIdx = findIndex("cpus", fields);
	int loadIdx = findIndex("load", fields);
	int memIdx = findIndex("memories", fields);
	int freeMemIdx = findIndex("freeMemories", fields);

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
	}
}

bool showMachineStatus(TCPClientPtr client, const std::string& monitorHost, double loadThreshold, int connCount)
{
	FPAnswerPtr answer = client->sendQuest(FPQWriter::emptyQuest("machineStatus"));
	FPAReader ar(answer);
	if (ar.status())
	{
		cout<<"[Exception] Error code: "<<ar.wantInt("code")<<", ex: "<<ar.wantString("ex")<<endl;
		return true;
	}
	else
	{
		std::vector<std::string> fields = ar.want("fields", std::vector<std::string>());
		std::vector<std::vector<std::string>> rows = ar.want("rows", std::vector<std::vector<std::string>>());

		formatStatus(fields, rows);
		printTable(fields, rows);

		int loadIdx = findIndex("load", fields);
		int hostIdx = findIndex("host", fields);
		int srcIdx = findIndex("source", fields);
		int connIdx = findIndex("tcpCount", fields);

		int loadStatus = 0;
		bool monitor = false;

		for (auto& row: rows)
		{
			double load = atof(row[loadIdx].c_str());
			if (load > loadThreshold)
				loadStatus++;

			if (row[srcIdx] == "Monitor" && row[hostIdx] == monitorHost)
				monitor = atoi(row[connIdx].c_str()) < connCount;
		}

		if (monitor && loadStatus == 0)
			return false;
		else
			return true;
	}
}

bool openMonitor(TCPClientPtr client)
{
	FPQWriter qw(1, "monitorMachineStatus");
	qw.param("monitor", true);

	FPAnswerPtr answer = client->sendQuest(qw.take());
	FPAReader ar(answer);
	if (ar.status())
	{
		cout<<"[Exception] Error code: "<<ar.wantInt("code")<<", ex: "<<ar.wantString("ex")<<endl;
		return false;
	}
	return true;
}

int main(int argc, const char* argv[])
{
	if (argc < 5 || argc > 6)
		return showUsage(argv[0]);

	std::string monitorHost;
	double load = 0.02;
	int connCount = 9;

	std::string endpoint = argv[1];
	if (argc == 6)
	{
		endpoint.append(":").append(argv[2]);
		monitorHost = argv[3];
		load = atof(argv[4]);
		connCount = atoi(argv[5]);
	}
	else
	{
		monitorHost = argv[2];
		load = atof(argv[3]);
		connCount = atoi(argv[4]);
	}

	TCPClientPtr client = TCPClient::createClient(endpoint);
	if (!client)
		return showUsage(argv[0]);

	if (!openMonitor(client))
		return 0;

	while (showMachineStatus(client, monitorHost, load, connCount))
	{
		cout<<endl;
		sleep(2);
	}

	return 0;
}