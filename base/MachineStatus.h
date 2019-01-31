#ifndef Machine_Status_H
#define Machine_Status_H

#include <stdint.h>

namespace fpnn
{
	namespace MachineStatus
	{
		float getCPULoad();
		int getConnectionCount();
		int getIPv4ConnectionCount();
		int getIPv6ConnectionCount();
		void getNetworkStatus(uint64_t& recvBytes, uint64_t& sendBytes);
	}
}

#endif