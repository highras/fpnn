#ifndef FPNN_Task_Thread_Pool_Array_H
#define FPNN_Task_Thread_Pool_Array_H

#include <atomic>
#include <vector>
#include "TaskThreadPool.h"
#include "PoolInfo.h"

namespace fpnn {
class TaskThreadPoolArray: public ITaskThreadPool
{
	std::atomic<uint64_t> _idx;
	std::vector<TaskThreadPool*> _array;

public:
	TaskThreadPoolArray(int count): _idx(0)
	{
		for (int i = 0; i < count; i++)
		{
			_array.push_back(new TaskThreadPool());
		}
	}

	TaskThreadPoolArray(): _idx(0) {}

	virtual ~TaskThreadPoolArray()
	{
		release();

		for (size_t i = 0; i < _array.size(); i++)
			delete _array[i];
	}

	void config(int count)
	{
		if (_array.size())
			return;

		for (int i = 0; i < count; i++)
		{
			_array.push_back(new TaskThreadPool());
		}
	}

	virtual bool init(int32_t initCount, int32_t perAppendCount, int32_t perfectCount, int32_t maxCount, size_t maxQueueLength = 0, size_t tempThreadLatencySeconds = 60)
	{
		int count = (int)_array.size();
		if (!count)
			return false;

		int realInit = initCount / count;
		int realAppend = perAppendCount / count;
		int realPerfect = perfectCount / count;
		int realMax = maxCount / count;
		size_t realMaxQueueLength = maxQueueLength / count;

		if (!realInit) realInit = 1;
		if (!realPerfect) realPerfect = 1;
		if (!realMax) realMax = 1;
		if (perAppendCount && !realAppend) realAppend = 1;
		if (maxQueueLength && !realMaxQueueLength) realMaxQueueLength = 1;

		for (size_t i = 0; i < _array.size(); i++)
			if (!_array[i]->init(realInit, realAppend, realPerfect, realMax, realMaxQueueLength, tempThreadLatencySeconds))
			{
				for (size_t k = 0; k < i; k++)
				{
					_array[k]->release();
				}
				return false;
			}

		return true;
	}

	bool wakeUp(int hint, TaskThreadPool::ITaskPtr task)
	{
		int idx = hint % (int)_array.size();
		return _array[idx]->wakeUp(task);
	}

	virtual bool wakeUp(TaskThreadPool::ITaskPtr task)
	{
		int idx = _idx++ % (int)_array.size();
		return _array[idx]->wakeUp(task);
	}

	bool wakeUp(int hint, std::function<void ()> task)
	{
		int idx = hint % (int)_array.size();
		return _array[idx]->wakeUp(std::move(task));
	}

	virtual bool wakeUp(std::function<void ()> task)
	{
		int idx = _idx++ % (int)_array.size();
		return _array[idx]->wakeUp(std::move(task));
	}

	virtual void release()
	{
		for (size_t i = 0; i < _array.size(); i++)
			_array[i]->release();
	}


	virtual void status(int32_t &normalThreadCount, int32_t &temporaryThreadCount, int32_t &busyThreadCount, int32_t &taskQueueSize, int32_t& min, int32_t& max, int32_t& maxQueue)
	{
		normalThreadCount = 0;
		temporaryThreadCount = 0;
		busyThreadCount = 0;
		taskQueueSize = 0;
		min = 0;
		max = 0;
		maxQueue = 0;

		for (size_t i = 0; i < _array.size(); i++)
		{
			int32_t n, t, b, q, mi, ma, mq;
			_array[i]->status(n, t, b, q, mi, ma, mq);

			normalThreadCount += n;
			temporaryThreadCount += t;
			busyThreadCount += b;
			taskQueueSize += q;
			min += mi;
			max += ma;
			maxQueue += mq;
		}
	}

	virtual std::string infos() {
		int32_t min = 0, max = 0;
		int32_t normalThreadCount = 0;
		int32_t temporaryThreadCount = 0;
		int32_t busyThreadCount = 0;
		int32_t taskQueueSize = 0;
		int32_t maxQueueLength = 0;

		status(normalThreadCount, temporaryThreadCount, busyThreadCount, taskQueueSize, min, max, maxQueueLength);

		return PoolInfo::threadPoolInfo(min, max, normalThreadCount, temporaryThreadCount, busyThreadCount, taskQueueSize,maxQueueLength);
	}

	virtual bool inited()
	{
		return _array[0]->inited();
	}

	virtual bool exiting()
	{
		return _array[0]->exiting();
	}
};
}
#endif
