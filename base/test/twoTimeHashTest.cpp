#include <sys/time.h>
#include <iostream>
#include <set>
#include <vector>
#include "jenkins.h"
#include "crc.h"
#include "strhash.h"
#include "FileSystemUtil.h"
#include "msec.h"
#include "uuid.h"

using namespace std;
using namespace fpnn;

int64_t from = 1000000000;
int64_t to = 1200000000;

void testTwoTimeHashFunc()
{
	set<uint64_t> uids;
	uint64_t start = slack_real_msec();
	for(int64_t i = from; i < to; ++i){
		string id = to_string(i);
		int64_t one = jenkins_hash(id.c_str(), id.length(), 0);
		int64_t two = crc32_checksum(id.c_str(), id.length());
		uint64_t id64 = ((one << 32) | two);
	}
	uint64_t end = slack_real_msec();
	cout<<"Two time Hash crc time: " << end - start << endl;

	start = slack_real_msec();
	for(int64_t i = from; i < to; ++i){
		string id = to_string(i);
		int64_t one = jenkins_hash(id.c_str(), id.length(), 0);
		int64_t two = crc32_checksum(id.c_str(), id.length());
		uint64_t id64 = ((one << 32) | two);
		uids.insert(id64);
	}
	end = slack_real_msec();
	cout<<"Two time Hash crc uniq id, all: "<< (to-from) << "  same: "<< (to-from) - uids.size() << endl;
}

void testTwoTimeHashFunc2()
{
	set<uint64_t> uids;
	uint64_t start = slack_real_msec();
	for(int64_t i = from; i < to; ++i){
		string id = to_string(i);
		int64_t one = jenkins_hash(id.c_str(), id.length(), 0);
		int64_t two = strhash(id.c_str(), 0);
		uint64_t id64 = ((one << 32) | two);
	}
	uint64_t end = slack_real_msec();
	cout<<"Two time Hash strHash time: " << end - start << endl;

	start = slack_real_msec();
	for(int64_t i = from; i < to; ++i){
		string id = to_string(i);
		int64_t one = jenkins_hash(id.c_str(), id.length(), 0);
		int64_t two = strhash(id.c_str(), 0);
		uint64_t id64 = ((one << 32) | two);
		uids.insert(id64);
	}
	end = slack_real_msec();
	cout<<"Two time Hash strHash uniq id, all: "<< (to-from) << "  same: "<< (to-from) - uids.size() << endl;
}

void testJenkinsHashFunc()
{
	set<uint64_t> uids;
	uint64_t start = slack_real_msec();
	for(int64_t i = from; i < to; ++i){
		string id = to_string(i);
		int64_t id64 = jenkins_hash64(id.c_str(), id.length(), 0);
	}
	uint64_t end = slack_real_msec();
	cout<<"jenkins Hash time: " << end - start << endl;;

	start = slack_real_msec();
	for(int64_t i = from; i < to; ++i){
		string id = to_string(i);
		int64_t id64 = jenkins_hash64(id.c_str(), id.length(), 0);
		uids.insert(id64);
	}
	end = slack_real_msec();
	cout<<"jenkins Hash uniq id, all: "<< (to-from) << "  same: "<< (to-from) - uids.size() << endl;
}

int main(int argc, const char *argv[])
{
	if (argc != 2)
	{
		cout<<"Usage: " << argv[0] << " file " <<endl;
		return 0;
	}

	testJenkinsHashFunc();
	testTwoTimeHashFunc();
	testTwoTimeHashFunc2();

	return 0;
}
