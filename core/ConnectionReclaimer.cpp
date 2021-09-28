#include "ConnectionReclaimer.h"

using namespace fpnn;

static std::atomic<bool> _created(false);
static std::mutex _createMutex;
static ConnectionReclaimerPtr _reclarimer;

ConnectionReclaimer* ConnectionReclaimer::nakedInstance()
{
	if (!_created)
	{
		std::unique_lock<std::mutex> lck(_createMutex);
		if (!_created)
		{
			_reclarimer.reset(new ConnectionReclaimer);
			_created = true;
		}
	}
	return _reclarimer.get();
}

ConnectionReclaimerPtr ConnectionReclaimer::instance()
{
	if (!_created)
	{
		std::unique_lock<std::mutex> lck(_createMutex);
		if (!_created)
		{
			_reclarimer.reset(new ConnectionReclaimer);
			_created = true;
		}
	}

	return _reclarimer;
}
