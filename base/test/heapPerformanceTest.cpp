#include <vector>
#include <deque>
#include <queue>
#include <iostream>
#include <algorithm>
#include <sys/time.h>
#include <stdint.h>
#include <stdlib.h>

using namespace std;

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

class ITest
{
public:
	virtual int64_t pop() = 0;
	virtual void push(int64_t) = 0;
	virtual const char* name() = 0;
	virtual bool remain() = 0;
};

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
		if (a & 0x11 == b & 0x11)
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

class VectorTest: public ITest
{
	std::vector<int64_t> _vector;

public:
	virtual int64_t pop()
	{
		int64_t v = 0;
		if (_vector.size())
		{
			if (_vector.size() > 1)
				std::pop_heap(_vector.begin(), _vector.end());

			v = _vector.back();
			_vector.pop_back();
		}
		return v;
	}
	virtual void push(int64_t v)
	{
		_vector.push_back(v);
		if (_vector.size() > 1)
			std::push_heap(_vector.begin(), _vector.end());
	}
	virtual const char* name() { return "vector"; }
	virtual bool remain() { return _vector.size(); }
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
			if (_queue.size() > 1)
				std::pop_heap(_queue.begin(), _queue.end());
			
			v = _queue.back();
			_queue.pop_back();
		}
		return v;
	}
	virtual void push(int64_t v)
	{
		_queue.push_back(v);
		if (_queue.size() > 1)
			std::push_heap(_queue.begin(), _queue.end());
	}
	
	virtual const char* name() { return "deque"; }
	virtual bool remain() { return _queue.size(); }
};

void orderTest()
{
	VectorTest vt;
	vt.push(545);
	vt.push(4543);
	vt.push(434);
	vt.push(3423);
	vt.push(232);

	vt.push(435);
	vt.push(7657);
	vt.push(3242);
	vt.push(234);
	vt.push(7657);

	vt.push(98765);
	vt.push(43534);
	vt.push(3434);
	vt.push(7686);
	vt.push(345);
	vt.push(32);

	vt.push(435);
	vt.push(7657);
	vt.push(3242);
	vt.push(234);
	vt.push(7657);

	vt.push(4546);
	vt.push(865);
	vt.push(2343);
	vt.push(656);
	vt.push(34);

	while (vt.remain())
		cout<<vt.pop()<<endl;
}

int main(int argc, const char* argv[])
{
	VectorTest vt;
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

	test_performance(count, vt);
	test_performance(count, dt);
	test_performance(count, pt);
	test_performance(count, pte);

	orderTest();

	return 0;
}
