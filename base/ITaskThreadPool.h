#ifndef FPNN_Task_Thread_Pool_Interface_H
#define FPNN_Task_Thread_Pool_Interface_H

#include <stdint.h>
#include <functional>
#include <memory>

namespace fpnn
{
	class ITaskThreadPool
	{
	public:
		class ITask
		{
			public:
				virtual void run() = 0;
				virtual ~ITask() {}
		};
		typedef std::shared_ptr<ITask> ITaskPtr;

	public:
		virtual bool init(int32_t initCount, int32_t appendCount, int32_t perfectCount, int32_t maxCount, size_t maxQueueLength = 0, size_t tempThreadLatencySeconds = 60) = 0;
		virtual bool wakeUp(ITaskPtr task) = 0;
		virtual bool wakeUp(std::function<void ()> task) = 0;
		virtual void release() = 0;
		virtual void status(int32_t &normalThreadCount, int32_t &temporaryThreadCount, int32_t &busyThreadCount, int32_t &taskQueueSize, int32_t& min, int32_t& max, int32_t& maxQueue) = 0;
		virtual std::string infos() = 0;

		virtual bool inited() = 0;
		virtual bool exiting() = 0;

		virtual ~ITaskThreadPool() {}
	};
	typedef std::shared_ptr<ITaskThreadPool> ITaskThreadPoolPtr;
}


#endif
