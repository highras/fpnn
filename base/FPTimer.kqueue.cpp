#include <algorithm>
#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>
#include <unistd.h>
#include <time.h>
#include "NetworkUtility.h"
#include "FPTimer.h"

//-- For unused the returned values of write() in Timer::wakeUpEpollWithLocker()/Timer::wakeUpEpollWithoutLocker().
#pragma GCC diagnostic ignored "-Wunused-result"

using namespace fpnn;

static const int Timer_defaultWaitMsec = 2000;

struct PeriodTask: public Timer::TimerTask
{
	int _cycleTimes;
	uint64_t _delayMsec; //-- delay msec & period & interval

	PeriodTask(Timer::ITaskPtr task, uint64_t intervalMsec, int cycleTimes, bool runRightNow):
		TimerTask(task), _cycleTimes(cycleTimes), _delayMsec(intervalMsec)
	{
		_rescheduleWhenTrigger = true;

		if (cycleTimes == 0)
			cycleTimes = -1;

		if (runRightNow)
			_triggeredTimeMsec = slack_real_msec();
		else
			_triggeredTimeMsec = slack_real_msec() + _delayMsec;
	}

	virtual void adjustSchedule()
	{
		_triggeredTimeMsec += _delayMsec;
	}

	virtual bool reschedulable()
	{
		if (_cycleTimes > 0)
		{
			_cycleTimes -= 1;
			if (_cycleTimes == 0)
				return false;
		}
		return true;
	}
};

struct RepeatedTask: public Timer::TimerTask
{
	int _cycleTimes;
	uint64_t _delayMsec; //-- delay msec & period & interval

	RepeatedTask(Timer::ITaskPtr task, uint64_t intervalMsec, int cycleTimes):
		TimerTask(task), _cycleTimes(cycleTimes), _delayMsec(intervalMsec)
	{
		if (cycleTimes == 0)
			cycleTimes = -1;

		adjustSchedule();
	}

	virtual void adjustSchedule()
	{
		_triggeredTimeMsec = slack_real_msec() + _delayMsec;
	}

	virtual bool reschedulable()
	{
		if (_cycleTimes > 0)
		{
			_cycleTimes -= 1;
			if (_cycleTimes == 0)
				return false;
		}
		return true;
	}
};

struct DailyTask: public Timer::TimerTask
{
	int _hour;
	int _minute;
	int _second;
	bool _rescheduled;

	DailyTask(Timer::ITaskPtr task, int hour, int minute, int second):
		TimerTask(task), _hour(hour), _minute(minute), _second(second), _rescheduled(false)
	{
		checkTime();
		adjustSchedule();
	}

	void checkTime();
	virtual void adjustSchedule();
	virtual bool reschedulable()
	{
		_rescheduled = true;
		return true;
	}
};

void DailyTask::checkTime()
{
	while (_second >= 60)
	{
		_second -= 60;
		_minute += 1;
	}
	while (_minute >= 60)
	{
		_minute -= 60;
		_hour += 1;
	}
	while (_hour >= 24)
		_hour -= 24;
}

void DailyTask::adjustSchedule()
{
	const int secondInDay = 24 * 60 * 60;

	time_t now = time(NULL);
	struct tm nowTm;
	gmtime_r(&now, &nowTm);

	nowTm.tm_hour = _hour;
	nowTm.tm_min = _minute;
	nowTm.tm_sec = _second;

	time_t triggeredSec = mktime(&nowTm);
	if (_rescheduled)
	{
		_triggeredTimeMsec += secondInDay * 1000;
	}
	else
	{
		if (triggeredSec < now)
			triggeredSec += secondInDay;

		_triggeredTimeMsec = triggeredSec * 1000;
	}
}

//========================================//
//-         Timer Thread Task            -//
//========================================//
class TimerThreadCancelTask: public ITaskThreadPool::ITask
{
	Timer::TimerTaskPtr _timerTask;

public:
	TimerThreadCancelTask(Timer::TimerTaskPtr timerTask): _timerTask(timerTask) {}
	virtual ~TimerThreadCancelTask() {}
	virtual void run()
	{
		try {
			_timerTask->_task->cancelled();
		}
		catch (...)
		{
			LOG_ERROR("Unknown error when running customer's cancelling task.");
		}
	}
};

//========================================//
//-       Timer Basic Functions          -//
//========================================//
bool Timer::initKqueue()
{
	_kqueue_fd = kqueue();
	if (_kqueue_fd == -1)
	{
		_kqueue_fd = 0;
		return false;
	}

	if (pipe(_eventNotifyFds))
	{
		stopKqueue();
		return false;
	}

	nonblockedFd(_eventNotifyFds[0]);

	struct kevent ev;
	EV_SET(&ev, _eventNotifyFds[0], EVFILT_READ, EV_ADD | EV_CLEAR, 0, 0, NULL);

	if (kevent(_kqueue_fd, &ev, 1, NULL, 0, NULL) == -1)
	{
		stopKqueue();
		return false;
	}

	return true;
}

void Timer::stopKqueue()
{
	if (_eventNotifyFds[0])
	{
		close(_eventNotifyFds[1]);
		close(_eventNotifyFds[0]);
		_eventNotifyFds[0] = 0;
		_eventNotifyFds[1] = 0;
	}
	
	if (_kqueue_fd)
	{
		close(_kqueue_fd);
		_kqueue_fd = 0;
	}
}

bool Timer::init(ITaskThreadPoolPtr threadPool)
{
	std::unique_lock<std::mutex> lck(_mutex);
	_threadPool = threadPool;

	if (!initKqueue())
		return false;

	_running = true;
	_eventThread = std::thread(&Timer::eventThread, this);

	return true;
}

void Timer::eventThread()
{
	struct kevent ev;
	struct timespec timeSpec;

	timeSpec.tv_sec  = Timer_defaultWaitMsec / 1000;
	timeSpec.tv_nsec = (Timer_defaultWaitMsec % 1000) * 1000 * 1000;

	while (_running)
	{
		int readyfdsCount = kevent(_kqueue_fd, NULL, 0, &ev, 1, &timeSpec);
		if (!_running)
			break;

		int waitMilliseconds = checkTasks();

		timeSpec.tv_sec  = waitMilliseconds / 1000;
		timeSpec.tv_nsec = (waitMilliseconds % 1000) * 1000 * 1000;

		if (readyfdsCount == -1)
		{
			if (errno == EINTR || errno == EFAULT)
				continue;

			if (errno == EBADF || errno == EINVAL)
			{
				LOG_ERROR("Invalid epoll fd.");
			}
			else
			{
				LOG_ERROR("Unknown Error when epoll_wait() errno: %d", errno);
			}

			break;
		}
	}
	clean();
}

int Timer::checkTasks()
{
	int maxWaitMsec = 1000 * 60 * 10;	//-- 10 minutes.
	{
		std::unique_lock<std::mutex> lck(_mutex);
		while (_priorityQueue.size())
		{
			TimerTaskPtr task = _priorityQueue.top();
			if (task->_removed)
			{
				_priorityQueue.pop();
				removeTask(task);
				continue;
			}

			uint64_t now = slack_real_msec();
			if (task->_triggeredTimeMsec <= now)
			{
				_priorityQueue.pop();
				executeTask(task);
			}
			else
			{
				uint64_t delta = task->_triggeredTimeMsec - now;
				if (delta < (uint64_t)maxWaitMsec)
					return (int)delta;
				else
					return maxWaitMsec;
			}
		}
	}
	return Timer_defaultWaitMsec;
}

//========================================//
//-          Timer Assist APIs           -//
//========================================//
void Timer::stop()
{
	if(!_running) return;

	_running = false;
	wakeUpEpollWithLocker();

	_eventThread.join();
}

void Timer::clean()
{
	std::unique_lock<std::mutex> lck(_mutex);
	stopKqueue();

	cleanTaskQueue();

	_taskMap.clear();
	_taskIdMap.clear();
	//_recycledIdPool.clear();
	while (_recycledIdPool.size())
		_recycledIdPool.pop();

	_idGen = 1;
}

void Timer::cleanTaskQueue()
{
	while (_priorityQueue.size())
	{
		TimerTaskPtr task = _priorityQueue.top();
		_priorityQueue.pop();

		std::shared_ptr<TimerThreadCancelTask> threadPoolTask(new TimerThreadCancelTask(task));
		_threadPool->wakeUp(threadPoolTask);
	}
}

void Timer::wakeUpEpollWithLocker()
{
	std::unique_lock<std::mutex> lck(_mutex);
	write(_eventNotifyFds[1], this, 4);
}

void Timer::wakeUpEpollWithoutLocker()
{
	write(_eventNotifyFds[1], this, 4);
}

void Timer::executeTask(TimerTaskPtr task)
{
	if (task->_rescheduleWhenTrigger)
		rescheduleWithoutLocker(task);

	std::shared_ptr<TimerThreadTask> threadPoolTask(new TimerThreadTask(shared_from_this(), task));
	_threadPool->wakeUp(threadPoolTask);
}

uint64_t Timer::assignId()
{
	if (_recycledIdPool.size() < 100 * 10000)	//-- build buffer area. Will take 7~8M memory.
		return _idGen++;
	else
	{
		uint64_t v = _recycledIdPool.front();
		_recycledIdPool.pop();
		return v;
	}
}

void Timer::reschedule(TimerTaskPtr task)
{
	if (task->reschedulable() == false)
	{
		std::unique_lock<std::mutex> lck(_mutex);
		removeTask(task);
		return;
	}
	
	task->adjustSchedule();

	std::unique_lock<std::mutex> lck(_mutex);
	_priorityQueue.push(task);
	wakeUpEpollWithoutLocker();
}

void Timer::rescheduleWithoutLocker(TimerTaskPtr task)
{
	if (task->reschedulable() == false)
	{
		removeTask(task);
		return;
	}
	
	task->adjustSchedule();

	_priorityQueue.push(task);
	wakeUpEpollWithoutLocker();
}

//========================================//
//-          Timer Main APIs             -//
//========================================//
uint64_t Timer::addTask(ITaskPtr task, uint64_t delayMsec)
{
	TimerTaskPtr ttp(new RepeatedTask(task, delayMsec, 1));

	std::unique_lock<std::mutex> lck(_mutex);

	uint64_t taskId = assignId();
	_taskMap[ttp] = taskId;
	_taskIdMap[taskId] = ttp;

	_priorityQueue.push(ttp);
	wakeUpEpollWithoutLocker();

	return taskId;
}

uint64_t Timer::addPeriodTask(ITaskPtr task, uint64_t periodInMsec, int cycleTimes, bool runRightNow)
{
	TimerTaskPtr ttp(new PeriodTask(task, periodInMsec, cycleTimes, runRightNow));

	std::unique_lock<std::mutex> lck(_mutex);

	uint64_t taskId = assignId();
	_taskMap[ttp] = taskId;
	_taskIdMap[taskId] = ttp;

	if (runRightNow == false)
	{
		_priorityQueue.push(ttp);
		wakeUpEpollWithoutLocker();
	}
	else
		executeTask(ttp);

	return taskId;
}

uint64_t Timer::addRepeatedTask(ITaskPtr task, uint64_t intervalInMsec, int cycleTimes, bool runRightNow)
{
	TimerTaskPtr ttp(new RepeatedTask(task, intervalInMsec, cycleTimes));

	std::unique_lock<std::mutex> lck(_mutex);

	uint64_t taskId = assignId();
	_taskMap[ttp] = taskId;
	_taskIdMap[taskId] = ttp;

	if (runRightNow == false)
	{
		_priorityQueue.push(ttp);
		wakeUpEpollWithoutLocker();
	}
	else
		executeTask(ttp);

	return taskId;
}

uint64_t Timer::addDailyTask(ITaskPtr task, int hour, int minute, int second)
{
	TimerTaskPtr ttp(new DailyTask(task, hour, minute, second));

	std::unique_lock<std::mutex> lck(_mutex);

	uint64_t taskId = assignId();
	_taskMap[ttp] = taskId;
	_taskIdMap[taskId] = ttp;

	_priorityQueue.push(ttp);
	wakeUpEpollWithoutLocker();

	return taskId;
}

void Timer::removeTask(uint64_t taskId)
{
	std::unique_lock<std::mutex> lck(_mutex);
	auto it = _taskIdMap.find(taskId);
	if (it == _taskIdMap.end())
		return;

	TimerTaskPtr task = it->second;
	task->_removed = true;

	_taskIdMap.erase(taskId);
	_taskMap.erase(task);

	_recycledIdPool.push(taskId);
}

void Timer::removeTask(TimerTaskPtr task)	//-- locking before calling this func.
{
	auto it = _taskMap.find(task);
	if (it == _taskMap.end())
		return;

	uint64_t taskId = it->second;
	task->_removed = true;

	_taskIdMap.erase(taskId);
	_taskMap.erase(task);

	_recycledIdPool.push(taskId);
}

//========================================//
//-        Functional Versions           -//
//========================================//
//--------------------[ Functional Type I ]--------------------//
class FunctionTaskI: public Timer::ITask
{
private:
	std::function<void ()> _function;

public:
	explicit FunctionTaskI(std::function<void ()> function): _function(function) {}
	virtual ~FunctionTaskI() {}

	virtual void run()
	{
		_function();
	}
};

uint64_t Timer::addTask(std::function<void ()> task, uint64_t delayMsec)
{
	ITaskPtr t(new FunctionTaskI(std::move(task)));
	return addTask(t, delayMsec);
}

uint64_t Timer::addPeriodTask(std::function<void ()> task, uint64_t periodInMsec, int cycleTimes, bool runRightNow)
{
	ITaskPtr t(new FunctionTaskI(std::move(task)));
	return addPeriodTask(t, periodInMsec, cycleTimes, runRightNow);
}

uint64_t Timer::addRepeatedTask(std::function<void ()> task, uint64_t intervalInMsec, int cycleTimes, bool runRightNow)
{
	ITaskPtr t(new FunctionTaskI(std::move(task)));
	return addRepeatedTask(t, intervalInMsec, cycleTimes, runRightNow);
}

uint64_t Timer::addDailyTask(std::function<void ()> task, int hour, int minute, int second)
{
	ITaskPtr t(new FunctionTaskI(std::move(task)));
	return addDailyTask(t, hour, minute, second);
}

//--------------------[ Functional Type II ]--------------------//
class FunctionTaskII: public Timer::ITask
{
private:
	std::function<void (bool)> _function;

public:
	explicit FunctionTaskII(std::function<void (bool)> function): _function(function) {}
	virtual ~FunctionTaskII() {}

	virtual void run()
	{
		_function(true);
	}
	virtual void cancelled()
	{
		_function(false);
	}
};

uint64_t Timer::addTask(std::function<void (bool)> task, uint64_t delayMsec)
{
	ITaskPtr t(new FunctionTaskII(std::move(task)));
	return addTask(t, delayMsec);
}

uint64_t Timer::addPeriodTask(std::function<void (bool)> task, uint64_t periodInMsec, int cycleTimes, bool runRightNow)
{
	ITaskPtr t(new FunctionTaskII(std::move(task)));
	return addPeriodTask(t, periodInMsec, cycleTimes, runRightNow);
}

uint64_t Timer::addRepeatedTask(std::function<void (bool)> task, uint64_t intervalInMsec, int cycleTimes, bool runRightNow)
{
	ITaskPtr t(new FunctionTaskII(std::move(task)));
	return addRepeatedTask(t, intervalInMsec, cycleTimes, runRightNow);
}

uint64_t Timer::addDailyTask(std::function<void (bool)> task, int hour, int minute, int second)
{
	ITaskPtr t(new FunctionTaskII(std::move(task)));
	return addDailyTask(t, hour, minute, second);
}

//--------------------[ Functional Type III ]--------------------//
class FunctionTaskIII: public Timer::ITask
{
private:
	std::function<void ()> _function;
	std::function<void ()> _cancelled;

public:
	explicit FunctionTaskIII(std::function<void ()> function, std::function<void ()> cancelled):
		_function(function), _cancelled(cancelled) {}
	virtual ~FunctionTaskIII() {}

	virtual void run()
	{
		_function();
	}
	virtual void cancelled()
	{
		_cancelled();
	}
};

uint64_t Timer::addTask(std::function<void ()> task, std::function<void ()> cannelled, uint64_t delayMsec)
{
	ITaskPtr t(new FunctionTaskIII(std::move(task), std::move(cannelled)));
	return addTask(t, delayMsec);
}

uint64_t Timer::addPeriodTask(std::function<void ()> task, std::function<void ()> cannelled, uint64_t periodInMsec, int cycleTimes, bool runRightNow)
{
	ITaskPtr t(new FunctionTaskIII(std::move(task), std::move(cannelled)));
	return addPeriodTask(t, periodInMsec, cycleTimes, runRightNow);
}

uint64_t Timer::addRepeatedTask(std::function<void ()> task, std::function<void ()> cannelled, uint64_t intervalInMsec, int cycleTimes, bool runRightNow)
{
	ITaskPtr t(new FunctionTaskIII(std::move(task), std::move(cannelled)));
	return addRepeatedTask(t, intervalInMsec, cycleTimes, runRightNow);
}

uint64_t Timer::addDailyTask(std::function<void ()> task, std::function<void ()> cannelled, int hour, int minute, int second)
{
	ITaskPtr t(new FunctionTaskIII(std::move(task), std::move(cannelled)));
	return addDailyTask(t, hour, minute, second);
}
