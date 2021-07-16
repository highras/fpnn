#include <iostream>
#include "TCPClient.h"
#include "UDPClient.h"
#include "CommandLineUtil.h"
#include "IQuestProcessor.h"

using namespace std;
using namespace fpnn;

class QuestProcessor: public IQuestProcessor
{
	QuestProcessorClassPrivateFields(QuestProcessor)
public:
	virtual void connected(const ConnectionInfo&) { cout<<"client connected."<<endl; }
	virtual void connectionWillClose(const ConnectionInfo&, bool closeByError)
	{
		if (!closeByError)
			cout<<"client close event processed."<<endl;
		else
			cout<<"client error event processed."<<endl;
	}

	QuestProcessorClassBasicPublicFuncs
};

FPQuestPtr QWriter(const char* method, bool oneway, FPMessage::FP_Pack_Type def_ptype){
    FPQWriter qw(6,method, oneway, def_ptype);
    qw.param("quest", "one");
    qw.param("int", 2); 
    qw.param("double", 3.3);
    qw.param("boolean", true);
    qw.paramArray("ARRAY",2);
    qw.param("first_vec");
    qw.param(4);
    qw.paramMap("MAP",5);
    qw.param("map1","first_map");
    qw.param("map2",true);
    qw.param("map3",5);
    qw.param("map4",5.7);
    qw.param("map5","中文");
	return qw.take();
}

int main(int argc, char* argv[])
{
	CommandLineParser::init(argc, argv);
	std::vector<std::string> mainParams = CommandLineParser::getRestParams();

	if (mainParams.size() != 3)
	{
		cout<<"Usage: "<<argv[0]<<" ip port quest_period(seconds) [-udp] [-keepAlive]"<<endl;
		return 0;
	}

	int period = std::stoi(mainParams[2]);
	
	std::shared_ptr<Client> client;
	if (CommandLineParser::exist("udp"))
	{
		UDPClientPtr udpClient = Client::createUDPClient(mainParams[0], atoi(mainParams[1].c_str()));
		
		if (CommandLineParser::exist("keepAlive"))
			udpClient->keepAlive();

		client = udpClient;
	}
	else
	{
		TCPClientPtr tcpClient = Client::createTCPClient(mainParams[0], atoi(mainParams[1].c_str()));

		if (CommandLineParser::exist("keepAlive"))
			tcpClient->keepAlive();

		client = tcpClient;
	}

	client->setQuestProcessor(std::make_shared<QuestProcessor>());

	while (true)
	{
		FPQuestPtr quest = QWriter("two way demo", false, FPMessage::FP_PACK_MSGPACK);
		try
		{
			FPAnswerPtr answer = client->sendQuest(quest);
			cout << "send a quest, answer: " << answer->json() << endl;
		}
		catch (...)
		{
			cout<<"error occurred when sending"<<endl;
			break;
		}

		sleep(period);
	}

	client->close();
	return 0;
}

