#include <sys/sysinfo.h>
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
	int cpuCount = get_nprocs();
	if (cpuCount < 2)
		cpuCount = 2;

	_ioWorker.reset(new TCPEpollIOWorker);
	_ioPool.config(cpuCount);
	_ioPool.setProcessor(_ioWorker);
}


