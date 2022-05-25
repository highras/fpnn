#ifndef FPNN_Pool_Info_H
#define FPNN_Pool_Info_H

#include <string>
#include <sstream>

namespace fpnn {
namespace PoolInfo{
	inline std::string threadPoolInfo(int32_t min, int32_t max, 
			int32_t normalThreadCount, int32_t temporaryThreadCount,
			int32_t busyThreadCount, int32_t taskQueueSize, int32_t maxQueueLength){
		std::stringstream ss;   

		ss<<"{";
		ss<<"\"min\":"<<min<<",\"max\":"<<max<<",";
		ss<<"\"normalCount\":"<<normalThreadCount<<",\"tempCount\":"<<temporaryThreadCount<<",";
		ss<<"\"busyCount\":"<<busyThreadCount<<",\"taskSize\":"<<taskQueueSize<<",";
		ss<<"\"maxTask\":"<<maxQueueLength;
		ss<<"}";
		return ss.str();
	}

	inline std::string threadPoolInfo(size_t arraySize, int32_t min, int32_t max, 
			int32_t normalThreadCount, int32_t temporaryThreadCount,
			int32_t busyThreadCount, int32_t taskQueueSize, int32_t maxQueueLength){
		std::stringstream ss;   

		ss<<"{";
		ss<<"\"arraySize\":"<<arraySize<<",";
		ss<<"\"min\":"<<min<<",\"max\":"<<max<<",";
		ss<<"\"normalCount\":"<<normalThreadCount<<",\"tempCount\":"<<temporaryThreadCount<<",";
		ss<<"\"busyCount\":"<<busyThreadCount<<",\"taskSize\":"<<taskQueueSize<<",";
		ss<<"\"maxTask\":"<<maxQueueLength;
		ss<<"}";
		return ss.str();
	}

	inline std::string memoryPoolInfo() {
		return "";
	}

}

}
#endif
