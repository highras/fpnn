#include <iostream>
#include <string>
#include <unistd.h>
#include "TaskThreadPool.h"

using namespace fpnn;
using std::cout;
using std::endl;

class Task: public TaskThreadPool::ITask
{
	int _times;
	std::string _name;
public:
	Task(int times, const std::string& name): _times(times), _name(name) {}

	virtual void run()
	{
		sleep(_times);
		cout<<"Hello, "<<_name<<". Times: "<<_times<<endl;
	}

	virtual ~Task()	{}
};

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
		tp.wakeUp(std::make_shared<Task>(i, (i%2) ? "Kittly" : "Dreamon"));
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
