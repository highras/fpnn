#ifndef Machine_Status_H
#define Machine_Status_H

#include <stdint.h>
#include <list>

namespace fpnn
{
	namespace MachineStatus
	{
		struct GPUCardInfo
		{
			unsigned int index;
			unsigned int usage;

			struct {
				unsigned int usage;
				unsigned long long used;
				unsigned long long total;
			} memory;
		};

		float getCPULoad();
		int getInusedSocketCount(bool IPv4, bool TCP);
		void getNetworkStatus(uint64_t& recvBytes, uint64_t& sendBytes);

		/**
		 * 		If 'CUDA_SUPPORT=true' is disable, or 'CUDA_SUPPORT' is not 'true' in def.mk,
		 *		the function getGPUInfo(std::list<struct GPUCardInfo>& infos) will return false.
		 * 
		 * 		If 'CUDA_SUPPORT' is enable, please call 'nvmlInit()' before the function
		 * 		getGPUInfo(...) first called; and please call 'nvmlShutdown()' after the function
		 * 		getGPUInfo(...) last called.
		 * 
		 * */
		bool getGPUInfo(std::list<struct GPUCardInfo>& infos);
	}
}

#endif