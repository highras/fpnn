//-- g++ -std=c++11 -O2 -o avgDiff-float avgDiff-float.cpp -I../../../base -L../../../base -lfpbase
#include <iostream>
#include <map>
#include <vector>
#include "FileSystemUtil.h"
#include "StringUtil.h"
#include "CommandLineUtil.h"

using namespace fpnn;
using namespace std;

void filterValue(map<float, int>& dataMap, map<float, int>::iterator it)
{
	if (it->second == 1)
		dataMap.erase(it);
	else
		dataMap[it->first] = it->second - 1;
}

int main(int argc , const char**argv)
{
	CommandLineParser::init(argc, argv);
	std::vector<std::string> mainParams = CommandLineParser::getRestParams();
	
	if (mainParams.size() != 2)
	{
		cout<<"Usage: "<<argv[0]<<" <elem-index> <org-list-file> [-filterMin count] [-filterMax count]"<<endl;
		return 0;
	}

	vector<string> lines;
	FileSystemUtil::fetchFileContentInLines(mainParams[1], lines);

	int idx = std::stoi(mainParams[0]);

	map<float, int> dataMap;
	for (auto& line: lines)
	{
		vector<string> elems;
		StringUtil::split(line, " ", elems);
		float value = std::stof(elems[idx]);
		if (value > 0.)
		{
			if (dataMap.find(value) == dataMap.end())
				dataMap[value] = 1;
			else
				dataMap[value] = dataMap[value] + 1;
		}
	}

	int n = CommandLineParser::getInt("filterMin");
	while (n--)
		filterValue(dataMap, dataMap.begin());

	n = CommandLineParser::getInt("filterMax");
	while (n--)
		filterValue(dataMap, --(dataMap.end()));

	float max = dataMap.rbegin()->first;

	int count = 0;
	double v = 0;
	
	for (auto& pp: dataMap)
	{
		count += pp.second;
		v += pp.first * pp.second;
	}

	double avg = v/count;

	double diff = 0;
	for (auto& pp: dataMap)
	{
		double a = (pp.first - avg) * pp.second;
		if (a < 0)
			diff -= a;
		else
			diff += a;
	}

	double avgDiff = diff / count;
	float exp = (float)diff * 1.0 / count / (float)avg;

	cout<<"max: "<<max<<", avg: "<<avg<<", avg diff: "<<avgDiff<<", exp: "<<exp<<endl;
	
	return 0;
}
