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

void SegmentTimeAnalyst::showAnalysis()
{
	for (int i = 0; i < _index; i++)
	{
		auto it = _results.find(i);
		if (it != _results.end())
		{
			struct TimeCost& tc = it->second;
			if (!tc.mark)
			{
				struct timeval cost = diff_timeval(tc.start, tc.end);

				UXLOG("Time cost", "[id %d][%s][%d][%s] range: [%lld - %lld], cost %ld sec %f ms", _id, _desc.c_str(), i, tc.desc.c_str(),
					tc.start.tv_sec * 1000 + tc.start.tv_usec/1000,
					tc.end.tv_sec * 1000 + tc.end.tv_usec/1000,
					cost.tv_sec, ((float)cost.tv_usec/1000));
			}
			else
			{
				UXLOG("Time cost", "[id %d][%s][%d][%s] occurred ms: %lld", _id, _desc.c_str(), i, tc.desc.c_str(), tc.start.tv_sec * 1000 + tc.start.tv_usec/1000);
			}
		}
		else
		{
			struct SequentCost& sc = _sequentResults[i];
			if (sc.count == 0)
			{
				LOG_ERROR("Invalid Fragement [id %d][%s][%d][%s], count is 0.", _id, _desc.c_str(), i, sc.desc.c_str());
				continue;
			}
			else if (sc.start.tv_sec != 0)
			{
				LOG_ERROR("Invalid Fragement [id %d][%s][%d][%s], count is %lld, but not completed.", _id, _desc.c_str(), i, sc.desc.c_str(), sc.count);
				continue;
			}

			long long usec = sc.total.tv_sec * 1000 * 1000 + sc.total.tv_usec;
			usec /= sc.count;

			long long sec = usec / (1000 * 10000);
			UXLOG("Time cost", "[id %d][%s][%d][%s] count %lld, total cost %ld sec %f ms, avg %lld sec %f ms", _id, _desc.c_str(), i, sc.desc.c_str(),
					sc.count,
					sc.total.tv_sec, ((float)sc.total.tv_usec/1000),
					sec, ((float)(usec - sec * 1000 * 1000)/1000));
		}
	}
}

void SegmentTimeAnalyst::endSegment(int index)
{
	gettimeofday(&(_results[index].end), NULL);
}

void SegmentTimeAnalyst::endFragment(int index)
{
	struct timeval end;
	gettimeofday(&end, NULL);

	struct SequentCost& sc = _sequentResults[index];
	struct timeval cost = diff_timeval(sc.start, end);

	sc.total.tv_sec += cost.tv_sec;
	sc.total.tv_usec += cost.tv_usec;
	if (sc.total.tv_usec >= 1000 * 1000)
	{
		sc.total.tv_usec -= 1000 * 1000;
		sc.total.tv_sec += 1;
	}

	sc.start = {0, 0};
	sc.count += 1;
}

int SegmentTimeAnalyst::addSegment(const std::string& desc, bool isMark)
{
	struct TimeCost tc;
	tc.mark = isMark;
	tc.desc = desc;
	gettimeofday(&tc.start, NULL);

	int index = _index++;
	_results[index] = tc;

	return index;
}

int SegmentTimeAnalyst::addFragment(const std::string& desc)
{
	auto it = _indexMap.find(desc);
	if (it != _indexMap.end())
	{
		int index = it->second;
		struct SequentCost& sc = _sequentResults[index];
		gettimeofday(&sc.start, NULL);
		return index;
	}

	struct SequentCost sc;
	sc.count = 0;
	sc.total = {0, 0};
	sc.desc = desc;
	gettimeofday(&sc.start, NULL);

	int index = _index++;
	_sequentResults[index] = sc;
	_indexMap[desc] = index;

	return index;
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
