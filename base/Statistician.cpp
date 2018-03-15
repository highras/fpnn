#include <time.h>
#include "FPLog.h"
#include "Statistician.h"

using namespace fpnn;

std::shared_ptr<Statistician> Statistician::_statistician;

void reformData(const std::string& name, struct StatisticUnit &su, struct StatisticKindResult &result)
{
	const size_t statisticTimeRange = 60;
	
	unsigned long long curr = su._perMinCount.exchange(0);
	su._countPerMin.push_front(curr);
	if (su._countPerMin.size() > statisticTimeRange)
		su._countPerMin.pop_back();
		
	unsigned long long total = su._total;
	result.total += total;

	if(result.total == 0){
		return;
	}
	
	int num = 0;
	for (std::list<unsigned long long>::iterator iter = su._countPerMin.begin();
		iter != su._countPerMin.end(); iter++, num++)
	{
		if (num < 1)
			result.currMin += *iter;
			
		if (num < 5)
			result.curr5Min += *iter;
			
		if (num < 10)
			result.curr10Min += *iter;
			
		if (num < 30)
			result.curr30Min += *iter;
			
		result.curr60Min += *iter;
	}
	
	result.ossQPM<<",\""<<name<<"\": "<<curr;
	result.ossQPS<<",\""<<name<<"\": "<<(curr/60);
	result.ossTotal<<",\""<<name<<"\": "<<total;
}

void CategoryStatisticor::reformResult(std::map<int, std::map<std::string, struct StatisticKindResult>> &results)
{
	std::set<int> projectIds;
	{
		RKeeper r(&_locker);
		projectIds = _projectIds;
	}

	for (int projectId: projectIds)
	{
		std::map<std::string, struct StatisticKindResult> &relustMap = results[projectId];

		CateStatUnitPtr csup = findStatNode(projectId);

		for (CateStatUnit::node_type* node = csup->next_node(NULL); node != NULL; node = csup->next_node(node))
		{
			struct StatisticKindResult &result = relustMap[node->key.kind];
			reformData(node->key.name, node->data, result);
		}
	}
}

void CategoryStatisticor::logStatus()
{
	std::map<int, std::map<std::string, struct StatisticKindResult>> projectResults;
	reformResult(projectResults);

	for (auto& projectResultPair: projectResults)
	{
		int projectId = projectResultPair.first;
		std::map<std::string, struct StatisticKindResult> &resultMap = projectResultPair.second;

		for (auto& kindResultPair: resultMap)
		{
			const std::string &kind = kindResultPair.first;
			struct StatisticKindResult& result = kindResultPair.second;

			if(result.total == 0)
				continue;

			std::ostringstream ossQps;
			std::ostringstream ossQpm;
			std::ostringstream ossTotal;

			std::string qpsInfo = result.ossQPS.str();
			std::string qpmInfo = result.ossQPM.str();
			std::string totalInfo = result.ossTotal.str();

			qpsInfo.front() = ' ';
			qpmInfo.front() = ' ';
			totalInfo.front() = ' ';

			ossQps<<"{\"kind\":\""<<kind<<"\", \"projectId\":"<<projectId<<", \"QPS\":{"<<qpsInfo<<"}}";
			ossQpm<<"{\"kind\":\""<<kind<<"\", \"projectId\":"<<projectId<<", \"calledInLastMinute\":{"<<qpmInfo<<"}}";
			ossTotal<<"{\"kind\":\""<<kind<<"\", \"projectId\":"<<projectId<<", \"totalCalled\":"<<result.total<<", \"interfaces\":{"<<totalInfo<<"}}";

			LOG_STAT("STAT.PROJECT.Interfaces.QPS", "%s", ossQps.str().c_str());
			LOG_STAT("STAT.PROJECT.Interfaces.lastMinute", "%s", ossQpm.str().c_str());
			LOG_STAT("STAT.PROJECT.Interfaces.Total", "%s", ossTotal.str().c_str());
		}
	}
}

void Statistician::reformResult()
{
	for (HashMap<struct StatisticKey, struct StatisticUnit>::node_type* node = _dataMap.next_node(NULL); node != NULL; node = _dataMap.next_node(node))
	{
		HashMap<std::string, struct StatisticKindResult>::node_type* resultNode = _resultMap.find(node->key.kind);
		if(!resultNode) continue;
		
		reformData(node->key.name, node->data, resultNode->data);
	}
}

void Statistician::reporter_thread()
{
	static int32_t count = 0;
	while (_running)
	{
		++count;
		usleep(200*1000);
		if(count % 300 != 0)
			continue;
		count = 0;
		
		reformResult();
		
		for (HashMap<std::string, struct StatisticKindResult>::node_type* node = _resultMap.next_node(NULL); node != NULL; node = _resultMap.next_node(node))
		{
			std::string& kind = node->key;
			struct StatisticKindResult& result = node->data;

			if(result.total == 0)
				continue;
			
			std::ostringstream ossQps;
			std::ostringstream ossQPM;
			std::ostringstream ossTotal;

			std::string qpmInfo = result.ossQPM.str();
			std::string totalInfo = result.ossTotal.str();
			qpmInfo.front() = ' ';
			totalInfo.front() = ' ';
			ossQPM<<"{\"kind\":\""<<kind<<"\", \"calledInLastMinute\":{"<<qpmInfo<<"}}";
			ossTotal<<"{\"kind\":\""<<kind<<"\", \"interfaces\":{"<<totalInfo<<"}}";
			
			ossQps<<"["<<kind<<" total called "<<result.total<<"]";
			ossQps<<"["<<kind<<" minutes called]: last 1 min: "<<result.currMin;
			ossQps<<", 5 mins: "<<result.curr5Min;
			ossQps<<", 10 mins: "<<result.curr10Min;
			ossQps<<", 30 mins: "<<result.curr30Min;
			ossQps<<", 60 mins: "<<result.curr60Min;
			ossQps<<". [QPS]: last 1 min: "<<(result.currMin/60);
			ossQps<<", 5 mins: "<<(result.curr5Min/(60*5));
			ossQps<<", 10 mins: "<<(result.curr10Min/(60*10));
			ossQps<<", 30 mins: "<<(result.curr30Min/(60*30));
			ossQps<<", 60 mins: "<<(result.curr60Min/(60*60));
			
			result.reset();
			
			LOG_STAT("STAT.GLOBAL.QPS", "%s", ossQps.str().c_str());
			LOG_STAT("STAT.GLOBAL.Interfaces.callInLastMinute", "%s", ossQPM.str().c_str());
			LOG_STAT("STAT.GLOBAL.Interfaces.Total", "%s", ossTotal.str().c_str());
		}

		_cateStator.logStatus();
	}
}

std::string Statistician::genJSONDescription(const std::string& kind)
{
	std::ostringstream oss;
	
	bool comma = false;
	
	oss<<"{";
	for (HashMap<struct StatisticKey, struct StatisticUnit>::node_type* node = _dataMap.next_node(NULL); node != NULL; node = _dataMap.next_node(node))
	{
		if (node->key.kind == kind)
		{
			if (comma)
				oss<<",";
			else
				comma = true;
				
			oss<<"\""<<node->key.name<<"\":"<<node->data._total;
		}
	}
	oss<<"}";
	return oss.str();
}
