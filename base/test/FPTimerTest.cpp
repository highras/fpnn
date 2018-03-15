// g++ --std=c++11 -I.. -o FPTimerTest FPTimerTest.cpp -L.. -lfpbase -lcurl -lpthread

#include <unistd.h>
#include <string.h>
#include <iostream>
#include <string>
#include <time.h>
#include <mutex>
#include "msec.h"
#include "FPTimer.h"
#include "TaskThreadPool.h"

using namespace std;
using namespace fpnn;

std::mutex _mutex;
class Task: public Timer::ITask
{
	string _name;
	int64_t _periodMsec;
	int64_t _lastRun;
	int _times;

public:
	virtual void run()
	{
		int64_t now = slack_real_msec();
		int64_t diff = now - _lastRun;
		if (_times)
		{
			std::unique_lock<std::mutex> lck(_mutex);
			cout<<"Task "<<_name<<" executed. now: "<<now<<", periodMsec: "<<_periodMsec<<", diff "<<diff<<", times: "<<_times<<endl;
			_times--;
		}
		else
		{
			std::unique_lock<std::mutex> lck(_mutex);
			cout<<"Task "<<_name<<" executed. now: "<<now<<", periodMsec: "<<_periodMsec<<", diff "<<diff<<endl;
		}

		_lastRun = slack_real_msec();
	}
	virtual void cancelled()
	{
		std::unique_lock<std::mutex> lck(_mutex);
		cout<<"Task "<<_name<<" cancelled."<<endl;
	}
	Task(const std::string& name, int periodMsec = 0, int times = 0): _name(name), _periodMsec(periodMsec), _times(times)
	{
		_lastRun = slack_real_msec();
	}
	~Task() {}
};
typedef std::shared_ptr<Task> TaskPtr;

void testTask(TimerPtr timer)
{
	TaskPtr rightNowOnce(new Task("rightNowOnce (AAA)", 0));
	TaskPtr delayOnce(new Task("delayOnce (BBB)", 2000));
	TaskPtr periodOnce(new Task("periodOnce (CCC)", 3000));
	TaskPtr period10(new Task("period10 (DDD)", 2000, 10));
	TaskPtr period10RighrNow(new Task("period10RightNow (EEE)", 2000, 10));
	TaskPtr forever(new Task("forever (FFF)", 5000, 0));

	timer->addTask(rightNowOnce, 0);
	timer->addTask(delayOnce, 2000);

	timer->addPeriodTask(periodOnce, 3000, 1);
	timer->addPeriodTask(period10, 2000, 10);
	timer->addPeriodTask(period10RighrNow, 2000, 10, true);
	timer->addPeriodTask(forever, 5000);
}

void testFuncTask(TimerPtr timer)
{
	int64_t now = slack_real_msec();

	timer->addTask([now](){
		int64_t now2 = slack_real_msec();
		std::unique_lock<std::mutex> lck(_mutex);
		cout<<"Func Task (AAA) right now once! diff "<<(now2 - now)<<endl;
	}, 0);

	timer->addTask([now](){
		int64_t now2 = slack_real_msec();
		std::unique_lock<std::mutex> lck(_mutex);
		cout<<"Func Task (BBB) delay 2000 msec! diff "<<(now2 - now)<<endl;
	}, 2000);

	timer->addPeriodTask([now](){
		int64_t now2 = slack_real_msec();
		std::unique_lock<std::mutex> lck(_mutex);
		cout<<"Func Task (CCC) period once delay 3000 msec! diff "<<(now2 - now)<<endl;
	}, 3000, 1);

	timer->addPeriodTask([now](){
		static int times = 10;
		int64_t now2 = slack_real_msec();
		std::unique_lock<std::mutex> lck(_mutex);
		cout<<"Func Task (DDD) period 10 period 2000 msec! diff "<<(now2 - now)<<", times: "<<times<<endl;
		times--;
	}, 2000, 10);

	timer->addPeriodTask([now](){
		static int times = 10;
		int64_t now2 = slack_real_msec();
		std::unique_lock<std::mutex> lck(_mutex);
		cout<<"Func Task (EEE) period 10 period 2000 msec right now! diff "<<(now2 - now)<<", times: "<<times<<endl;
		times--;
	}, 2000, 10, true);

	timer->addPeriodTask([now](){
		int64_t now2 = slack_real_msec();
		std::unique_lock<std::mutex> lck(_mutex);
		cout<<"Func Task (FFF) period forever period 5000 msec! diff "<<(now2 - now)<<endl;
	}, 5000);
}

void testParamFuncTask(TimerPtr timer)
{
	int64_t now = slack_real_msec();

	timer->addTask([now](bool running){
		int64_t now2 = slack_real_msec();
		std::unique_lock<std::mutex> lck(_mutex);
		cout<<"Func(bool) Task (AAA) right now once! diff "<<(now2 - now)<<endl;
	}, 0);

	timer->addTask([now](bool running){
		int64_t now2 = slack_real_msec();
		std::unique_lock<std::mutex> lck(_mutex);
		cout<<"Func(bool) Task (BBB) delay 2000 msec! diff "<<(now2 - now)<<endl;
	}, 2000);

	timer->addPeriodTask([now](bool running){
		int64_t now2 = slack_real_msec();
		std::unique_lock<std::mutex> lck(_mutex);
		cout<<"Func(bool) Task (CCC) period once delay 3000 msec! diff "<<(now2 - now)<<endl;
	}, 3000, 1);

	timer->addPeriodTask([now](bool running){
		static int times = 10;
		int64_t now2 = slack_real_msec();
		std::unique_lock<std::mutex> lck(_mutex);
		cout<<"Func(bool) Task (DDD) period 10 period 2000 msec! diff "<<(now2 - now)<<", times: "<<times<<endl;
		times--;
	}, 2000, 10);

	timer->addPeriodTask([now](bool running){
		static int times = 10;
		int64_t now2 = slack_real_msec();
		std::unique_lock<std::mutex> lck(_mutex);
		cout<<"Func(bool) Task (EEE) period 10 period 2000 msec right now! diff "<<(now2 - now)<<", times: "<<times<<endl;
		times--;
	}, 2000, 10, true);

	timer->addPeriodTask([now](bool running){
		if (running == false)
		{
			std::unique_lock<std::mutex> lck(_mutex);
			cout<<"[Cancelled] Func(bool) Task (FFF) period forever period 5000 msec!"<<endl;
			return;
		}
		int64_t now2 = slack_real_msec();
		std::unique_lock<std::mutex> lck(_mutex);
		cout<<"Func(bool) Task (FFF) period forever period 5000 msec! diff "<<(now2 - now)<<endl;
	}, 5000);
}

void testDoubleFuncTask(TimerPtr timer)
{
	int64_t now = slack_real_msec();

	timer->addTask([now](){
		int64_t now2 = slack_real_msec();
		std::unique_lock<std::mutex> lck(_mutex);
		cout<<"Func(double) Task (AAA) right now once! diff "<<(now2 - now)<<endl;
	},
	[](){
		std::unique_lock<std::mutex> lck(_mutex);
		cout<<"[Cancelled] Func(double) Task (AAA) right now once!"<<endl;
	}, 0);

	timer->addTask([now](){
		int64_t now2 = slack_real_msec();
		std::unique_lock<std::mutex> lck(_mutex);
		cout<<"Func(double) Task (BBB) delay 2000 msec! diff "<<(now2 - now)<<endl;
	},
	[](){
		std::unique_lock<std::mutex> lck(_mutex);
		cout<<"[Cancelled] Func(double) (BBB) Task delay 2000 msec!"<<endl;
	}, 2000);

	timer->addPeriodTask([now](){
		int64_t now2 = slack_real_msec();
		std::unique_lock<std::mutex> lck(_mutex);
		cout<<"Func(double) Task (CCC) period once delay 3000 msec! diff "<<(now2 - now)<<endl;
	},
	[](){
		std::unique_lock<std::mutex> lck(_mutex);
		cout<<"[Cancelled] Func(double) Task (CCC) period once delay 3000 msec!"<<endl;
	}, 3000, 1);

	timer->addPeriodTask([now](){
		static int times = 10;
		int64_t now2 = slack_real_msec();
		std::unique_lock<std::mutex> lck(_mutex);
		cout<<"Func(double) Task (DDD) period 10 period 2000 msec! diff "<<(now2 - now)<<", times: "<<times<<endl;
		times--;
	},
	[](){
		std::unique_lock<std::mutex> lck(_mutex);
		cout<<"[Cancelled] Func(double) Task (DDD) period 10 period 2000 msec!"<<endl;
	}, 2000, 10);

	timer->addPeriodTask([now](){
		static int times = 10;
		int64_t now2 = slack_real_msec();
		std::unique_lock<std::mutex> lck(_mutex);
		cout<<"Func(double) Task (EEE) period 10 period 2000 msec right now! diff "<<(now2 - now)<<", times: "<<times<<endl;
		times--;
	},
	[](){
		std::unique_lock<std::mutex> lck(_mutex);
		cout<<"[Cancelled] Func(double) Task (EEE) period 10 period 2000 msec right now!"<<endl;
	}, 2000, 10, true);

	timer->addPeriodTask([now](){
		int64_t now2 = slack_real_msec();
		std::unique_lock<std::mutex> lck(_mutex);
		cout<<"Func(double) Task (FFF) period forever period 5000 msec! diff "<<(now2 - now)<<endl;
	},
	[](){
		std::unique_lock<std::mutex> lck(_mutex);
		cout<<"[Cancelled] Func(double) Task (FFF) period forever period 5000 msec!"<<endl;
	}, 5000);
}

void testDiffRepatedPeriodTask(TimerPtr timer)
{
	int64_t now = slack_real_msec();

	timer->addRepeatedTask([now](){
		static int turn = 0;
		turn++;
		int64_t now2 = slack_real_msec();
		{
			std::unique_lock<std::mutex> lck(_mutex);
			cout<<"Repeated Task (AAA) interval 5000 msec called! will sleep 2 sec. turn "<<turn<<", diff "<<(now2 - now)<<endl;
		}
		sleep(2);

		now2 = slack_real_msec();
		{
			std::unique_lock<std::mutex> lck(_mutex);
			cout<<"Repeated Task (AAA) interval 5000 msec DONE!. turn "<<turn<<", diff "<<(now2 - now)<<endl;
		}

	}, 5000);

	timer->addPeriodTask([now](){
		static int turn = 0;
		turn++;
		int64_t now2 = slack_real_msec();
		{
			std::unique_lock<std::mutex> lck(_mutex);
			cout<<"Period Task (BBB) period 5000 msec called! will sleep 2 sec. turn "<<turn<<", diff "<<(now2 - now)<<endl;
		}
		sleep(2);

		now2 = slack_real_msec();
		{
			std::unique_lock<std::mutex> lck(_mutex);
			cout<<"Period Task (BBB) period 5000 msec DONE!. turn "<<turn<<", diff "<<(now2 - now)<<endl;
		}
	}, 5000);
}

void testDiffRepatedPeriodRightNowTask(TimerPtr timer)
{
	int64_t now = slack_real_msec();

	timer->addRepeatedTask([now](){
		static int turn = 0;
		turn++;
		int64_t now2 = slack_real_msec();
		{
			std::unique_lock<std::mutex> lck(_mutex);
			cout<<"Repeated Task (AAA) interval 5000 msec called! will sleep 2 sec. turn "<<turn<<", diff "<<(now2 - now)<<endl;
		}
		sleep(2);

		now2 = slack_real_msec();
		{
			std::unique_lock<std::mutex> lck(_mutex);
			cout<<"Repeated Task (AAA) interval 5000 msec DONE!. turn "<<turn<<", diff "<<(now2 - now)<<endl;
		}

	}, 5000, 0, true);

	timer->addPeriodTask([now](){
		static int turn = 0;
		turn++;
		int64_t now2 = slack_real_msec();
		{
			std::unique_lock<std::mutex> lck(_mutex);
			cout<<"Period Task (BBB) period 5000 msec called! will sleep 2 sec. turn "<<turn<<", diff "<<(now2 - now)<<endl;
		}
		sleep(2);

		now2 = slack_real_msec();
		{
			std::unique_lock<std::mutex> lck(_mutex);
			cout<<"Period Task (BBB) period 5000 msec DONE!. turn "<<turn<<", diff "<<(now2 - now)<<endl;
		}
	}, 5000, 0, true);
}

void testDailyTask(TimerPtr timer)
{
	timer->addDailyTask([](){
		time_t now = time(NULL);
		struct tm nowTm;
		gmtime_r(&now, &nowTm);

		cout<<"Func(daily) Task run! Order: 07:20:20, curr "<<(nowTm.tm_year + 1990)<<"-"<<(nowTm.tm_mon + 1)<<"-"<<(nowTm.tm_mday)<<" "<<(nowTm.tm_hour)<<":"<<(nowTm.tm_min)<<":"<<(nowTm.tm_sec)<<endl;
	},
	[](){
		cout<<"[Cancelled] Func(daily) Task!"<<endl;
	}, 7, 20, 20);
}

void testRemoveTask(TimerPtr timer)
{
	int64_t now = slack_real_msec();

	uint64_t taskId = timer->addPeriodTask([now](){
		int64_t now2 = slack_real_msec();
		std::unique_lock<std::mutex> lck(_mutex);
		cout<<"Func(double) Task (FFF) period forever period 5000 msec! diff "<<(now2 - now)<<endl;
	},
	[](){
		std::unique_lock<std::mutex> lck(_mutex);
		cout<<"[Cancelled] Func(double) Task (FFF) period forever period 5000 msec!"<<endl;
	}, 5000);

	timer->addTask([now, timer, taskId](){
		int64_t now2 = slack_real_msec();
		timer->removeTask(taskId);
		std::unique_lock<std::mutex> lck(_mutex);
		cout<<"Remove Task executed! diff "<<(now2 - now)<<endl;
	}, 60000);
}

int main(int argc, const char* argv[])
{
	ITaskThreadPoolPtr tp(new TaskThreadPool());
	tp->init(4, 3, 20, 60);

	TimerPtr timer = Timer::create(tp);
	if (timer == nullptr)
	{
		cout<<"init timer failed."<<endl;
		return 0;
	}

	if (argc != 2)
		goto error;

	if (strcmp(argv[1], "0") == 0)
	{
		testTask(timer);
		sleep(60);
	}
	else if (strcmp(argv[1], "1") == 0)
	{
		testFuncTask(timer);
		sleep(60);
	}
	else if (strcmp(argv[1], "2") == 0)
	{
		testParamFuncTask(timer);
		sleep(60);
	}
	else if (strcmp(argv[1], "3") == 0)
	{
		testDoubleFuncTask(timer);
		sleep(60);
	}
	else if (strcmp(argv[1], "4") == 0)
	{
		testDiffRepatedPeriodTask(timer);
		sleep(120);
	}
	else if (strcmp(argv[1], "5") == 0)
	{
		testDiffRepatedPeriodRightNowTask(timer);
		sleep(120);
	}
	else if (strcmp(argv[1], "6") == 0)
	{
		cout<<"entery daily mode"<<endl;
		testDailyTask(timer);
		while (true)
			sleep(60);
	}
	else if (strcmp(argv[1], "7") == 0)
	{
		testRemoveTask(timer);
		sleep(90);
	}
	else
		goto error;

	return 0;

	error:
	cout<<"Usage: "<<argv[0]<<" testID"<<endl;
	cout<<"  testID: 0:normal, 1:func, 2:boolFunc, 3:doubleFunc,"<<endl;
	cout<<"          4:diff(repeated task & period task), 5:diff(repeated task & period task right now version),"<<endl;
	cout<<"          6:daily, 7:remove"<<endl;
	return 0;
}
