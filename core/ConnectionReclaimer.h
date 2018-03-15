#ifndef FPNN_Client_Connection_Reclaimer_H
#define FPNN_Client_Connection_Reclaimer_H

#include <unistd.h>
#include <atomic>
#include <thread>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <set>

namespace fpnn
{
	class IReleaseable;
	typedef std::shared_ptr<IReleaseable> IReleaseablePtr;
	class ConnectionReclaimer;
	typedef std::shared_ptr<ConnectionReclaimer> ConnectionReclaimerPtr;

	class IReleaseable
	{
	public:
		virtual bool releaseable() = 0;
		virtual ~IReleaseable() {}
	};

	class ConnectionReclaimer
	{
		static std::mutex _mutex;
		std::condition_variable _condition;
		std::thread _reclaimer;
		std::atomic<bool> _running;
		std::set<IReleaseablePtr> _collections;

		void reclaimer_thread()
		{
			bool remainedTask = false;
			while (_running || remainedTask)
			{
				std::set<IReleaseablePtr> deleted;
				{
					std::unique_lock<std::mutex> lck(_mutex);

					while (_collections.size() == 0 && _running)
						_condition.wait(lck);

					for (IReleaseablePtr object: _collections)
					{
						if (object->releaseable())
							deleted.insert(object);
					}
					for (IReleaseablePtr object: deleted)
						_collections.erase(object);

					remainedTask = (_collections.size() != 0);
				}
				deleted.clear();

				usleep(10000);
			}
		}

	public:
		static ConnectionReclaimer* nakedInstance();
		static ConnectionReclaimerPtr instance();

		ConnectionReclaimer(): _running(true)
		{
			_reclaimer = std::thread(&ConnectionReclaimer::reclaimer_thread, this);
		}
		~ConnectionReclaimer()
		{
			_running = false;
			_condition.notify_all();
			_reclaimer.join();
		}

		void reclaim(IReleaseablePtr object)
		{
			std::unique_lock<std::mutex> lck(_mutex);
			_collections.insert(object);
			_condition.notify_one();
		}
	};
}

#endif
