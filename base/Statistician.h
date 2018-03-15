#ifndef Statistician_H
#define Statistician_H

#include <atomic>
#include <thread>
#include <list>
#include <memory>
#include <sstream>
#include <string>
#include <map>
#include <set>
#include "RWLocker.hpp"
#include "HashMap.h"
#include "jenkins.h"

namespace fpnn {

struct StatisticKey
{
	std::string kind;
	std::string name;
	
	StatisticKey(const std::string& kind_, const std::string& name_): kind(kind_), name(name_) {}
	
	bool operator==(const struct StatisticKey& r) const
	{
		return kind == r.kind && name == r.name;
	}

	bool operator<(const struct StatisticKey& r) const
	{
		if (kind != r.kind) 
			return kind < r.kind;

		return name < r.name;
	}

	unsigned int hash() const
	{
		uint32_t initval = jenkins_hash(kind.data(), kind.length(), 0);
		return jenkins_hash(name.data(), name.length(), initval);
	}
};

struct StatisticUnit
{
	std::atomic<unsigned long long> _total;
	std::atomic<unsigned long long> _perMinCount;
	std::list<unsigned long long> _countPerMin;		//-- only operated by reporter thread!!!
	
	StatisticUnit(): _total(0), _perMinCount(0) {}
	StatisticUnit(const StatisticUnit& r): _total(r._total.load()), _perMinCount(r._perMinCount.load()), _countPerMin(r._countPerMin) {}
};

struct StatisticKindResult
{
	unsigned long long total;
	unsigned long long currMin;
	unsigned long long curr5Min;
	unsigned long long curr10Min;
	unsigned long long curr30Min;
	unsigned long long curr60Min;
	
	std::ostringstream ossQPM;
	std::ostringstream ossQPS;
	std::ostringstream ossTotal;
	
	StatisticKindResult(): total(0), currMin(0), curr5Min(0), curr10Min(0), curr30Min(0), curr60Min(0) {}
	/* Do nothing, just construct an empty instance. */
	StatisticKindResult(const StatisticKindResult& r): total(0), currMin(0), curr5Min(0), curr10Min(0), curr30Min(0), curr60Min(0) {}
	
	void reset()
	{
		total = 0;
		currMin = 0;
		curr5Min = 0;
		curr10Min = 0;
		curr30Min = 0;
		curr60Min = 0;
		
		ossQPM.str("");
		ossQPS.str("");
		ossTotal.str("");
	}
};

class CategoryStatisticor
{
private:
	typedef HashMap<struct StatisticKey, struct StatisticUnit> CateStatUnit;
	typedef std::shared_ptr<CateStatUnit>   CateStatUnitPtr;

	std::map<std::string, std::list<std::string>> _statisticItems;

	HashMap<int, CateStatUnitPtr> _cateStatMap;
	std::set<int> _projectIds;
	RWLocker _locker;

	CateStatUnitPtr findStatNode(int project_id)
	{
		{
			RKeeper r(&_locker);
			HashMap<int, CateStatUnitPtr>::node_type* node = _cateStatMap.find(project_id);
			if (node)
				return node->data;
		}

		CateStatUnitPtr csup(new CateStatUnit(_statisticItems.size()));
		initCateStatUnit(csup);

		{
			WKeeper w(&_locker);
			HashMap<int, CateStatUnitPtr>::node_type* node = _cateStatMap.find(project_id);
			if (node)
				return node->data;

			_cateStatMap.insert(project_id, csup);
			_projectIds.insert(project_id);
			return csup;
		}
	}

	void initCateStatUnit(CateStatUnitPtr csup)
	{
		struct StatisticKey key("", "");
		struct StatisticUnit value;

		for (auto& siPair: _statisticItems)
		{
			key.kind = siPair.first;
			for (const std::string& name: siPair.second)
			{
				key.name = name;
				csup->insert(key, value);
			}
		}
	}

	void reformResult(std::map<int, std::map<std::string, struct StatisticKindResult>> &results);

public:
	CategoryStatisticor(): _cateStatMap(1024) {}

	inline void statistic(int project_id, const std::string& kind, const std::string& name, int count = 1)
	{
		CateStatUnitPtr csup = findStatNode(project_id);

		struct StatisticKey key(kind, name);
		
		HashMap<struct StatisticKey, struct StatisticUnit>::node_type* node = csup->find(key);
		if(!node) return;

		StatisticUnit& su = node->data;
		su._total.fetch_add(count);
		su._perMinCount.fetch_add(count);
	}

	inline void add(const std::string& kind, const std::string& name)
	{
		_statisticItems[kind].push_back(name);
	}
	
	inline void add(const std::string& kind, const std::list<std::string>& names)
	{
		for (const std::string& name: names)
			_statisticItems[kind].push_back(name);
	}

	void logStatus();
};

class Statistician
{
private:
	std::thread _reporter;
	std::atomic<bool> _running;
	
	HashMap<struct StatisticKey, struct StatisticUnit>   _dataMap;
	HashMap<std::string, struct StatisticKindResult>   _resultMap;

	CategoryStatisticor _cateStator;
	
	static std::shared_ptr<Statistician> _statistician;
	
	void reformResult();
	void reporter_thread();
	
private:
	Statistician(size_t statisticItemCount, size_t statisticKindCount = 8): _running(true), _dataMap(statisticItemCount), _resultMap(statisticKindCount) {}
	
public:
	static inline Statistician* getNakedInstance() { return _statistician.get(); }
	inline static std::shared_ptr<Statistician> create(size_t statisticItemCount, size_t statisticKindCount = 8)
	{
		_statistician.reset(new Statistician(statisticItemCount, statisticKindCount));
		return _statistician;
	}
	
	~Statistician()
	{
		_running = false;
		_reporter.join();
	}
	
	void add(const std::string& kind, const std::string& name)
	{
		struct StatisticKey key(kind, name);
		struct StatisticUnit value;
		
		_dataMap.insert(key, value);
		
		if (_resultMap.find(kind) == NULL)
		{
			struct StatisticKindResult result;
			_resultMap.insert(kind, result);
		}

		_cateStator.add(kind, name);
	}
	
	void add(const std::string& kind, const std::list<std::string>& names)
	{
		struct StatisticKey key(kind, "");
		struct StatisticUnit value;
		
		for (const std::string& name: names)
		{
			key.name = name;
			_dataMap.insert(key, value);
		}
		
		if (_resultMap.find(kind) == NULL)
		{
			struct StatisticKindResult result;
			_resultMap.insert(kind, result);
		}

		_cateStator.add(kind, names);
	}
	
	inline void init(bool usingUXLog = false)
	{
		_reporter = std::thread(&Statistician::reporter_thread, this);
	}
	
	inline void statistic(const std::string& kind, const std::string& name, int project_id, int count = 1)
	{
		struct StatisticKey key(kind, name);
		
		HashMap<struct StatisticKey, struct StatisticUnit>::node_type* node = _dataMap.find(key);
		if(!node) return;

		StatisticUnit& su = node->data;
		su._total.fetch_add(count);
		su._perMinCount.fetch_add(count);

		if (project_id)
			_cateStator.statistic(project_id, kind, name, count);
	}
	
	std::string genJSONDescription(const std::string& kind);
};

inline void GlobalStatistic(const std::string& kind, const std::string& name, int project_id, int count = 1)
{
	Statistician::getNakedInstance()->statistic(kind, name, project_id, count);
}

class StatisticCounter
{
private:
	std::string _kind;
	std::string _name;
	int _project_id;
	int _count;
	
public:
	StatisticCounter(const std::string& kind, const std::string& name, int project_id, int count = 1):
		_kind(kind), _name(name), _project_id(project_id), _count(count) {}
	~StatisticCounter()
	{
		Statistician::getNakedInstance()->statistic(_kind, _name, _project_id, _count);
	}
};

}

#endif
