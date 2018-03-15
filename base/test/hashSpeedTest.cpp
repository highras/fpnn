#include <sys/time.h>
#include <iostream>
#include "jenkins.h"
#include "strhash.h"
#include "TimeAnalyst.h"

using namespace std;

struct timeval start;
struct timeval finish;

void printResult(const char* name, int times, unsigned int value)
{
	struct timeval result = diff_timeval(start, finish);
	cout<< "[" << name << "][times: " << times << "][cost: " << result.tv_sec << " seconds, " << result.tv_usec << " usec]";
	cout<< "[value "<< value <<"]"<<endl;
}

void testStrHashFunc(int times, const std::string& str)
{
	unsigned int value = 0;

	gettimeofday(&start, NULL);
	for (int i = 0; i < times; i++)
	{
		value += strhash(str.c_str(), 0);
	}
	gettimeofday(&finish, NULL);
	printResult("strhash", times, value);
}

void testjenkinsHashFunc(int times, const std::string& str)
{
	unsigned int value = 0;

	gettimeofday(&start, NULL);
	for (int i = 0; i < times; i++)
	{
		value += jenkins_hash(str.c_str(), str.length(), 0);
	}
	gettimeofday(&finish, NULL);
	printResult("jenkins_hash", times, value);
}

int main(int argc, const char *argv[])
{
	int times = 10000 * 100;
	std::string data = "0123456789abcdefghijklmnopqrstuvwxyz!@#$%^&*()_+{}|:<>?/.,;'[]=-";

	if (argc > 1)
		times = 10000 * atoi(argv[1]);

	if (argc > 2)
		data = argv[2];

	if (argc > 3)
	{
		cout<<"Usage: " << argv[0] << " <times> <str_data>" <<endl;
		return 0;
	}

	testStrHashFunc(times, data);
	testjenkinsHashFunc(times, data);

	return 0;
}