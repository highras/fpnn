#ifndef FPNN_Param_Thread_Pool_H
#define FPNN_Param_Thread_Pool_H

/*===============================================================================
  INCLUDES AND VARIABLE DEFINITIONS
  =============================================================================== */
#include <mutex>
#include <queue>
#include <list>
#include <memory>
#include <thread>
#include <condition_variable>
#include "PoolInfo.h"
namespace fpnn {
/*===============================================================================
  CLASS & STRUCTURE DEFINITIONS
  =============================================================================== */
class ParamThreadPool
{
	public:
		class IProcessor
		{
			public:
				/** Please release param in run() function. */
				virtual void run(void * param) = 0;
				virtual ~IProcessor() {}
		};
		typedef std::shared_ptr<IProcessor> IProcessorPtr;

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

		std::queue<void *>		_taskQueue;
		std::list<std::thread>	_threadList;

		IProcessorPtr			_processor;

		bool					_inited;
		bool					_willExit;

		void					ReviseDataRelation();
		bool					append();
		void					process();
		void					temporaryProcess();

	public:
		bool					init(int32_t initCount, int32_t perAppendCount, int32_t perfectCount, int32_t maxCount, size_t maxQueueLength = 0, size_t tempThreadLatencySeconds = 60);
		bool					wakeUp(void * task);
		void					release();
		void					status(int32_t &normalThreadCount, int32_t &temporaryThreadCount, int32_t &busyThreadCount, int32_t &taskQueueSize, int32_t& min, int32_t& max, int32_t& maxQueue);
		std::string				infos();

		inline void				setProcessor(IProcessorPtr processor)
		{
			std::unique_lock<std::mutex> lck(_mutex);
			_processor = processor;
		}

		inline IProcessorPtr		processor()
		{
			std::unique_lock<std::mutex> lck(_mutex);
			return _processor;
		}

		inline bool inited()
		{
			return _inited;
		}

		inline bool exiting()
		{
			return _willExit;
		}

		ParamThreadPool(IProcessorPtr processor): _initCount(0), _appendCount(0), _perfectCount(0), _maxCount(0),
		_maxQueueLength(0), _tempThreadLatencySeconds(0), _normalThreadCount(0), _busyThreadCount(0), _tempThreadCount(0),
		_processor(processor), _inited(false), _willExit(false)
	{
	}

		ParamThreadPool(): ParamThreadPool(nullptr) {}

		~ParamThreadPool()
		{
			release();
		}
};
}
#endif
