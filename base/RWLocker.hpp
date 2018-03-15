#ifndef RWLocker_hpp
#define RWLocker_hpp

#include <pthread.h>

namespace fpnn {

class RWLocker
{
	pthread_rwlock_t _rwlock;
	
public:
	RWLocker()
	{
		pthread_rwlock_init(&_rwlock, NULL);
	}
	~RWLocker()
	{
		pthread_rwlock_destroy(&_rwlock);
	}

	void rlock()
	{
		pthread_rwlock_rdlock(&_rwlock);
	}
	
	void wlock()
	{
		pthread_rwlock_wrlock(&_rwlock);
	}

	void unlock()
	{
		pthread_rwlock_unlock(&_rwlock);
	}
};

class RKeeper
{
	RWLocker *_locker;
	pthread_rwlock_t *_lock;
	
public:
	RKeeper(RWLocker *locker)
	{
		_lock = NULL;
		_locker = locker;
		_locker->rlock();
	}
	RKeeper(pthread_rwlock_t *rwlock)
	{
		_locker = NULL;
		_lock = rwlock;
		pthread_rwlock_rdlock(_lock);
	}
	
	~RKeeper()
	{
		if (_lock)
			pthread_rwlock_unlock(_lock);
		else
			_locker->unlock();
	}
};

class WKeeper
{
	RWLocker *_locker;
	pthread_rwlock_t *_lock;
	
public:
	WKeeper(RWLocker *locker)
	{
		_lock = NULL;
		_locker = locker;
		_locker->wlock();
	}
	WKeeper(pthread_rwlock_t *rwlock)
	{
		_locker = NULL;
		_lock = rwlock;
		pthread_rwlock_wrlock(_lock);
	}
	
	~WKeeper()
	{
		if (_lock)
			pthread_rwlock_unlock(_lock);
		else
			_locker->unlock();
	}
};
}
#endif
