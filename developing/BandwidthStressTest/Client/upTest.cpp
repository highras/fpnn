#include <iostream>
#include "CommandLineUtil.h"
#include "TCPClient.h"
#include "UDPClient.h"
#include "../Transfer/Transfer.cpp"

using namespace std;
using namespace fpnn;

int main(int argc, const char** argv)
{
	CommandLineParser::init(argc, argv);
	std::vector<std::string> mainParams = CommandLineParser::getRestParams();
	
	if (mainParams.size() != 3)
	{
		cout<<"Usgae: "<<argv[0]<<" <endpoint> <server-path> <local-path> [[-kbs trans-block-KB] | [-bs trans-block-bytes]] [-c concurrent-transfer-count] [-r max-retry-send-count] [-t timeout] [-pem public-key] [-udp]"<<endl;
		return 1;
	}

	ClientPtr client;
	if (CommandLineParser::exist("udp"))
	{
		client = UDPClient::createClient(mainParams[0]);
		if (CommandLineParser::exist("pem"))
		{
			std::string pemFile = CommandLineParser::getString("pem");
			if (((UDPClient*)(client.get()))->enableEncryptorByPemFile(pemFile.c_str()) == false)
			{
				cout<<"Invalid PEM file: "<<pemFile<<endl;
				return 1;
			}
		}
	}
	else
	{
		client = TCPClient::createClient(mainParams[0]);
		if (CommandLineParser::exist("pem"))
		{
			std::string pemFile = CommandLineParser::getString("pem");
			if (((TCPClient*)(client.get()))->enableEncryptorByPemFile(pemFile.c_str(), false, true) == false)
			{
				cout<<"Invalid PEM file: "<<pemFile<<endl;
				return 1;
			}
		}
	}

	int blockSize = 200 * 1024;
	if (CommandLineParser::exist("kbs"))
		blockSize = CommandLineParser::getInt("kbs") * 1024;

	if (CommandLineParser::exist("bs"))
		blockSize = CommandLineParser::getInt("bs");

	int concurrent = 10;
	if (CommandLineParser::exist("c"))
		concurrent = CommandLineParser::getInt("c");

	int maxRetrySendCount = 5;
	if (CommandLineParser::exist("r"))
		maxRetrySendCount = CommandLineParser::getInt("r");

	int timeout = 0;
	if (CommandLineParser::exist("t"))
		timeout = CommandLineParser::getInt("t");

	SenderPtr sender = Sender::create(client, mainParams[2], mainParams[1], blockSize);
	sender->setMaxRetryCount(maxRetrySendCount);
	sender->setConcurrentCount(concurrent);
	sender->setTimeout(timeout);
	sender->launch();

	while (sender->done() == false)
	{
		cout<<"["<<sender->currDone()<<"/"<<sender->totalCount()<<"]"<<endl;
		sleep(1);
	}
	cout<<"Finish:["<<sender->currDone()<<"/"<<sender->totalCount()<<"]"<<endl;

	return 0;
}