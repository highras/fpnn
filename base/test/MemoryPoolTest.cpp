#include <iostream>
#include "MemoryPool.h"
#include "UnlockedMemoryPool.h"

using namespace fpnn;
using std::cout;
using std::endl;

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

template<typename MPtype>
void MemoryPoolTest(size_t bolckSize)
{
	MPtype mp;
	std::list<void*> li;

	mp.init(bolckSize, 100, 10, 200, 600);

	for (int i = 0; i < count; i++)
	{
		void* memory = mp.gain();
		if (memory)
			li.push_back(memory);
			
		//if (i > 0 && i % 45 == 0)
		//{
			//cout<<"turn "<<i<<" alloc status: ";
			//status(mp);
		//}
	}
	
	cout<<"---------init finish------"<<endl;
	status(mp);

	for (void* memory: li)
		mp.recycle(memory);

	cout<<"---------recycle finish------"<<endl;
        status(mp);
}

void testFunc(size_t bolckSize)
{
	cout<<"-------------------[test MemoryPool]-------------------"<<endl;
	MemoryPoolTest<MemoryPool>(bolckSize);

	cout<<"-------------------[test UnlockedMemoryPool]-------------------"<<endl;
	MemoryPoolTest<UnlockedMemoryPool>(bolckSize);
	
}

int main()
{
	
	cout<<"-------------------[test MemoryPool with block size 4 ]-------------------"<<endl;
	testFunc(4);
	
	cout<<"-------------------[test MemoryPool with block size 8 ]-------------------"<<endl;
	testFunc(8);
	
	cout<<"-------------------[test MemoryPool with block size 32 ]-------------------"<<endl;
	testFunc(32);
	
	cout<<"-------------------[test MemoryPool with block size 256 ]-------------------"<<endl;
	testFunc(256);
	
	cout<<"-------------------[test MemoryPool with block size 512 ]-------------------"<<endl;
	testFunc(512);


	return 0;
}

