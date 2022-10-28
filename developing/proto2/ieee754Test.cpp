#include <stdio.h>
#include <stdint.h>
#include <iostream>
#include <set>

// https://zh.wikipedia.org/wiki/IEEE_754
// https://www.cnblogs.com/HDK2016/p/10506083.html
// check:
// https://www.binaryconvert.com/convert_double.html
// https://www.h-schmidt.net/FloatConverter/IEEE754.html

/*

  float 1325400064 (1.3254e+09)
  org:           00 00 00 4f                 ...O    
  binary:        ff ff 7f 3f                 ...?    
  decode:        ff ff 7f 3f                 ...?   


  float 2147483648 (2.14748e+09)
  org:           00 00 00 80                 ....    
  binary:        00 00 00 00                 ....    
  decode:        00 00 00 00                 ....   

  float 3472883712 (3.47288e+09)
  org:           00 00 00 cf                 ....    
  binary:        ff ff 7f bf                 ....    
  decode:        ff ff 7f bf                 ....   

*/

void printMemory(const void* memory, size_t size)
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

uint64_t superBig = 0;
uint64_t largeThanOne = 0;
uint64_t lessThanOne = 0;

#include "FPIEEE754.cpp"

void test(const char* floatString, float value)
{
	uint32_t bin = fpnn::toIeee754(value);
	float rfloat = fpnn::fromIeee754(bin);

	std::cout<<"String: "<<floatString<<std::endl;
	std::cout<<"Input:  "<<value<<std::endl;
	std::cout<<"rfloat: "<<rfloat<<std::endl;
	std::cout<<"org:   "; printMemory(&value, sizeof(value));
	std::cout<<"binary:"; printMemory(&bin, sizeof(bin));
	std::cout<<"decode:"; printMemory(&rfloat, sizeof(rfloat));
	std::cout<<std::endl;
}

void testD(const char* doubleString, double value)
{
	uint64_t bin = fpnn::toIeee754(value);
	double rdouble = fpnn::fromIeee754(bin);

	std::cout<<"String:  "<<doubleString<<std::endl;
	std::cout<<"Input:   "<<value<<std::endl;
	std::cout<<"rdouble: "<<rdouble<<std::endl;
	std::cout<<"org:   "; printMemory(&value, sizeof(value));
	std::cout<<"binary:"; printMemory(&bin, sizeof(bin));
	std::cout<<"decode:"; printMemory(&rdouble, sizeof(rdouble));
	std::cout<<std::endl;
}

bool testDCmp(const char* doubleString, double value)
{
	uint64_t bin = fpnn::toIeee754(value);
	double rdouble = fpnn::fromIeee754(bin);

	if (memcmp(&bin, &value, 8) == 0 && memcmp(&rdouble, &value, 8) == 0)
		return true;

	std::cout<<"String:  "<<doubleString<<std::endl;
	std::cout<<"Input:   "<<value<<std::endl;
	std::cout<<"rdouble: "<<rdouble<<std::endl;
	std::cout<<"org:   "; printMemory(&value, sizeof(value));
	std::cout<<"binary:"; printMemory(&bin, sizeof(bin));
	std::cout<<"decode:"; printMemory(&rdouble, sizeof(rdouble));
	std::cout<<std::endl;

	return false;
}

bool testDCmpPlus(uint64_t base, uint64_t radius)
{
	std::cout<<"----------[base "<<base<<" ]----------------"<<std::endl;

	double value;
	bool rev = true;
	for (uint64_t sss = base - radius; sss < base + radius; sss++)
	{
		memcpy(&value, &sss, 8);
		if (!testDCmp(std::to_string(sss).c_str(), value))
			rev = false;
	}
	return rev;
}

void testAll()
{

	float ff, revf;
	uint32_t fbin;

	uint32_t fcycle = 0;
	uint32_t cycleUnit = 100 * 10000;

	std::cout<<"==========[ float + ]============="<<std::endl;

	superBig = 0;
	largeThanOne = 0;
	lessThanOne = 0;

	for (uint32_t i = 0; i < 0x7F7FFFFF; i++)
	{
		memcpy(&ff, &i, 4);
		fbin = fpnn::toIeee754(ff);
		revf = fpnn::fromIeee754(fbin);

		if (memcmp(&fbin, &i, 4))
		{
			std::cout<<"float "<<i<<" ("<<(float)i<<")"<<std::endl;
			std::cout<<"  org:   "; printMemory(&i, 4);
			std::cout<<"  binary:"; printMemory(&fbin, 4);
			std::cout<<"  decode:"; printMemory(&revf, 4);
		}
		if (memcmp(&revf, &i, 4))
		{
			std::cout<<"float "<<i<<" ("<<(float)i<<")"<<std::endl;
			std::cout<<"  org:   "; printMemory(&i, 4);
			std::cout<<"  binary:"; printMemory(&fbin, 4);
			std::cout<<"  decode:"; printMemory(&revf, 4);
		}

		fcycle += 1;
		if (fcycle % cycleUnit == 0)
			std::cout<<"-------------- float +: "<<(fcycle/cycleUnit)<<"M - "<<fcycle<<", superBig: "<<superBig<<", largeThanOne: "<<largeThanOne<<", lessThanOne: "<<lessThanOne<<" -----------------------"<<std::endl;
	}

	fcycle = 0;
	std::cout<<"==========[ float - ]============="<<std::endl;

	superBig = 0;
	largeThanOne = 0;
	lessThanOne = 0;

	for (uint32_t i = 0x80000000; i < 0xFF7FFFFF; i++)
	{
		memcpy(&ff, &i, 4);
		fbin = fpnn::toIeee754(ff);
		revf = fpnn::fromIeee754(fbin);

		if (memcmp(&fbin, &i, 4))
		{
			std::cout<<"float "<<i<<" ("<<(float)i<<")"<<std::endl;
			std::cout<<"  org:   "; printMemory(&i, 4);
			std::cout<<"  binary:"; printMemory(&fbin, 4);
			std::cout<<"  decode:"; printMemory(&revf, 4);
		}
		if (memcmp(&revf, &i, 4))
		{
			std::cout<<"float "<<i<<" ("<<(float)i<<")"<<std::endl;
			std::cout<<"  org:   "; printMemory(&i, 4);
			std::cout<<"  binary:"; printMemory(&fbin, 4);
			std::cout<<"  decode:"; printMemory(&revf, 4);
		}

		fcycle += 1;
		if (fcycle % cycleUnit == 0)
			std::cout<<"-------------- float -: "<<(fcycle/cycleUnit)<<"M - "<<fcycle<<", superBig: "<<superBig<<", largeThanOne: "<<largeThanOne<<", lessThanOne: "<<lessThanOne<<" -----------------------"<<std::endl;
	}

	double dd, revd;
	uint64_t dbin;

	uint64_t dcycle = 0;

	std::cout<<"==========[ Double + ]============="<<std::endl;

	superBig = 0;
	largeThanOne = 0;
	lessThanOne = 0;

	for (uint64_t i = 0; i < 0x7FEFFFFFFFFFFFFF; i++)
	{
		memcpy(&dd, &i, 8);
		dbin = fpnn::toIeee754(dd);
		revd = fpnn::fromIeee754(dbin);

		if (memcmp(&dbin, &i, 8))
		{
			std::cout<<"double "<<i<<" ("<<(double)i<<")"<<std::endl;
			std::cout<<"  org:   "; printMemory(&i, 8);
			std::cout<<"  binary:"; printMemory(&dbin, 8);
			std::cout<<"  decode:"; printMemory(&revd, 8);
		}
		if (memcmp(&revd, &i, 8))
		{
			std::cout<<"double "<<i<<" ("<<(double)i<<")"<<std::endl;
			std::cout<<"  org:   "; printMemory(&i, 8);
			std::cout<<"  binary:"; printMemory(&dbin, 8);
			std::cout<<"  decode:"; printMemory(&revd, 8);
		}

		dcycle += 1;
		if (dcycle % cycleUnit == 0)
			std::cout<<"-------------- double +: "<<(dcycle/cycleUnit)<<"M - "<<dcycle<<", superBig: "<<superBig<<", largeThanOne: "<<largeThanOne<<", lessThanOne: "<<lessThanOne<<" -----------------------"<<std::endl;
	}

	dcycle = 0;
	std::cout<<"==========[ Double - ]============="<<std::endl;
	
	superBig = 0;
	largeThanOne = 0;
	lessThanOne = 0;

	for (uint64_t i = 0x8000000000000000; i < 0xFFEFFFFFFFFFFFFF; i++)
	{
		memcpy(&dd, &i, 8);
		dbin = fpnn::toIeee754(dd);
		revd = fpnn::fromIeee754(dbin);

		if (memcmp(&dbin, &i, 8))
		{
			std::cout<<"double "<<i<<" ("<<(double)i<<")"<<std::endl;
			std::cout<<"  org:   "; printMemory(&i, 8);
			std::cout<<"  binary:"; printMemory(&dbin, 8);
			std::cout<<"  decode:"; printMemory(&revd, 8);
		}
		if (memcmp(&revd, &i, 8))
		{
			std::cout<<"double "<<i<<" ("<<(double)i<<")"<<std::endl;
			std::cout<<"  org:   "; printMemory(&i, 8);
			std::cout<<"  binary:"; printMemory(&dbin, 8);
			std::cout<<"  decode:"; printMemory(&revd, 8);
		}

		dcycle += 1;
		if (dcycle % cycleUnit == 0)
			std::cout<<"-------------- double -: "<<(dcycle/cycleUnit)<<"M - "<<dcycle<<", superBig: "<<superBig<<", largeThanOne: "<<largeThanOne<<", lessThanOne: "<<lessThanOne<<" -----------------------"<<std::endl;
	}
}

//-- https://www.javaroad.cn/questions/291872
void unsigned_int_2charArray(char *outputArray, unsigned int X) {    //long int is the same size as regular int
    int o = 0;
    int x;
    char byteNum[4];
    byteNum[0] = (X & 0x000000FF);
    byteNum[1] = (X & 0x0000FF00) >> 8;
    byteNum[2] = (X & 0x00FF0000) >> 16;
    byteNum[3] = (X & 0xFF000000) >> 24;
    //if (!isBigEndian)  
       for (x = 0; x <= 3; x++)    {outputArray[o]=byteNum[x]; o++;}  // datagram.append(byteNum[x]);
    //else if (isBigEndian) for (x = 3; x >= 0; x--)    {outputArray[o]=byteNum[x]; o++;}  // datagram.append(byteNum[x]);
}

void float_2charArray(char *outputArray, float testVariable1) {    //long int is the same size as regular intz
    int o = 0;
    int x;
    char byteNum[8];
    unsigned int    sign = 0;
    float           mantessa = 0;
    int             exp = 0;
    unsigned int    theResult;
    if (testVariable1 ==0){theResult = 0;}
    else{   if (testVariable1 < 0) {
                sign =  0x80000000;
                testVariable1 = testVariable1 * -1.0;
                }

            while (1){
 
                mantessa = testVariable1 / powf(2,exp);
                if      (mantessa >= 1 && mantessa < 2) {break;}
                else if (mantessa >= 2.0)               {exp = exp + 1;}
                else if (mantessa < 1   )               {exp = exp - 1;}
                }
            unsigned int fixedExponent =   ((exp+127)<<23);
            unsigned int fixedMantessa = (float) (mantessa -1) * pow((float)2,(float)23);
            theResult = sign + fixedExponent + fixedMantessa;
            }
    unsigned_int_2charArray(byteNum, theResult); 
    //if (!isBigEndian)    
     for (x = 0; x <= 7; x++)    {outputArray[o]=byteNum[x]; o++;}  // datagram.append(byteNum[x]);
    //else if (isBigEndian) for (x = 7; x >= 0; x--)    {outputArray[o]=byteNum[x]; o++;}  // datagram.append(byteNum[x]);
}

#include <sys/time.h>
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

void testProfprmance()
{
	struct timeval start, end;

	float ff, revf;
	uint32_t fbin;

	uint32_t fcycle = 0;
	uint32_t cycleUnit = 100 * 10000;

	superBig = 0;
	largeThanOne = 0;
	lessThanOne = 0;

	gettimeofday(&start, NULL);

	//for (uint32_t i = 0x3B9ACA00; i < 0x7F7FFFFF; i++)	//--  2139095039 - 1000000000
	for (uint32_t i = 1566000000; i < 2066000000; i++)
	{
		memcpy(&ff, &i, 4);
		fbin = fpnn::toIeee754(ff);
		revf = fpnn::fromIeee754(fbin);

		if (memcmp(&ff, &i, 4))
		{
			std::cout<<"float "<<i<<" ("<<(float)i<<")"<<std::endl;
			std::cout<<"  org:   "; printMemory(&i, 4);
			std::cout<<"  binary:"; printMemory(&fbin, 4);
			std::cout<<"  decode:"; printMemory(&revf, 4);
		}
		if (memcmp(&revf, &i, 4))
		{
			std::cout<<"float "<<i<<" ("<<(float)i<<")"<<std::endl;
			std::cout<<"  org:   "; printMemory(&i, 4);
			std::cout<<"  binary:"; printMemory(&fbin, 4);
			std::cout<<"  decode:"; printMemory(&revf, 4);
		}

		//fcycle += 1;
		//if (fcycle % cycleUnit == 0)
		//	std::cout<<"-------------- float +: "<<(fcycle/cycleUnit)<<"M - "<<fcycle<<", superBig: "<<superBig<<", largeThanOne: "<<largeThanOne<<", lessThanOne: "<<lessThanOne<<" -----------------------"<<std::endl;
	}

	gettimeofday(&end, NULL);

	struct timeval cost = diff_timeval(start, end);
	std::cout<<"test self: times cost "<<cost.tv_sec<<" sec "<<(cost.tv_usec/1000)<<" msec "<<std::endl;
}

int main()
{
	int testcase = 0;

	if (testcase == 0)
	{
		testAll();
	}
	else if (testcase == 1)
	{
		testProfprmance();
	}
	else if (testcase == 2)
	{
		std::cout<<"==========[ float ]============="<<std::endl;

		test("1256.3443", 1256.3443f);
		test("0.00023456", 0.00023456f);
		test("134217727.89", 134217727.89f);
		test("0", 0);
		test("NAN", NAN);
		test("-NAN", -NAN);
		test("INFINITY", INFINITY);
		test("-INFINITY", -INFINITY);
		test("1.4013e-45", 1.4013e-45f);
		
		{
			float dd = 0;
			uint32_t ss = 0x1;
			memcpy(&dd, &ss, 4);
			test("1.4013e-45", dd);

			ss = 0x2;
			memcpy(&dd, &ss, 4);
			test("2.8026e-45", dd);
		}

		std::cout<<"==========[ double ]============="<<std::endl;

		testD("1256.3443454545", 1256.3443454545);
		testD("0.00000023456", 0.00000023456);
		testD("134217723327.89232323434", 134217723327.89232323434);
		testD("0", 0);
		testD("NAN", NAN);
		testD("-NAN", -NAN);
		testD("INFINITY", INFINITY);
		testD("-INFINITY", -INFINITY);
		testD("4.94066e-324", 4.94066e-324);
		
		{
			double dd = 0;
			uint64_t ss = 0x1;
			memcpy(&dd, &ss, 8);
			testD("4.94066e-324", dd);

			ss = 0x2;
			memcpy(&dd, &ss, 8);
			testD("9.88131e-324", dd);
		}
	}
	else
	{
		float dd = 0;
		uint32_t ss = 0x400000;
		memcpy(&dd, &ss, 4);
		test("0x400000", dd);

		dd = 0;
		ss = 0x600000;
		memcpy(&dd, &ss, 4);
		test("0x600000", dd);

		dd = 0;
		ss = 0x1;
		memcpy(&dd, &ss, 4);
		test("0x1", dd);

		std::cout<<"~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~"<<std::endl;

		dd = 0;
		ss = 1266680667;
		memcpy(&dd, &ss, 4);
		test("1266680667", dd);

		std::cout<<"----------------------------------"<<std::endl;

		dd = 0;
		ss = 1325400064;
		memcpy(&dd, &ss, 4);
		test("1325400064", dd);

		float_2charArray((char*)&ss, dd); printMemory(&ss, 4);
		std::cout<<std::endl<<std::endl;

		dd = 0;
		ss = 3472883712;
		memcpy(&dd, &ss, 4);
		test("3472883712", dd);

		float_2charArray((char*)&ss, dd); printMemory(&ss, 4);
		std::cout<<std::endl<<std::endl;

		{
			uint64_t ddd;
			double ssss;

			ddd = 4971973988617027583;

			memcpy(&ssss, &ddd, 8);
			testD(std::to_string(ddd).c_str(), ssss);


			ddd = 0x4f00000000000000;

			memcpy(&ssss, &ddd, 8);
			testD(std::to_string(ddd).c_str(), ssss);

			ddd = 0xcf00000000000000;

			memcpy(&ssss, &ddd, 8);
			testD(std::to_string(ddd).c_str(), ssss);


			return 0;
		}

		std::set<uint64_t> exp;
		for (uint64_t i = 0; i <= 0xff; i++)
		{
			uint64_t base = i << 56;
			if (testDCmpPlus(base, 1) == false)
				exp.insert(base);
		}

		std::cout<<std::endl<<std::endl;
		for (auto v: exp)
			std::cout<<" -- "<<std::dec<<v<<" "<<std::hex<<v<<std::endl;

		return 0;

		int count = 10000 * 10000;
		uint64_t vvv = 0;
		float zz = 45.235667;
		struct timeval start, end;

		gettimeofday(&start, NULL);
		for (int i = 0; i < count; i++, zz++)
		{
			float_2charArray((char*)&ss, zz);

			vvv += ss;
		}
		gettimeofday(&end, NULL);

		struct timeval cost = diff_timeval(start, end);
		std::cout<<"test powf: "<<count<<" times cost "<<cost.tv_sec<<" sec "<<(cost.tv_usec/1000)<<" msec "<<vvv<<std::endl;


		zz = 45.235667;

		gettimeofday(&start, NULL);
		for (int i = 0; i < count; i++, zz++)
		{
			vvv += fpnn::toIeee754(zz);
		}
		gettimeofday(&end, NULL);

		cost = diff_timeval(start, end);
		std::cout<<"test self: "<<count<<" times cost "<<cost.tv_sec<<" sec "<<(cost.tv_usec/1000)<<" msec "<<vvv<<std::endl;
	}

	return 0;
}
