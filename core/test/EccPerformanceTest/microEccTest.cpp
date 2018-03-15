// g++ -o microEccTest microEccTest.cpp ../../micro-ecc/uECC.c

#include <stdio.h>
#include <string.h>
#include "../../micro-ecc/uECC.h"

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

int main(int argc, const char* argv[])
{
	uint8_t private1[32];
	uint8_t private2[32];

	uint8_t public1[64];
	uint8_t public2[64];

	uECC_Curve curve = uECC_secp256k1();
	int pubKeySize = uECC_curve_public_key_size(curve);
	int prvKeySize = uECC_curve_private_key_size(curve);

	printf("public key size %d, private key size %d\n", pubKeySize, prvKeySize);

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

	const char *data = "1234567890abcdefghijklmnopqrstuvwxyz";

	memcpy(secret1, data, 32);

	printf("secret1 inited\n");
	printMemory(secret1, 32);

	int r = uECC_shared_secret(public2, private1, secret1, curve);
	if (r == 0)
	{
		printf("uECC_shared_secret 1 failed.\n");
		return 0;
	}

	printf("secret1 shared\n");
	printMemory(secret1, 32);

	memcpy(secret2, secret1, 32);

	printf("secret2 inited\n");
	printMemory(secret2, 32);

	r = uECC_shared_secret(public1, private2, secret2, curve);
	if (r == 0)
	{
		printf("uECC_shared_secret 2 failed.\n");
		return 0;
	}

	printf("secret2 shared\n");
	printMemory(secret2, 32);

	if (memcmp(data, secret2, 32) != 0)
	{
		printf("memcmp failed.\n");
	}
	else
		printf("memcmp OK.\n");


	return 0;
}



