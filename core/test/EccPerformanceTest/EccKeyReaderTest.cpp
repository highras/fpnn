// g++ -std=c++11 -I../../../base -I../.. -L../../../base -L../.. -o EccKeyReaderTest EccKeyReaderTest.cpp -lfpnn -lfpbase -lcurl -lpthread

#include <iostream>
#include "FileSystemUtil.h"
#include "PEM_DER_SAX.h"

using namespace std;
using namespace fpnn;

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

	EccKeyReader reader;

	PemSAX pemSAX;
	pemSAX.parse(content, &reader);

	cout<<"Curve: "<<reader.curveName()<<endl;
	cout<<"Key length: "<<reader.rawPublicKey().length()<<endl;

	return 0;
}
