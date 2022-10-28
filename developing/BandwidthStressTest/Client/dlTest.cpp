#include <iostream>
#include "CommandLineUtil.h"
#include "TCPClient.h"
#include "UDPClient.h"
#include "../Transfer/Transfer.cpp"

using namespace std;
using namespace fpnn;

class QuestProcessor: public IQuestProcessor
{
	QuestProcessorClassPrivateFields(QuestProcessor)

	DataReceiver *_receiver;

public:
	FPAnswerPtr data(const FPReaderPtr args, const FPQuestPtr quest, const ConnectionInfo& ci)
	{
		_receiver->processQuest(quest);
		return FPAWriter::emptyAnswer(quest);
	}

	QuestProcessor(DataReceiver *receiver)
	{
		_receiver = receiver;
		registerMethod("data", &QuestProcessor::data);
	}

	QuestProcessorClassBasicPublicFuncs
};

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

	int timeout = 0;
	if (CommandLineParser::exist("t"))
		timeout = CommandLineParser::getInt("t");

	int concurrent = 10;
	if (CommandLineParser::exist("c"))
		concurrent = CommandLineParser::getInt("c");

	int maxRetrySendCount = 5;
	if (CommandLineParser::exist("r"))
		maxRetrySendCount = CommandLineParser::getInt("r");

	DataReceiver receiver(mainParams[2], blockSize);
	client->setQuestProcessor(std::make_shared<QuestProcessor>(&receiver));

	FPQWriter qw(6, "dl");
	qw.param("path", mainParams[1]);
	qw.param("taskId", 123);
	qw.param("blockSize", blockSize);
	qw.param("concurrentCount", concurrent);
	qw.param("maxRetryCount", maxRetrySendCount);
	qw.param("timeout", timeout);

	FPQuestPtr quest = qw.take();
	FPAnswerPtr answer = client->sendQuest(quest, timeout);
	FPAReader ar(answer);
	if (ar.getInt("code"))
	{
		cout<<"[Error] "<<answer->json()<<endl;
		return 2;
	}

	int count = ar.wantInt("count");
	receiver.setTotalCount(count);

	while (receiver.done() == false)
	{
		cout<<"["<<receiver.currDone()<<"/"<<receiver.totalCount()<<"]"<<endl;
		sleep(1);
	}
	cout<<"Finish:["<<receiver.currDone()<<"/"<<receiver.totalCount()<<"]"<<endl;

	return 0;
}
