#ifndef FPNN_Timer_h
#define FPNN_Timer_h

#include <atomic>
#include <map>
#include <mutex>
#include <memory>
#include <queue>
#include <thread>
#include "msec.h"
#include "FPLog.h"
#include "ITaskThreadPool.h"

namespace fpnn {
	class Timer;
	typedef std::shared_ptr<Timer> TimerPtr;

	class Timer: public std::enable_shared_from_this<Timer>
	{
	public:
		class ITask
		{
		public:
			virtual void run() = 0;
			virtual void cancelled() {}		//-- When Timer exiting.
			virtual ~ITask() {}
		};
		typedef std::shared_ptr<ITask> ITaskPtr;

		struct TimerTask
		{
			bool _removed;
			bool _rescheduleWhenTrigger;
			uint64_t _triggeredTimeMsec;	//-- schedule weight
			ITaskPtr _task;

			TimerTask(ITaskPtr task): _removed(false), _rescheduleWhenTrigger(false), _triggeredTimeMsec(0), _task(task) {}
			virtual ~TimerTask() {}
			virtual void adjustSchedule() = 0;
			virtual bool reschedulable() = 0;
		};
		typedef std::shared_ptr<TimerTask> TimerTaskPtr;

	private:
		class TimeTaskPriorityComp
		{
		public:
			bool operator() (const TimerTaskPtr& a, const TimerTaskPtr& b) const
			{
				return a->_triggeredTimeMsec > b->_triggeredTimeMsec;  //-- smaller is first. MinHeap.
			}
		};

		class TimerThreadTask: public ITaskThreadPool::ITask
		{
			TimerPtr _timer;
			Timer::TimerTaskPtr _timerTask;

		public:
			TimerThreadTask(TimerPtr timer, Timer::TimerTaskPtr timerTask): _timer(timer), _timerTask(timerTask) {}
			virtual ~TimerThreadTask() {}

			virtual void run()
			{
				try {
					_timerTask->_task->run();
				}
				catch (...)
				{
					LOG_ERROR("Unknown error when running customer's task.");
				}

				if (_timerTask->_rescheduleWhenTrigger == false)
					_timer->reschedule(_timerTask);
			}
		};

	private:
#ifdef __APPLE__
		int _kqueue_fd;
#else
		int _epoll_fd;
		int _max_events;
		struct epoll_event* _epollEvents;
#endif
		int _eventNotifyFds[2];

		std::mutex _mutex;
		std::atomic<bool> _running;

		std::thread _eventThread;
		ITaskThreadPoolPtr _threadPool;
		std::priority_queue<TimerTaskPtr, std::vector<TimerTaskPtr>, TimeTaskPriorityComp> _priorityQueue;

		//-- for removing operations.
		uint64_t _idGen;
		std::queue<uint64_t> _recycledIdPool;	//-- build buffer area. Will take 7~8M memory.
		std::map<TimerTaskPtr, uint64_t> _taskMap;
		std::map<uint64_t, TimerTaskPtr> _taskIdMap;

		void eventThread();
		int checkTasks();
#ifdef __APPLE__
		bool initKqueue();
		void stopKqueue();
#else
		bool initEpoll();
		void stopEpoll();
#endif
		void stop();
		void clean();
		void cleanTaskQueue();
		void wakeUpEpollWithLocker();
		void wakeUpEpollWithoutLocker();
		void executeTask(TimerTaskPtr task);

		uint64_t assignId();
		void removeTask(TimerTaskPtr task);
		void reschedule(TimerTaskPtr task);
		void rescheduleWithoutLocker(TimerTaskPtr task);

#ifdef __APPLE__
		Timer(): _kqueue_fd(0), _running(false), _idGen(1)
#else
		Timer(): _epoll_fd(0), _max_events(4), _epollEvents(NULL), _running(false), _idGen(1)
#endif
		{
			_eventNotifyFds[0] = 0;
			_eventNotifyFds[1] = 0;
		}

	public:
		inline static TimerPtr create() { return TimerPtr(new Timer()); }
		inline static TimerPtr create(ITaskThreadPoolPtr threadPool)
		{
			TimerPtr timer(new Timer());
			return timer->init(threadPool) ? timer : nullptr;
		}
		~Timer() { stop(); }

		bool init(ITaskThreadPoolPtr threadPool);

		/*
		  Intro:
		  	addPeriodTask(): periodInMsec will be re-calculated when task triggered.
		  	addRepeatedTask(): intervalInMsec will be re-calculated after task done.
		  	  Repeated: |- intervalInMsec -|- task executing time -|- intervalInMsec -|- task next executed -|- ...
		  	  Period: |- periodInMsec -|- periodInMsec -|- periodInMsec -|- ...

		  Tolerance for period task:
		  	In normal load, 5 msec.
		  	heavy load will be larger.

		  Parameters:
			cycleTimes: 0 means infinite.
			hour: using UTC time.

		  Return Value:
			return 0 is meaning false, lager Zero, is success, and the value is taskId. Using for removing task.
			!!! taskId will be reused. !!!
		*/
		uint64_t addTask(ITaskPtr task, uint64_t delayMsec);
		uint64_t addPeriodTask(ITaskPtr task, uint64_t periodInMsec, int cycleTimes = 0, bool runRightNow = false);
		uint64_t addRepeatedTask(ITaskPtr task, uint64_t intervalInMsec, int cycleTimes = 0, bool runRightNow = false);
		uint64_t addDailyTask(ITaskPtr task, int hour, int minute, int second);

		uint64_t addTask(std::function<void ()> task, uint64_t delayMsec);
		uint64_t addPeriodTask(std::function<void ()> task, uint64_t periodInMsec, int cycleTimes = 0, bool runRightNow = false);
		uint64_t addRepeatedTask(std::function<void ()> task, uint64_t intervalInMsec, int cycleTimes = 0, bool runRightNow = false);
		uint64_t addDailyTask(std::function<void ()> task, int hour, int minute, int second);

		//-- std::function<void (bool): bool means: true: normal, false: Timer is exiting.
		uint64_t addTask(std::function<void (bool)> task, uint64_t delayMsec);
		uint64_t addPeriodTask(std::function<void (bool)> task, uint64_t periodInMsec, int cycleTimes = 0, bool runRightNow = false);
		uint64_t addRepeatedTask(std::function<void (bool)> task, uint64_t intervalInMsec, int cycleTimes = 0, bool runRightNow = false);
		uint64_t addDailyTask(std::function<void (bool)> task, int hour, int minute, int second);

		uint64_t addTask(std::function<void ()> task, std::function<void ()> cannelled, uint64_t delayMsec);
		uint64_t addPeriodTask(std::function<void ()> task, std::function<void ()> cannelled, uint64_t periodInMsec, int cycleTimes = 0, bool runRightNow = false);
		uint64_t addRepeatedTask(std::function<void ()> task, std::function<void ()> cannelled, uint64_t intervalInMsec, int cycleTimes = 0, bool runRightNow = false);
		uint64_t addDailyTask(std::function<void ()> task, std::function<void ()> cannelled, int hour, int minute, int second);

		void removeTask(uint64_t taskId);
	};
}
#endif
