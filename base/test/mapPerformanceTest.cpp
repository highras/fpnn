//-- g++ --std=c++11 -o mapPerformanceTest mapPerformanceTest.cpp

#include <map>
#include <unordered_map>
#include <iostream>
#include <string>
#include <sys/time.h>
#include <stdint.h>

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

const int stringArrayCount = 6;
const std::string stringArray[] = {"aaa", "bbb", "ccc", "ddd", "111", "222"};

void intMapTest(int members, int times)
{
	std::map<int, int> ins;

	for (int i = 0; i < members; i++)
		ins[i] = i;

	struct timeval start, end;
	int64_t v = 0;
	gettimeofday(&start, NULL);
	for (int i = 0, k = 0; k < times; k++, i++)
	{
		if (i >= members)
			i = 0;

		v += ins[i];
	}
	gettimeofday(&end, NULL);

	struct timeval cost = diff_timeval(start, end);
	cout<<"test int map (member = "<<members<<", times = "<<times<<", v = "<<v<<") times cost "<<cost.tv_sec<<" sec "<<(cost.tv_usec/1000)<<" msec"<<endl;
}

void stringMapTest(int members, int times)
{
	int count = stringArrayCount;

	std::map<string, int> ins;

	for (int i = 0; i < count; i++)
		ins[stringArray[i]] = i;
	for (int i = count; i < members; i++)
		ins[std::to_string(i)] = i;

	struct timeval start, end;
	int64_t v = 0;
	gettimeofday(&start, NULL);
	for (int i = 0, k = 0; k < times; k++, i++)
	{
		if (i >= count)
			i = 0;

		v += ins[stringArray[i]];
	}
	gettimeofday(&end, NULL);

	struct timeval cost = diff_timeval(start, end);
	cout<<"test string map (member = "<<members<<", times = "<<times<<", v = "<<v<<") times cost "<<cost.tv_sec<<" sec "<<(cost.tv_usec/1000)<<" msec"<<endl;
}

void stringUnorderedMapTest(int members, int times)
{
	int count = stringArrayCount;

	std::unordered_map<string, int> ins;

	for (int i = 0; i < count; i++)
		ins[stringArray[i]] = i;
	for (int i = count; i < members; i++)
		ins[std::to_string(i)] = i;

	struct timeval start, end;
	int64_t v = 0;
	gettimeofday(&start, NULL);
	for (int i = 0, k = 0; k < times; k++, i++)
	{
		if (i >= count)
			i = 0;

		v += ins[stringArray[i]];
	}
	gettimeofday(&end, NULL);

	struct timeval cost = diff_timeval(start, end);
	cout<<"test string unordered_map (member = "<<members<<", times = "<<times<<", v = "<<v<<") times cost "<<cost.tv_sec<<" sec "<<(cost.tv_usec/1000)<<" msec"<<endl;
}

void intUnorderedMapTest(int members, int times)
{
	std::unordered_map<int, int> ins;

	for (int i = 0; i < members; i++)
		ins[i] = i;

	struct timeval start, end;
	int64_t v = 0;
	gettimeofday(&start, NULL);
	for (int i = 0, k = 0; k < times; k++, i++)
	{
		if (i >= members)
			i = 0;

		v += ins[i];
	}
	gettimeofday(&end, NULL);

	struct timeval cost = diff_timeval(start, end);
	cout<<"test int unordered_map (member = "<<members<<", times = "<<times<<", v = "<<v<<") times cost "<<cost.tv_sec<<" sec "<<(cost.tv_usec/1000)<<" msec"<<endl;
}

void basicTest(int members, int times)
{
	intMapTest(members, times);
	stringMapTest(members, times);
	intUnorderedMapTest(members, times);
	stringUnorderedMapTest(members, times);
}

int main()
{
	int times = 1000000;
	
	basicTest(100, times);
	basicTest(300, times);
	basicTest(500, times);
	basicTest(1000, times);
	basicTest(2000, times);
	basicTest(5000, times);
	basicTest(10000, times);

	return 0;
}
