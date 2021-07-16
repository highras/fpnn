#ifndef Test_Utils_H
#define Test_Utils_H

#include <sys/time.h>
#include <iostream>
#include <atomic>
#include <string>
#include <map>
#include <unordered_map>

struct timeval diff_timeval(struct timeval start, struct timeval finish);

namespace fpnn {
class SegmentTimeAnalyst
{
private:
	struct SequentCost
	{
		long long count;
		std::string desc;
		struct timeval start;
		struct timeval total;
	};

	struct TimeCost
	{
		bool mark;
		std::string desc;
		struct timeval start;
		struct timeval end;
	};

	int _id;
	int _index;
	std::string _desc;
	std::unordered_map<std::string, int> _indexMap;
	std::map<int, struct TimeCost> _results;
	std::map<int, struct SequentCost> _sequentResults;
	
	void showAnalysis();
	void endSegment(int index);
	void endFragment(int index);
	int addSegment(const std::string& desc, bool isMark);
	int addFragment(const std::string& desc);

	friend struct SegmentTimeMonitor;
	friend struct FragmentTimeMonitor;

public:
	SegmentTimeAnalyst(const std::string& desc): _index(0), _desc(desc)
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
		_indexMap[desc] = addSegment(desc, false);
	}
	
	inline void end(const std::string& desc)
	{
		auto it = _indexMap.find(desc);
		if (it != _indexMap.end())
			endSegment(it->second);
	}

	inline void mark(const std::string& desc)
	{
		addSegment(desc, true);
	}

	inline void fragmentStart(const std::string& desc)
	{
		addFragment(desc);
	}

	inline void fragmentEnd(const std::string& desc)
	{
		auto it = _indexMap.find(desc);
		if (it != _indexMap.end())
			endFragment(it->second);
	}
};

struct SegmentTimeMonitor
{
private:
	int _index;
	bool _done;
	SegmentTimeAnalyst* _analyst;

public:
	SegmentTimeMonitor(SegmentTimeAnalyst* analyst, const std::string& desc): _done(false), _analyst(analyst)
	{
		_index = _analyst->addSegment(desc, false);
	}

	~SegmentTimeMonitor()
	{
		if (!_done)
			end();
	}

	void end()
	{
		_done = true;
		_analyst->endSegment(_index);
	}
};

struct FragmentTimeMonitor
{
private:
	int _index;
	bool _done;
	SegmentTimeAnalyst* _analyst;

public:
	FragmentTimeMonitor(SegmentTimeAnalyst* analyst, const std::string& desc): _done(false), _analyst(analyst)
	{
		_index = _analyst->addFragment(desc);
	}

	~FragmentTimeMonitor()
	{
		if (!_done)
			end();
	}

	void end()
	{
		_done = true;
		_analyst->endFragment(_index);
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
