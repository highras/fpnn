// g++ -o microEccPerformanceTest microEccPerformanceTest.cpp ../../micro-ecc/uECC.c

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <iostream>
#include "../../micro-ecc/uECC.h"

uint8_t ass[32];

void timeCalc(struct timeval start, struct timeval finish)
{
	int sec = finish.tv_sec - start.tv_sec;
	int usec;
	
	if (finish.tv_usec >= start.tv_usec)
		usec = finish.tv_usec - start.tv_usec;
	else
	{
		usec = 100 * 10000 + finish.tv_usec - start.tv_usec;
		sec -= 1;
	}
	
	std::cout<<"time cost "<< (sec * 1000 + usec / 1000) << "."<<(usec % 1000)<<" ms"<<std::endl;
}

void printMemory(const void* memory, size_t size)
{
	char buf[8];
	const char* data = (char*)memory;
	size_t unit_size = sizeof(void*);
	size_t range = size % unit_size;
	
	if (range)
		range = size - range + unit_size;
	else
		range = size;

	for (size_t index = 0, column = 0; index < range; index++, column++)
	{
		if (column == unit_size)
		{
			printf("    %.*s\n", (int)unit_size, buf);
			column = 0;
		}

		if (column == 0)
			printf("        ");

		char c = ' ';
		if (index < size)
		{
			printf("%02x ", (uint8_t)data[index]);
			
			c = data[index];
			if (c < '!' || c > '~')
				c = '.';
		}
		else
			printf("   ");
		buf[column] = c;
	}

	if (size)
		printf("    %.*s\n", (int)unit_size, buf);
}

int test()
{
	uint8_t private1[32];
	uint8_t private2[32];

	uint8_t public1[64];
	uint8_t public2[64];

	uECC_Curve curve = uECC_secp256k1();
	int pubKeySize = uECC_curve_public_key_size(curve);
	int prvKeySize = uECC_curve_private_key_size(curve);

	if (!uECC_make_key(public1, private1, curve))
	{
		printf("uECC_make_key() 1 failed\n");
		return 0;
	}

	if (!uECC_make_key(public2, private2, curve))
	{
		printf("uECC_make_key() 2 failed\n");
		return 0;
	}

	uint8_t secret1[32];
	uint8_t secret2[32];

	int r = uECC_shared_secret(public2, private1, secret1, curve);
	if (r == 0)
	{
		printf("uECC_shared_secret 1 failed.\n");
		return 0;
	}

	r = uECC_shared_secret(public1, private2, secret2, curve);
	if (r == 0)
	{
		printf("uECC_shared_secret 2 failed.\n");
		return 0;
	}

	if (memcmp(secret1, secret2, 32) != 0)
	{
		printf("memcmp failed.\n");
	}
	else
		for (int i = 0; i < 32; i++)
			ass[i] &= secret1[i];

	return 0;
}

int main(int argc, const char* argv[])
{
	for (int i = 0; i < 32; i++)
		ass[i] = 0;

	int count = 10000;
	if (argc > 1)
		count = atoi(argv[1]);

	std::cout<<"times: "<<count<<std::endl;
	
	struct timeval start, finish;
	gettimeofday(&start, NULL);

	for (int i = 0; i < count; i++)
		test();

	gettimeofday(&finish, NULL);

	printMemory(ass, 32);
	timeCalc(start, finish);

	return 0;
}
