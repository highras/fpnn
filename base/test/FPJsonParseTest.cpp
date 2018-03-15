/*
	Linux:
	g++ --std=c++11 -I. -I../../base/ -lstdc++ -o FPJsonParseTest ../FPJson.cpp ../FPJsonParser.cpp FPJsonParseTest.cpp ../../base/FpnnError.o ../../base/unixfs.o ../../base/StringUtil.o

	Mac:
	g++ --std=c++11 -lstdc++ -o FPJsonParseTest ../FPJson.cpp ../FPJsonParser.cpp FPJsonParseTest.cpp ../../base/StringUtil.cpp ../../base/unixfs.c ../../base/FpnnError.cpp -I.. -I../../base/
*/

#include <iostream>
#include "AutoRelease.h"
#include "unixfs.h"
#include "FPJson.h"

using namespace std;
using namespace fpnn;

void parseTest(const char *filenme, const char *path)
{
	char *buf = NULL;
	size_t size = 0;
	unixfs_get_content(filenme, &buf, &size);

	AutoFreeGuard contentGuard(buf);

	try {
		JsonPtr json = Json::parse(buf);
		cout<<"get json:"<<endl;
		cout<<json<<endl<<endl;
		cout<<"get value by path "<<path<<endl;
		cout<<(*json)[path]<<endl;
	}
	catch (const FpnnError &e)
	{
		cout<<"[Error]: "<<e.message()<<" line: "<<e.line()<<", fun: "<<e.fun()<<", code: "<<e.code()<<endl;
	}
}

int main(int argc, const char** argv)
{
	if (argc < 3)
	{
		std::cout<<"Usage: "<<argv[0]<<" <json_filename> <fetch_path>"<<std::endl;
		return 0;
	}


	parseTest(argv[1], argv[2]);
	return 0;
}
