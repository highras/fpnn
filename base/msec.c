#include "msec.h"
#include <stdlib.h>
#include <pthread.h>

static pthread_once_t _slack_once = PTHREAD_ONCE_INIT;
static volatile int64_t _slack_mono_msec;
static volatile int64_t _slack_mono_us;
static volatile int64_t _slack_real_delta;
static volatile int64_t _slack_mono_sec;
static volatile int64_t _slack_real_sec;


static inline int64_t get_mono_us()
{
	struct timespec now;
	clock_gettime(CLOCK_MONOTONIC, &now);
	return (int64_t)now.tv_sec * 1000000 + now.tv_nsec / 1000;
}

static inline int64_t get_real_us()
{
	struct timespec now;
	clock_gettime(CLOCK_REALTIME, &now);
	return (int64_t)now.tv_sec * 1000000 + now.tv_nsec / 1000;
}

static void *msec_thread(void *arg)
{
	int i;
	struct timespec delay = { 0, 0 };

	for (i = 0; 1; ++i)
	{
		int64_t us = get_mono_us();
		_slack_mono_msec = us / 1000;
		_slack_mono_us = us;

		if (i >= 400)
		{
			int64_t rus = get_real_us();
			_slack_real_delta = rus - us;
			_slack_mono_sec = _slack_mono_msec / 1000;
			_slack_real_sec = rus / 1000 / 1000;
			i = 0;
		}

		// Make nanosleep() wakes up at the time of 0.1 ms past every millisecond.
		//delay.tv_nsec = (1000 - (us - 100) % 1000) * 1000;
		//run it every 0.4ms
		delay.tv_nsec = 400 * 1000;
		nanosleep(&delay, NULL);
	}

	return NULL;
}

static void slack_init()
{
	int rc;
	pthread_attr_t attr;
	pthread_t thr;

	int64_t us = get_mono_us();
	_slack_mono_msec = us / 1000;
	_slack_mono_sec = us / 1000 / 1000;
	_slack_mono_us = us;
	int64_t rus =  get_real_us();
	_slack_real_delta = rus - us;
	_slack_real_sec = rus / 1000 / 1000;

	pthread_attr_init(&attr);
	pthread_attr_setstacksize(&attr, 16 * 1024);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	rc = pthread_create(&thr, &attr, msec_thread, NULL);
	pthread_attr_destroy(&attr);

	if (rc != 0)
		abort();
}

int64_t slack_mono_msec()
{
	pthread_once(&_slack_once, slack_init);
	return _slack_mono_msec; 
}

int64_t slack_real_msec()
{
	pthread_once(&_slack_once, slack_init);
	return (_slack_mono_us + _slack_real_delta) / 1000;
}

int64_t slack_mono_sec()
{
	pthread_once(&_slack_once, slack_init);
	return _slack_mono_sec; 
}

int64_t slack_real_sec()
{
	pthread_once(&_slack_once, slack_init);
	return _slack_real_sec;
}

int64_t exact_mono_msec()
{
	struct timespec now;
	clock_gettime(CLOCK_MONOTONIC, &now);
	return (int64_t)now.tv_sec * 1000 + now.tv_nsec / 1000000;
}

int64_t exact_real_msec()
{
	struct timespec now;
	clock_gettime(CLOCK_REALTIME, &now);
	return (int64_t)now.tv_sec * 1000 + now.tv_nsec / 1000000;
}

int64_t exact_mono_usec()
{
	struct timespec now;
	clock_gettime(CLOCK_MONOTONIC, &now);
	return (int64_t)now.tv_sec * 1000000 + now.tv_nsec / 1000;
}

int64_t exact_real_usec()
{
	struct timespec now;
	clock_gettime(CLOCK_REALTIME, &now);
	return (int64_t)now.tv_sec * 1000000 + now.tv_nsec / 1000;
}

int64_t exact_mono_nsec()
{
	struct timespec now;
	clock_gettime(CLOCK_MONOTONIC, &now);
	return (int64_t)now.tv_sec * 1000000000 + now.tv_nsec;
}

int64_t exact_real_nsec()
{
	struct timespec now;
	clock_gettime(CLOCK_REALTIME, &now);
	return (int64_t)now.tv_sec * 1000000000 + now.tv_nsec;
}

#ifdef TEST_MSEC_
#include <sys/time.h>
#include <stdio.h>
int main()
{
	int count = 10000000;
	int i = 0;
	int64_t start = exact_mono_msec();
	for(i = 0; i < count; ++i){
		slack_mono_msec();
	}
	int64_t end = exact_mono_msec();
	printf("slack_mono_msec: run:%d times, cost:%ld\n", count, end-start);
	start = end;

	for(i = 0; i < count; ++i){
		slack_real_msec();
	}
	end = exact_mono_msec();
	printf("slack_real_msec: run:%d times, cost:%ld\n", count, end-start);
	start = end;

	for(i = 0; i < count; ++i){
		exact_mono_msec();
	}
	end = exact_mono_msec();
	printf("exact_mono_msec: run:%d times, cost:%ld\n", count, end-start);
	start = end;

	for(i = 0; i < count; ++i){
		exact_real_msec();
	}
	end = exact_mono_msec();
	printf("exact_real_msec: run:%d times, cost:%ld\n", count, end-start);
	start = end;

	for(i = 0; i < count; ++i){
		time(NULL);
	}
	end = exact_mono_msec();
	printf("time: run:%d times, cost:%ld\n", count, end-start);
	start = end;

	for(i = 0; i < count; ++i){
		struct timeval tstart;
		gettimeofday(&tstart, NULL);
	}
	end = exact_mono_msec();
	printf("gettimeofday: run:%d times, cost:%ld\n", count,end-start);
	start = end;

	return 0;
}
#endif
