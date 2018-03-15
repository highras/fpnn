// g++ -std=c++11 -I../../../base -I../.. -L../../../base -L../.. -o X690ReaderTester X690ReaderTester.cpp -lfpnn -lfpbase -lcurl -lpthread

#include <iostream>
#include "FileSystemUtil.h"
#include "StringUtil.h"
#include "PEM_DER_SAX.h"

using namespace std;
using namespace fpnn;

class TestReader: public X690ReaderInterface
{
public:
	virtual enum SAXSignType enterContainer(int tagClass, int tag, bool constructed, long dataLength, int depth)
	{
		cout<<"d="<<depth<<" Enter Container. tag: "<<tag<<", class: "<<tagClass<<", len: "<<dataLength<<endl;
		return SAXContinue;
	}
	virtual enum SAXSignType exitContainer(int tagClass, int tag, bool constructed, long dataLength, int depth)
	{
		cout<<"d="<<depth<<" Exit Container. tag: "<<tag<<", class: "<<tagClass<<", len: "<<dataLength<<endl;
		return SAXContinue;
	}
	virtual enum SAXSignType skip(int tagClass, int tag, bool constructed, long dataLength, int depth)
	{
		cout<<"d="<<depth<<" skip tag: "<<tag<<", class: "<<tagClass<<", len: "<<dataLength<<endl;
		return SAXContinue;
	}
	virtual enum SAXSignType objectIdentifier(int depth, const std::vector<uint64_t>& value)
	{
		std::string oid = StringUtil::join(value, ",");
		cout<<"d="<<depth<<" Object ID: "<<oid<<endl;
		return SAXContinue;
	}
	virtual enum SAXSignType BITString(int depth, int paddedBits, const std::string& value)
	{
		cout<<"d="<<depth<<" BIT String, padded: "<<paddedBits<<", len: "<<value.length()<<endl;
		return SAXContinue;
	}
	virtual void documentError(const char* reason)
	{
		cout<<"Error: "<<reason<<endl;
	}
};

int main(int argc, const char **argv)
{
	if (argc != 2)
	{
		cout<<"Usage: "<<argv[0]<<" <asn1-pem-file>"<<endl;
		return 0;
	}

	std::string content;
	if (FileSystemUtil::readFileContent(argv[1], content) == false)
	{
		cout<<"read file "<<argv[1]<<" failed."<<endl;
		return 0;
	}

	TestReader reader;

	PemSAX pemSAX;
	pemSAX.parse(content, &reader);

	return 0;
}
