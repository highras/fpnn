#include "TimeAnalyst.h"
#include "FPLog.h"
using namespace fpnn;

struct timeval diff_timeval(struct timeval start, struct timeval finish)
{
	struct timeval diff;
	diff.tv_sec = finish.tv_sec - start.tv_sec;
	
	if (finish.tv_usec >= start.tv_usec)
		diff.tv_usec = finish.tv_usec - start.tv_usec;
	else
	{
		diff.tv_usec = 1000 * 1000 + finish.tv_usec - start.tv_usec;
		diff.tv_sec -= 1;
	}
	
	return diff;
}


void SegmentTimeAnalyst::reformData(std::map<int, struct TimeCost>& result)
{
	HashMap<std::string, struct TimeFragment>::node_type* node = _map.next_node(NULL);
	while (node)
	{
		struct TimeCost tc;
		tc.desc = node->key;
		tc.cost = diff_timeval(node->data.start, node->data.end);
		
		result[node->data.index] = tc;
		
		node = _map.next_node(node);
	}
}

void SegmentTimeAnalyst::showAnalysis()
{
	std::map<int, struct TimeCost> results;
	
	reformData(results);
	
	for (int i = 0; i < _index; i++)
	{
		struct TimeCost& tc = results[i];
		
		LOG_INFO("[Time cost][id %d][%s][%s] cost %ld sec %f ms", _id, _desc.c_str(), tc.desc.c_str(), tc.cost.tv_sec, ((float)tc.cost.tv_usec/1000));
	}
}

TimeCostAlarm::~TimeCostAlarm()
{
	struct timeval end;
	gettimeofday(&end, NULL);
	
	struct timeval diff = diff_timeval(_start, end);
	
	bool printInfo = false;
	if (diff.tv_sec > _threshold.tv_sec)
		printInfo = true;
	else if ((diff.tv_sec == _threshold.tv_sec) && (diff.tv_usec > _threshold.tv_usec))
		printInfo = true;
		
	if (printInfo)
		LOG_WARN("[Time Cost Alarm][%s] cost %ld sec %f ms", _desc.c_str(), diff.tv_sec, ((float)diff.tv_usec/1000));
}
