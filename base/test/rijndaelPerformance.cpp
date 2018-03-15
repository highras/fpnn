// g++ --std=c++11 -O2 -o rijndaelPerformance_O2 rijndaelPerformance.cpp ../rijndael.c -I..

#include <string.h>
#include <stdlib.h>
#include <iostream>
#include <sys/time.h>
#include "rijndael.h"

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

void test(const char *key, int key_len, int count, const char *data, int data_len, bool enc)
{
	const char* stdIV = "1234567890123456";

	uint8_t sample = 0xff;
	char *buf = (char *)malloc(data_len);


	struct timeval start, end;
	gettimeofday(&start, NULL);
	
	for (int i = 0; i < count; i++)
	{
		uint8_t iv[16];
		memcpy(iv, stdIV, 16);

		rijndael_context enCtx;
		rijndael_setup_encrypt(&enCtx, (const uint8_t *)key, key_len);

		size_t pos = 0;
		rijndael_cfb_encrypt(&enCtx, enc, (const uint8_t *)data, (uint8_t *)buf, data_len, iv, &pos);

		sample &= buf[data_len - 2];
	}
	gettimeofday(&end, NULL);

	free(buf);

	struct timeval cost = diff_timeval(start, end);
	cout<<"test "<<(enc?"enc":"dec")<<"("<<sample<<"): count "<<count<<" times cost "<<cost.tv_sec<<" sec "<<(cost.tv_usec/1000)<<" msec"<<endl;
}

int main()
{
	const char* key32 = "qazwsxedcrfvtgbyhnujmikolp123456";
	const char* key16 = "qwertyuiopasdfgh";

	const char* data = "ees dcfdsf sdxcewaf34cesdgcscferxf4wxfcwer23e 3 43c4ed ecsce gce gec esc gef cd fd grfd g5fedt4ec grefc gcefd gefdgcef fc ec fc fcd fcd fcd gfdv gcr rc gcrv c rfcd res rd erfcd fcd cd fv";
	int data_len = (int)strlen(data);

	int count = 10 * 10000;

	cout<<"======= key 16 =========="<<endl;
	test(key16, 16, count, data, data_len, true);
	test(key16, 16, count, data, data_len, false);

	cout<<"======= key 32 =========="<<endl;
	test(key32, 32, count, data, data_len, true);
	test(key32, 32, count, data, data_len, false);

	cout<<"======= key 16 =========="<<endl;
	test(key16, 16, count * 10, data, data_len, true);
	test(key16, 16, count * 10, data, data_len, false);

	cout<<"======= key 32 =========="<<endl;
	test(key32, 32, count * 10, data, data_len, true);
	test(key32, 32, count * 10, data, data_len, false);

	return 0;
}