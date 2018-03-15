#include <iostream>
#include "ObjectPool.h"
#include "UnlockedObjectPool.h"

using namespace fpnn;
using std::cout;
using std::endl;

struct SD
{
	int a;
	int b;
	char c;
	double d;
};

class CDA
{
public:
	CDA()
	{
		cout<<"CDA construct"<<endl;
	}

	~CDA()
	{
		cout<<"CDA destroy"<<endl;
	}
};
class CDB
{
public:
	CDB(int a, const char* b)
	{
		cout<<"CDB construct a = "<<a<<", b = "<<b<<endl;
	}

	~CDB()
	{
		cout<<"CDB destroy"<<endl;
	}
};

template<typename MPtype>
void status(MPtype& mp)
{
	MemoryPoolStatus mps;
	mp.status(mps);
	cout<<"[Status]"<<endl;
	cout<<"used: "<<mps.usedCount<<endl;
	cout<<"free: "<<mps.freeCount<<endl;
	cout<<"overdraft: "<<mps.overdraftCount<<endl;
	cout<<"total used: "<<mps.totalUsedCount<<endl;
	cout<<"total: "<<mps.totalCount<<endl;
	cout<<endl;
}

#include <list>
const int count = 1000;

template<typename MPtype, typename UNIT>
void MemoryPoolTest()
{
	MPtype mp;
	std::list<UNIT*> li;

	mp.init(100, 10, 200, 600);

	for (int i = 0; i < count; i++)
	{
		UNIT* data = mp.gainMemory();
		if (data)
			li.push_back(data);
		//if (i > 0 && i % 45 == 0)
		//{
			//cout<<"first turn "<<i<<" ";
			//status(mp);
		//}
	}
	
	cout<<"---------init finish------"<<endl;
	status(mp);

	for (UNIT* data: li)
		mp.recycleMemory(data);

	cout<<"---------recycle finish------"<<endl;
        status(mp);
}

template<typename MPtype, typename UNIT, typename... Params>
void ObjectPoolTest(Params&&... values)
{
	MPtype mp;
	std::list<UNIT*> li;

	mp.init(100, 10, 1000, 2000);

	for (int i = 0; i < count; i++)
	{
		UNIT* data = mp.gain(std::forward<Params>(values)...);
		if (data)
			li.push_back(data);
		//if (i > 0 && i % 45 == 0)
		//{
		//	cout<<"first turn "<<i<<" ";
		//	status(mp);
		//}
	}
	
	cout<<"---------init finish------"<<endl;
	status(mp);

	for (UNIT* data: li)
		mp.recycle(data);

	cout<<"---------recycle finish------"<<endl;
        status(mp);
}

template<typename K, typename... Params>
void testFunc(Params&&... values)
{
	cout<<"-------------------[test ObjectPool as memory pool]-------------------"<<endl;
	MemoryPoolTest<ObjectPool<K>, K>();
	
	cout<<"-------------------[test ObjectPool as object pool]-------------------"<<endl;
	ObjectPoolTest<ObjectPool<K>, K>(std::forward<Params>(values)...);

	cout<<"-------------------[test UnlckedObjectPool as memory pool]-------------------"<<endl;
	MemoryPoolTest<UnlockedObjectPool<K>, K>();
	
	cout<<"-------------------[test UnlockedObjectPool as object pool]-------------------"<<endl;
	ObjectPoolTest<UnlockedObjectPool<K>, K>(std::forward<Params>(values)...);
}

int main()
{
	
	cout<<"-------------------[test ObjectPool with int type ]-------------------"<<endl;
	testFunc<int>();
	cout<<"-------------------[test ObjectPool with struct SD type ]-------------------"<<endl;
	testFunc<SD>();
	cout<<"-------------------[test ObjectPool with class CDA without init params]-------------------"<<endl;
	testFunc<CDA>();
	cout<<"-------------------[test ObjectPool with class CDB with init params]-------------------"<<endl;
	testFunc<CDB>(100, "hello, Kitty!");

	return 0;
}

