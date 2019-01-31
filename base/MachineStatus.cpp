#include <string.h>
#include <string>
#include <vector>
#include <fstream>
#include "StringUtil.h"
#include "MachineStatus.h"

using namespace fpnn;

int MachineStatus::getIPv4ConnectionCount()
{
	std::ifstream fin("/proc/net/sockstat");
	if (fin.is_open()) {
		char line[1024];
		while(fin.getline(line, sizeof(line))){
			if (strncmp("TCP:", line, 4))
				continue;

			std::string sLine(line);
			std::vector<std::string> items;
			StringUtil::split(sLine, " ", items);
			if(items.size() > 3)
			{
				fin.close();
				return std::stoi(items[2]);			// number of inused TCP connections
			}
		}
		fin.close();
	}
	return -1;
}

int MachineStatus::getIPv6ConnectionCount()
{
	std::ifstream fin("/proc/net/sockstat6");
	if (fin.is_open()) {
		char line[1024];
		while(fin.getline(line, sizeof(line))){
			if (strncmp("TCP6:", line, 5))
				continue;

			std::string sLine(line);
			std::vector<std::string> items;
			StringUtil::split(sLine, " ", items);
			if(items.size() > 3)
			{
				fin.close();
				return std::stoi(items[2]);			// number of inused TCP connections
			}
		}
		fin.close();
	}
	return -1;
}

int MachineStatus::getConnectionCount()
{
	int ipv4 = getIPv4ConnectionCount();
	int ipv6 = getIPv6ConnectionCount();

	if (ipv4 < 0) ipv4 = 0;
	if (ipv6 < 0) ipv6 = 0;

	return ipv4 + ipv6;
}

float MachineStatus::getCPULoad()
{
	std::ifstream fin("/proc/loadavg");
	if (fin.is_open()) {
		char line[1024];
		fin.getline(line, sizeof(line));
		std::string sLine(line);
		std::vector<std::string> items;
		StringUtil::split(sLine, " ", items);
		fin.close();
		try {
			float res = std::stof(items[0]);		// load average within 1 miniute
			return res;
		} catch(std::exception& e) {
			return -1;
		}
	}
	return -1;			// cannot open the file
}

void MachineStatus::getNetworkStatus(uint64_t& recvBytes, uint64_t& sendBytes)
{
	recvBytes = 0;
	sendBytes = 0;

	std::ifstream fin("/proc/net/dev");
	if (fin.is_open())
	{
		char line[1024];
		while(fin.getline(line, sizeof(line)))
		{
			std::string sLine(line);
			std::vector<std::string> items;
			StringUtil::split(sLine, " ", items);

			if (items.empty())
				continue;

			if (strncmp("eth", items[0].c_str(), 3) == 0
				|| strncmp("ens", items[0].c_str(), 3) == 0
				|| strncmp("eno", items[0].c_str(), 3) == 0
				|| strncmp("enp", items[0].c_str(), 3) == 0)
			{
				recvBytes += std::stoull(items[1]);
				sendBytes += std::stoull(items[9]);
			}
		}
		fin.close();
	}
}