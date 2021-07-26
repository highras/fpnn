#include <list>
#include <queue>
#include <deque>
#include <vector>
#include <iostream>
#include <sys/time.h>
#include <stdint.h>
#include <stdlib.h>

using namespace std;

class ITest
{
public:
	virtual int64_t pop() = 0;
	virtual void push(int64_t) = 0;
	virtual const char* name() = 0;
	virtual bool remain() = 0;
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

int64_t test_func(ITest& test, int64_t count)
{
	int push_count = 0;
	int64_t rev = 0;

	for (int64_t i = 0; i < count; ++i, push_count++)
	{
		if (push_count == 5)
		{
			rev -= test.pop();
			rev += test.pop();
			rev += test.pop();
			push_count = 0;
		}

		test.push(i);
	}

	while (test.remain())
		rev -= test.pop();

	return rev;
}

void test_performance(int64_t count, ITest& test)
{
	struct timeval start, end;
	gettimeofday(&start, NULL);
	int64_t rev = test_func(test, count);
	gettimeofday(&end, NULL);

	struct timeval cost = diff_timeval(start, end);
	cout<<"test category: "<<test.name()<<" "<<count<<" times cost "<<cost.tv_sec<<" sec "<<(cost.tv_usec/1000)<<" msec"<<endl;
}

class QueueTest: public ITest
{
	std::queue<int64_t> _queue;

public:
	virtual int64_t pop()
	{
		int64_t v = 0;
		if (_queue.size())
		{
			v = _queue.front();
			_queue.pop();
		}
		return v;
	}
	virtual void push(int64_t v) { _queue.push(v); }
	virtual const char* name() { return "queue"; }
	virtual bool remain() { return _queue.size(); }
};

class ListTest: public ITest
{
	std::list<int64_t> _queue;

public:
	virtual int64_t pop()
	{
		int64_t v = 0;
		if (_queue.size())
		{
			v = _queue.front();
			_queue.pop_front();
		}
		return v;
	}
	virtual void push(int64_t v) { _queue.push_back(v); }
	virtual const char* name() { return "list"; }
	virtual bool remain() { return _queue.size(); }
};

class DequeTest: public ITest
{
	std::deque<int64_t> _queue;

public:
	virtual int64_t pop()
	{
		int64_t v = 0;
		if (_queue.size())
		{
			v = _queue.front();
			_queue.pop_front();
		}
		return v;
	}
	virtual void push(int64_t v) { _queue.push_back(v); }
	virtual const char* name() { return "deque"; }
	virtual bool remain() { return _queue.size(); }
};

class PriorQueueTest: public ITest
{
	std::priority_queue<int64_t> _queue;

public:
	virtual int64_t pop()
	{
		int64_t v = 0;
		if (_queue.size())
		{
			v = _queue.top();
			_queue.pop();
		}
		return v;
	}
	virtual void push(int64_t v) { _queue.push(v); }
	virtual const char* name() { return "priority queue"; }
	virtual bool remain() { return _queue.size(); }
};

class Comp
{
	bool reverse;
public:
	Comp(const bool& revparam=false) {reverse=revparam;}

	bool operator() (const int& a, const int&b) const
	{
		if ((a & 0x11) == (b & 0x11))
		return false;

		return a < b;
	}
};

class PriorQueueTestEx: public ITest
{
	std::priority_queue<int64_t, std::vector<int64_t>, Comp> _queue;

public:
	virtual int64_t pop()
	{
		int64_t v = 0;
		if (_queue.size())
		{
			v = _queue.top();
			_queue.pop();
		}
		return v;
	}
	virtual void push(int64_t v) { _queue.push(v); }
	virtual const char* name() { return "priority queue extend"; }
	virtual bool remain() { return _queue.size(); }
};

int main(int argc, const char* argv[])
{
	QueueTest qt;
	ListTest lt;
	DequeTest dt;
	PriorQueueTest pt;
	PriorQueueTestEx pte;

	int64_t count = 100 * 10000;
	if (argc > 1)
	{
		count = atoll(argv[1]);
		if (count < 100)
			count = 100;
	}

	test_performance(count, qt);
	test_performance(count, lt);
	test_performance(count, dt);
	test_performance(count, pt);
	test_performance(count, pte);

	return 0;
}