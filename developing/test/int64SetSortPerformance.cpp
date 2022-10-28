#include <iostream>
#include <string>
#include <list>
#include <set>
#include <unordered_set>
#include <vector>
#include "msec.h"

using std::cout;
using std::endl;
//--  g++ --std=c++11 -O2 -o aaa aaa.cpp msec.c -lpthread
std::list<int64_t> genData(int count = 100 * 10000, int undulateFator = 1000, int64_t factor = 12345678)
{
	std::vector<std::list<int64_t>> cache;
	for (int i = 0; i < undulateFator; i++)
		cache.push_back(std::list<int64_t>());

	int subIdx = 0;
	for (int i = 0; i < count; i++)
	{
		int64_t v;
		if (i & 0x1)
			v = (slack_real_msec() << (i % 16)) + factor++;
		else    
			v = (slack_real_msec() >> (i % 16)) + factor++;

		cache[subIdx].push_back(v);

		subIdx += 1;
		if (subIdx == undulateFator)
			subIdx = 0;
	}

	std::list<int64_t> data;
	for (int i = 0; i < undulateFator; i++)
		data.insert(data.end(), cache[i].begin(), cache[i].end());

	return data;
}

void test(int count = 100 * 10000, int undulateFator = 1000, int64_t factor = 12345678)
{
	int64_t begin = slack_mono_msec();
	std::list<int64_t> data = genData(count, undulateFator, factor);
	int64_t prepareData = slack_mono_msec() - begin;

	cout<<"Prepare "<<count<<" int64_t data cost "<<prepareData<<" msec"<<endl;

	{
		std::set<int64_t> set64;
		begin = slack_mono_msec();
		for (int64_t i: data)
			set64.insert(i);
		int64_t setCost = slack_mono_msec() - begin;

		std::vector<int64_t> orderedData;
		orderedData.resize(set64.size());
		for (int64_t i : set64)
			orderedData.push_back(i);

		int64_t orderedCost = slack_mono_msec() - begin;

		cout<<"Insert "<<count<<" int64_t data to std::set<int64_t> cost "<<setCost<<" msec, unique data is "<<set64.size()<<endl;
		cout<<"Total sort cost "<<orderedCost<<" msec"<<endl;
	}

	{
		std::unordered_set<int64_t> uset64;
		begin = slack_mono_msec();
		for (int64_t i: data)
			uset64.insert(i);
		int64_t usetCost = slack_mono_msec() - begin;

		cout<<"Insert "<<count<<" int64_t data to std::unordered_set<int64_t> cost "<<usetCost<<" msec, unique data is "<<uset64.size()<<endl;
	}


	int arrCount = 2050;
	int64_t range = INT64_MAX / 1024;
	std::vector<std::set<int64_t>> arrSet;
	for (int i = 0; i < arrCount; i++)
		arrSet.push_back(std::set<int64_t>());

	begin = slack_mono_msec();
	for (int64_t i: data)
	{
		int64_t idx = i / range;
		arrSet[idx+1024].insert(i);
	}
	int64_t arrSetCost = slack_mono_msec() - begin;


	std::vector<int64_t> orderedData2;
	orderedData2.resize(count);
	for (auto& setInst: arrSet)
		for (int64_t i : setInst)
			orderedData2.push_back(i);

	int64_t orderedCost2 = slack_mono_msec() - begin;

	cout<<"Insert "<<count<<" int64_t data to arrSet cost "<<arrSetCost<<" msec"<<endl;
	cout<<"Total sort cost "<<orderedCost2<<" msec"<<endl;
}

int main(int argc, const char** argv)
{
	if (argc == 1)
		test();
	else
		test(atoi(argv[1]) * 10000);

	return 0;
}