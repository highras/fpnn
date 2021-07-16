#include <iostream>
#include <assert.h>
#include "TCPClient.h"
#include "UDPClient.h"
#include "CommandLineUtil.h"

using std::cout;
using std::endl;
using namespace fpnn;

const char* methodNames[] = {"one way demo", "tow way demo", "unkonwn method", "advance answer demo", "delay answer demo"};

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

	if (mainParams.size() != 2)
	{
		cout<<"Usage: "<<argv[0]<<" ip port [-udp]"<<endl;
		return 0;
	}

	std::shared_ptr<Client> client;
	if (CommandLineParser::exist("udp"))
		client = Client::createUDPClient(mainParams[0], atoi(mainParams[1].c_str()));
	else
		client = Client::createTCPClient(mainParams[0], atoi(mainParams[1].c_str()));

	int64_t start = slack_real_msec();
	int64_t sendQuest = 0;
	start = slack_real_msec();

	for (int i = 1; i; i++)
	{
		FPQuestPtr quest = QWriter("one way demo", true, FPMessage::FP_PACK_MSGPACK);
		try{
			client->sendQuest(quest);
			sendQuest++;

			if(sendQuest % 100000 == 0){
				int64_t end = slack_real_msec();
				cout<<"Speed:"<<(100000/(end-start))*1000<<" sent quest: "<<sendQuest<<endl;
				start = end;
			}
		}
		catch (...)
		{
			cout<<"error occurred when sending"<<endl;
		}
	}
	client->close();
	return 0;
}
