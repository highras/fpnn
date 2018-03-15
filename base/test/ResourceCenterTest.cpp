#include <iostream>
#include <atomic>
#include <unistd.h>
#include "ResourceCenter.h"
#include "TaskThreadPool.h"

using namespace std;
using namespace fpnn;

std::atomic<int> counter(0);
class AA
{
	int _id;
	int _order;
	std::string _name;
public:
	AA(const std::string& name, int order): _order(order), _name(name) { _id = ++counter; }
	~AA() { cout<<"-- AA idx: "<<_id<<", name: "<<_name<<", order: "<<_order<<" distroy."<<endl; }
};

class BB
{
	int _id;
	int _order;
	std::string _name;
public:
	BB(const std::string& name, int order): _order(order), _name(name) { _id = ++counter; }
	~BB() { cout<<"-- BB idx: "<<_id<<", name: "<<_name<<", order: "<<_order<<" distroy."<<endl; }
};

class ARes: public IResource
{
	AA a;

public:
	ARes(const std::string& name, int order): a(name, order) {}
};
typedef std::shared_ptr<ARes> AResPtr;

class BRes: public IResource
{
	BB b;

public:
	BRes(const std::string& name, int order): b(name, order) {}
};
typedef std::shared_ptr<BRes> BResPtr;

void normalTest()
{
	{
		ResourceCenterPtr inst = ResourceCenter::instance();
		AResPtr a(new ARes("aaa", 0)), b(new ARes("aab", 0)), c(new ARes("aac", 0));

		inst->add("aaa", a);
		inst->add("aab", b);
		inst->add("aac", c);
	}

	{
		ResourceCenterPtr inst = ResourceCenter::instance();
		inst->erase("aac");
	}

	{
		ResourceCenterPtr inst = ResourceCenter::instance();
		BResPtr a(new BRes("baa", 90)), b(new BRes("bab", -3)), c(new BRes("bac", 1));

		inst->add("baa", a, 90);
		inst->add("bab", b, -3);
		inst->add("bac", c, 1);
	}
}

class Task: public ITaskThreadPool::ITask
{
	int _idx;

public:
	virtual void run()
	{
		//cout<<"-- task "<<_idx<<" ready."<<endl;
		ResourceCenter::Guard guard;
		cout<<"-- task "<<_idx<<" begin."<<endl;
		sleep(2);
		cout<<"-- task "<<_idx<<" done."<<endl;
	}

	Task(int idx): _idx(idx) {}
	virtual ~Task() { }
};

void parallelTest()
{
	TaskThreadPool tp;
	tp.init(4, 2, 8, 16);

	for (int i = 0; i < 30; i++)
		tp.wakeUp(std::make_shared<Task>(i));
}

int main()
{
	normalTest();
	parallelTest();

	return 0;
}