#ifndef FPNN_Task_Thread_Pool_H
#define FPNN_Task_Thread_Pool_H

/*===============================================================================
  INCLUDES AND VARIABLE DEFINITIONS
=============================================================================== */
#include <mutex>
#include <queue>
#include <list>
#include <memory>
#include <thread>
#include <functional>
#include <condition_variable>
#include "ITaskThreadPool.h"
#include "PoolInfo.h"

namespace fpnn {
/*===============================================================================
  CLASS & STRUCTURE DEFINITIONS
=============================================================================== */
class TaskThreadPool: public ITaskThreadPool
{
	public:
		class FunctionTask: public ITask
		{
			private:
				std::function<void ()> _function;

			public:
				explicit FunctionTask(std::function<void ()> function): _function(function) {}
				virtual ~FunctionTask() {}

				virtual void run()
				{
					_function();
				}
		};

	private:
		std::mutex _mutex;
		std::condition_variable _condition;
		std::condition_variable _detachCondition;

		int32_t					_initCount;
		int32_t					_appendCount;
		int32_t					_perfectCount;
		int32_t					_maxCount;
		size_t					_maxQueueLength;
		size_t					_tempThreadLatencySeconds;

		int32_t					_normalThreadCount;		//-- The number of normal work threads in pool.
		int32_t					_busyThreadCount;		//-- The number of work threads which are busy for processing.
		int32_t					_tempThreadCount;		//-- The number of temporary/overdraft work threads.

		std::queue<ITaskPtr>	_taskQueue;
		std::list<std::thread>	_threadList;

		bool					_inited;
		bool					_willExit;

		void					ReviseDataRelation();
		bool					append();
		void					process();
		void					temporaryProcess();

	public:
		virtual bool			init(int32_t initCount, int32_t perAppendCount, int32_t perfectCount, int32_t maxCount, size_t maxQueueLength = 0, size_t tempThreadLatencySeconds = 60);
		virtual bool			wakeUp(ITaskPtr task);
		virtual bool			wakeUp(std::function<void ()> task);
		#if(0)
		/*
			!!! Followed function may be very dangerous. !!!
			*. If you just put non-class-member function, without any reference of class/struct instance, following function is safety.
			*. If you put class-member function, please ENSURE the instance of class/struct is available as far as the task finish.
				Following function cannot prevent the instance destroyed, and cannot inc the reference counter of shared pointer.
			*. If you pass any reference of class/struct instance, please know: A copy constructor will be called when task running,
				and a destructor maybe called before the copy constructor calling and before the task be run.
				So, please ENSURE the instance of class/struct is available as far as the task begin running, and without any shallow copy!
		*/
		template <class Fn, class... Args>
		virtual bool			wakeUp(Fn&& func, Args&&... args)
		{
			ITaskPtr t = std::make_shared<FunctionTask>([&func, &args...](){ auto fun = std::bind(func, args...); fun(); });
			return wakeUp(t);
		}
		#endif
		virtual void			release();
		virtual void			status(int32_t &normalThreadCount, int32_t &temporaryThreadCount, int32_t &busyThreadCount, int32_t &taskQueueSize, int32_t& min, int32_t& max, int32_t& maxQueue);
		virtual std::string		infos();

		virtual bool inited()
		{
			return _inited;
		}

		virtual bool exiting()
		{
			return _willExit;
		}

		TaskThreadPool(): _initCount(0), _appendCount(0), _perfectCount(0), _maxCount(0), _maxQueueLength(0),
		_tempThreadLatencySeconds(0), _normalThreadCount(0), _busyThreadCount(0), _tempThreadCount(0),
		_inited(false), _willExit(false)
	{
	}

		virtual ~TaskThreadPool()
		{
			release();
		}
};
typedef std::shared_ptr<TaskThreadPool> TaskThreadPoolPtr;
}
#endif
