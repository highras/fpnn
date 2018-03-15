#ifndef Test_Utils_H
#define Test_Utils_H

#include <sys/time.h>
#include <iostream>
#include <atomic>
#include <string>
#include <map>
#include "HashMap.h"
#include "jenkins.h"

struct timeval diff_timeval(struct timeval start, struct timeval finish);

namespace fpnn {
class SegmentTimeAnalyst
{
private:
	struct TimeFragment
	{
		int index;
		struct timeval start;
		struct timeval end;
		
		TimeFragment(int index_): index(index_), start{0, 0}, end{0, 0} {}
		TimeFragment(const TimeFragment& r): index(r.index)
		{
			start = r.start;
			end = r.end;
		}
	};
	
	struct TimeCost
	{
		std::string desc;
		struct timeval cost;
	};

	int _id;
	int _index;
	std::string _desc;
	HashMap<std::string, struct TimeFragment>   _map;
	
	void reformData(std::map<int, struct TimeCost>& result);
	void showAnalysis();
	
public:
	SegmentTimeAnalyst(const std::string& desc): _index(0), _desc(desc), _map(16)
	{
		static std::atomic<int> globalIDGenerator(0);
		_id = globalIDGenerator.fetch_add(1);
		
		start("__FLOW_COST__");
	}
	
	~SegmentTimeAnalyst()
	{
		end("__FLOW_COST__");
		showAnalysis();
	}
	
	inline void start(const std::string& desc)
	{
		struct TimeFragment tf(_index);
		_map.insert(desc, tf);
		
		_index += 1;
		
		HashMap<std::string, struct TimeFragment>::node_type* node = _map.find(desc);
		gettimeofday(&(node->data.start), NULL);
	}
	
	inline void end(const std::string& desc)
	{
		HashMap<std::string, struct TimeFragment>::node_type* node = _map.find(desc);
		gettimeofday(&(node->data.end), NULL);
	}
};

class TimeCostAlarm
{
private:
	struct timeval _threshold;
	struct timeval _start;
	std::string _desc;
	
public:
	TimeCostAlarm(const std::string& desc, time_t second_threshold, suseconds_t microsecond_threshold): _threshold{second_threshold, microsecond_threshold}, _desc(desc)
	{
		gettimeofday(&_start, NULL);
	}
	
	TimeCostAlarm(const std::string& desc, int32_t millisecond_threshold): _threshold{0, millisecond_threshold * 1000}, _desc(desc)
	{
		if (millisecond_threshold >= 1000)
		{
			_threshold.tv_sec = millisecond_threshold / 1000;
			_threshold.tv_usec = (millisecond_threshold % 1000) * 1000;
		}
		
		gettimeofday(&_start, NULL);
	}
	
	~TimeCostAlarm();
};
}
#endif
