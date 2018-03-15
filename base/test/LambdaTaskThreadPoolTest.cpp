#include <iostream>
#include <string>
#include <unistd.h>
#include "TaskThreadPool.h"

using namespace fpnn;
using std::cout;
using std::endl;

int main()
{
	TaskThreadPool tp;
	tp.init(4, 3, 20, 60);

	{
		int32_t normal, temp, busy, queue, min, max, maxQueue;
		tp.status(normal, temp, busy, queue, min, max, maxQueue);
		cout<<"[Status] normal: "<<normal<<", temp: "<<temp<<", busy: "<<busy<<", queue: "<<queue<<endl;
		sleep(1);
	}

	for (int i = 0; i < 100; i++)
	{
		std::string name = (i%2) ? "Kittly" : "Dreamon";
		tp.wakeUp([i, name](){
			sleep(i);
			cout<<"Hello, "<<name<<". Times: "<<i<<endl;
		});
	}

	for (int i = 0; i < 100; i++)
	{
		int32_t normal, temp, busy, queue, min, max, maxQueue;
		tp.status(normal, temp, busy, queue, min, max, maxQueue);
		cout<<"[Status] normal: "<<normal<<", temp: "<<temp<<", busy: "<<busy<<", queue: "<<queue<<endl;
		sleep(1);
	}
	//cout<<"will exiting ..."<<endl;
	return 0;
}
