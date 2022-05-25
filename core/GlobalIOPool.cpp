#ifdef __APPLE__
	#include <unistd.h>
#else
	#include <sys/sysinfo.h>
#endif
#include "Setting.h"
#include "GlobalIOPool.h"

using namespace fpnn;

static std::atomic<bool> _created(false);
std::mutex GlobalIOPool::_mutex;
static GlobalIOPoolPtr _globalIOPool;

GlobalIOPool* GlobalIOPool::nakedInstance()
{
	if (!_created)
	{
		std::unique_lock<std::mutex> lck(_mutex);
		if (!_created)
		{
			_globalIOPool.reset(new GlobalIOPool);
			_created = true;
		}
	}
	return _globalIOPool.get();
}

GlobalIOPoolPtr GlobalIOPool::instance()
{
	if (!_created)
	{
		std::unique_lock<std::mutex> lck(_mutex);
		if (!_created)
		{
			_globalIOPool.reset(new GlobalIOPool);
			_created = true;
		}
	}

	return _globalIOPool;
}

GlobalIOPool::GlobalIOPool()
{
#ifdef __APPLE__
	int cpuCount = (int)sysconf(_SC_NPROCESSORS_ONLN);
#else
	int cpuCount = get_nprocs();
#endif
	
	if (cpuCount < 2)
		cpuCount = 2;

	int arraySize = Setting::getInt("FPNN.global.io.thread.min.size", cpuCount);
	if (arraySize <= 0 || cpuCount < arraySize)
		arraySize = cpuCount;

	_ioWorker.reset(new TCPEpollIOWorker);
	_ioPool.config(arraySize);
	_ioPool.setProcessor(_ioWorker);
}


