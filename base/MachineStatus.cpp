#include <string.h>
#include <string>
#include <vector>
#include <fstream>
#include "StringUtil.h"
#include "MachineStatus.h"

using namespace fpnn;

int MachineStatus::getInusedSocketCount(bool IPv4, bool TCP)
{
	const char* procFile;
	const char* key;
	int keyLen;
	if (IPv4)
	{
		procFile = "/proc/net/sockstat";
		key = TCP ? "TCP:" : "UDP:";
		keyLen = 4;
	}
	else
	{
		procFile = "/proc/net/sockstat6";
		key = TCP ? "TCP6:" : "UDP6:";
		keyLen = 5;
	}

	std::ifstream fin(procFile);
	if (fin.is_open()) {
		char line[1024];
		while(fin.getline(line, sizeof(line))){
			if (strncmp(key, line, keyLen))
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
	return 0;
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

#ifdef SUPPORT_CUDA
#include "nvml.h"

bool MachineStatus::getGPUInfo(std::list<struct GPUCardInfo>& infos)
{
	nvmlReturn_t result;
	unsigned int device_count, i;

	result = nvmlDeviceGetCount(&device_count);
	if (NVML_SUCCESS != result)
		return false;

	for (i = 0; i < device_count; i++)
	{
		nvmlDevice_t device;
		//char name[NVML_DEVICE_NAME_BUFFER_SIZE];
		result = nvmlDeviceGetHandleByIndex(i, &device);
		if (NVML_SUCCESS != result)
			continue;

		nvmlUtilization_t utilization;
		result = nvmlDeviceGetUtilizationRates(device, &utilization);
		if (NVML_SUCCESS == result)
		{
			GPUCardInfo info;
			info.index = i;
			info.usage = utilization.gpu;
			info.memory.usage = utilization.memory;

			nvmlMemory_t memory;
			result = nvmlDeviceGetMemoryInfo(device, &memory);
			if (NVML_SUCCESS == result)
			{
				info.memory.used = memory.used;
				info.memory.total = memory.total;
			}
			else
			{
				info.memory.used = 0;
				info.memory.total = 0;
			}

			infos.push_back(info);
		}
	}

	return true;
}

#else

bool MachineStatus::getGPUInfo(std::list<struct GPUCardInfo>& infos)
{
	return false;
}

#endif
