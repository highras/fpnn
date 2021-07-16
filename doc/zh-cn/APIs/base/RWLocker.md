## RWLocker

### 介绍

Pthread 读写锁的 C++ 11 封装。

### 命名空间

	namespace fpnn;

### RWLocker

	class RWLocker
	{
	public:
		RWLocker();
		~RWLocker();

		void rlock();	
		void wlock();
		void unlock();
	};

#### rlock

	void rlock();

加读锁。

#### wlock

	void wlock();

加写锁。

#### unlock

	void unlock();

解锁。

### RKeeper

	class RKeeper
	{
	public:
		RKeeper(RWLocker *locker);
		RKeeper(pthread_rwlock_t *rwlock);
		~RKeeper();
	};

读操作守卫。类似于 C++ 11 的 lock_guard。当超出作用域后，读锁自动解除。


### WKeeper

	class WKeeper
	{
	public:
		WKeeper(RWLocker *locker);
		WKeeper(pthread_rwlock_t *rwlock);
		~WKeeper();
	};

写操作守卫。类似于 C++ 11 的 lock_guard。当超出作用域后，写锁自动解除。
