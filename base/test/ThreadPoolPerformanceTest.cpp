#include <iostream>
#include <string>
#include <atomic>
#include <unistd.h>
#include <sys/time.h>
#include "ParamThreadPool.h"
#include "ParamTemplateThreadPool.h"
#include "TaskThreadPool.h"

using namespace fpnn;
using std::cout;
using std::endl;

int cycle = 10000 * 10;
int perfectThreadCount = 4;
const int maxThreadCount = 20;
const int appThreadCount = 2;

//std::atomic<int> localValue(0);
//std::atomic<int> localCount(0);

int localValue(0);
int localCount(0);

struct Params
{
	int times;
	int value;
};

class DealerPTTP: public ParamTemplateThreadPool<struct Params *>::IProcessor
{
public:
	virtual void run(struct Params * param)
	{
		localValue += param->times & param->value;
		localCount += 1;
		delete param;
	}

	virtual ~DealerPTTP()	{}
};

class DealerPTP: public ParamThreadPool::IProcessor
{
public:
	virtual void run(void * param)
	{
		Params* data = (Params*)param;
		localValue += data->times & data->value;
		localCount += 1;
		delete data;
	}

	virtual ~DealerPTP()	{}
};

class Task: public TaskThreadPool::ITask
{
	int _times;
	int _value;
public:
	Task(int times, int v): _times(times), _value(v) {}

	virtual void run()
	{
		localValue += _times & _value;
		localCount += 1;
	}

	virtual ~Task()	{}
};

struct timeval diff_timeval(struct timeval start, struct timeval finish)
{
	struct timeval diff;
	diff.tv_sec = finish.tv_sec - start.tv_sec;
	
	if (finish.tv_usec >= start.tv_usec)
		diff.tv_usec = finish.tv_usec - start.tv_usec;
	else
	{
		diff.tv_usec = 1000 * 1000 + finish.tv_usec - start.tv_usec;
		diff.tv_sec -= 1;
	}
	
	return diff;
}

void testPTTP()
{
	ParamTemplateThreadPool<struct Params *>* tp = new ParamTemplateThreadPool<struct Params *>(std::make_shared<DealerPTTP>());
	tp->init(4, appThreadCount, perfectThreadCount, maxThreadCount, 0, 60);

	{
		int32_t normal, temp, busy, queue, min, max, maxQueue;
		tp->status(normal, temp, busy, queue, min, max, maxQueue);
		cout<<"[Status] normal: "<<normal<<", temp: "<<temp<<", busy: "<<busy<<", queue: "<<queue<<endl;
		sleep(1);
	}

	localValue = 0;
	localCount  = 0;
	struct timeval start, end;
	gettimeofday(&start, NULL);

	for (int i = 0; i < cycle; i++)
	{
		Params *p = new Params;
		p->times = i;
		p->value = i & 0xac;

		tp->wakeUp(p);
	}

	delete tp;
	gettimeofday(&end, NULL);

	struct timeval cost = diff_timeval(start, end);
	cout<<"test category: ParamTemplateTheadPool, localValue: "<<localValue<<", count: "<<localCount<<" times cost "<<cost.tv_sec<<" sec "<<(cost.tv_usec/1000)<<" msec"<<endl;
}

void testPTP()
{
	ParamThreadPool* tp = new ParamThreadPool(std::make_shared<DealerPTP>());
	tp->init(4, appThreadCount, perfectThreadCount, maxThreadCount, 0, 60);

	{
		int32_t normal, temp, busy, queue, min, max, maxQueue;
		tp->status(normal, temp, busy, queue, min, max, maxQueue);
		cout<<"[Status] normal: "<<normal<<", temp: "<<temp<<", busy: "<<busy<<", queue: "<<queue<<endl;
		sleep(1);
	}

	localValue = 0;
	localCount  = 0;
	struct timeval start, end;
	gettimeofday(&start, NULL);

	for (int i = 0; i < cycle; i++)
	{
		Params *p = new Params;
		p->times = i;
		p->value = i & 0xac;

		tp->wakeUp(p);
	}

	delete tp;
	gettimeofday(&end, NULL);

	struct timeval cost = diff_timeval(start, end);
	cout<<"test category: ParamThreadPool, localValue: "<<localValue<<", count: "<<localCount<<" times cost "<<cost.tv_sec<<" sec "<<(cost.tv_usec/1000)<<" msec"<<endl;
}

void testTTP()
{
	TaskThreadPool* tp = new TaskThreadPool();
	tp->init(4, appThreadCount, perfectThreadCount, maxThreadCount, 0, 60);

	{
		int32_t normal, temp, busy, queue, min, max, maxQueue;
		tp->status(normal, temp, busy, queue, min, max, maxQueue);
		cout<<"[Status] normal: "<<normal<<", temp: "<<temp<<", busy: "<<busy<<", queue: "<<queue<<endl;
		sleep(1);
	}

	localValue = 0;
	localCount  = 0;
	struct timeval start, end;
	gettimeofday(&start, NULL);

	for (int i = 0; i < cycle; i++)
	{
		tp->wakeUp(std::make_shared<Task>(i, i & 0xac));
	}

	delete tp;
	gettimeofday(&end, NULL);

	struct timeval cost = diff_timeval(start, end);
	cout<<"test category: TaskThreadPool, localValue: "<<localValue<<", count: "<<localCount<<" times cost "<<cost.tv_sec<<" sec "<<(cost.tv_usec/1000)<<" msec"<<endl;
}

void testHold(int times4Cycle, int perfectThread)
{
	cycle = times4Cycle * 10000;
	perfectThreadCount = perfectThread;

	cout<<"------------ cycleTimes "<<cycle<<" ---- perfectThreadCount "<<perfectThreadCount<<" ---------"<<endl;
	testPTTP();
	testPTP();
	testTTP();
}

int main()
{
	testHold(10, 4);
	testHold(10, 8);
	testHold(100, 4);
	testHold(100, 8);

	return 0;
}
