#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS	1
#endif

#include "rdtsc.h"
#include <sys/time.h>
#include <unistd.h>
#include <poll.h>
#include <pthread.h>
#include <limits.h>
#include <stdio.h>

#define TELLING_USEC	(1000*100)
#define FREQ_MIN	UINT64_C(1000*1000*100)
#define FREQ_MAX	UINT64_C(1000*1000*1000*10)


uint64_t xs_cpu_frequency;


uint64_t rdtsc()
{
        union {
                struct { uint32_t low, high; } l;
                uint64_t ll;
        } t;

        __asm__ __volatile__ ("rdtsc"
                : "=a" (t.l.low), "=d" (t.l.high));

        return t.ll;
}

static void _get_tsc_us(uint64_t *tsc, uint64_t *us)
{
	struct timespec ts0, ts1;
	uint64_t t0, t1, t2, delta0, delta1;

	t0 = rdtsc();
	clock_gettime(CLOCK_MONOTONIC, &ts0);
	t1 = rdtsc();
	clock_gettime(CLOCK_MONOTONIC, &ts1);
	t2 = rdtsc();

	delta0 = (t1 - t0);
	delta1 = (t2 - t1);
	if (delta0 < delta1)
	{
		*tsc = t0;
		*us = ts0.tv_sec * 1000000 + ts0.tv_nsec/1000;
	}
	else
	{
		*tsc = t1;
		*us = ts1.tv_sec * 1000000 + ts1.tv_nsec/1000;
	}
}

static uint64_t _calc_freq(uint64_t tsc, uint64_t us)
{
	uint64_t freq;
	if (us < 10)
		us = 10;
	freq = (uint64_t)(tsc * ((double)1000000.0) / us);
	if (us < TELLING_USEC)
	{
		// NB: Adjust abnormal frequency.
		//     What is abnormal?
		if (freq < FREQ_MIN)
			freq = FREQ_MIN;
		else if (freq > FREQ_MAX)
			freq = FREQ_MAX;
	}
	return freq;
}

uint64_t get_cpu_frequency(unsigned int milliseconds)
{
	static pthread_mutex_t _mutex = PTHREAD_MUTEX_INITIALIZER;
	static uint64_t _last_tsc, _last_usec;

	uint64_t start_tsc, cur_tsc, used_tsc;
	uint64_t start_usec, cur_usec, used_usec;
	uint64_t freq;

	if (milliseconds)
	{
		int timeout = milliseconds < INT_MAX ? milliseconds : INT_MAX;

		_get_tsc_us(&start_tsc, &start_usec);
		do {
			poll(NULL, 0, timeout);
			_get_tsc_us(&cur_tsc, &cur_usec);
			used_usec = cur_usec - start_usec;
			timeout = milliseconds - (used_usec / 1000);
		} while (timeout > 0);

		used_tsc = cur_tsc - start_tsc;
		freq = _calc_freq(used_tsc, used_usec);

		if (!xs_cpu_frequency)
		{
			pthread_mutex_lock(&_mutex);
			if (!xs_cpu_frequency)
				xs_cpu_frequency = freq;
			pthread_mutex_unlock(&_mutex);
		}
	}
	else
	{
		_get_tsc_us(&cur_tsc, &cur_usec);

		pthread_mutex_lock(&_mutex);
		if (!_last_usec)
		{
			_last_tsc = cur_tsc;
			_last_usec = cur_usec;
			pthread_mutex_unlock(&_mutex);

			start_usec = cur_usec;
			do {
				poll(NULL, 0, 1);
				_get_tsc_us(&cur_tsc, &cur_usec);
			} while (cur_usec - start_usec < 1000);

			pthread_mutex_lock(&_mutex);
		}

		used_usec = cur_usec - _last_usec;
		if (used_usec >= TELLING_USEC || !xs_cpu_frequency)
		{
			used_tsc = cur_tsc - _last_tsc;
			freq = _calc_freq(used_tsc, used_usec);
			xs_cpu_frequency = freq;
			_last_tsc = cur_tsc;
			_last_usec = cur_usec;
		}
		else
		{
			freq = xs_cpu_frequency;
		}
		pthread_mutex_unlock(&_mutex);
	}

	return freq;
}

uint64_t cpu_frequency()
{
	return xs_cpu_frequency ? xs_cpu_frequency : get_cpu_frequency(0);
}


#ifdef TEST_RDTSC

#include <stdio.h>

int main()
{
	printf("%llu\n", (unsigned long long)get_cpu_frequency(0));
	printf("%llu\n", (unsigned long long)get_cpu_frequency(1000));
	printf("%llu\n", (unsigned long long)get_cpu_frequency(1000));
	printf("%llu\n", (unsigned long long)get_cpu_frequency(1000));
	printf("%llu\n", (unsigned long long)get_cpu_frequency(0));
	return 0;
}

#endif
