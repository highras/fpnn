#include <stdio.h>
#include <stdint.h>
#include "PrintMemory.h"

void fpnn::printMemory(const void* memory, size_t size)
{
	//char buf[8];
	const char* data = (char*)memory;
	size_t unit_size = sizeof(void*);
	size_t range = size % unit_size;
	char buf[unit_size];
	
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
