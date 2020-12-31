#ifndef Base_Actor_h
#define Base_Actor_h

#include "StressSource.h"

using namespace fpnn;

class TaskFinisher
{
	int _taskId;
public:
	TaskFinisher(int taskId): _taskId(taskId) {}
	TaskFinisher(int taskId, const std::string& method, const std::string& desc): _taskId(taskId)
	{
		ControlCenter::beginTask(taskId, method, desc);
	}
	~TaskFinisher() { ControlCenter::finishTask(_taskId); }
};
typedef std::shared_ptr<TaskFinisher> TaskFinisherPtr;

struct StressTask
{
	TaskFinisherPtr taskFinisher;
	StressSourcePtr stressSource;

	~StressTask()
	{
		stressSource.reset();
		taskFinisher.reset();
	}
};

/*
	Just rewrite the following class.
	The default three methods are interfaces of executive actor, which cannot be removed.
*/
class ExecutiveActor
{
	std::mutex _mutex;
	std::string _region;
	std::atomic<bool> _running;
	std::map<int, StressTask> _taskMap;
	std::string _actorInstanceName;

public:
	bool globalInit();
	bool actorStopped();
	std::string actorName();
	void setRegion(const std::string& region) { _region = region; }
	void action(int taskId, const std::string& method, const FPReaderPtr payload);
	static std::string customParamsUsage();

public:
	ExecutiveActor(): _running(true) {}
	void addAction(int taskId, struct StressTask& stressTask);
};


#endif