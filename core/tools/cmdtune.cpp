#include <iostream>
#include <vector>
#include <thread>
#include <assert.h>
#include "TCPClient.h"
#include "UDPClient.h"
#include "CommandLineUtil.h"
#include "IQuestProcessor.h"

using namespace std;
using namespace fpnn;

int main(int argc, char* argv[])
{
	CommandLineParser::init(argc, argv);
	std::vector<std::string> mainParams = CommandLineParser::getRestParams();
	
	if (mainParams.size() != 3 && mainParams.size() != 4)
	{   
		cout<<"Usage: "<<argv[0]<<" [localhost] port key value [-udp]"<<endl;
		return 0;
	}   
	string ip = "localhost";
	int port = 0;
	string key;
	string value;
	if(mainParams.size() == 3){
		port = std::stoi(mainParams[0]);
		key = mainParams[1];
		value = mainParams[2];
	}
	else{
		ip = mainParams[0];
		port = std::stoi(mainParams[1]);
		key = mainParams[2];
		value = mainParams[3];
	}
	string method = "*tune";
	string body = "{\"key\":\""+key+"\",\"value\":\""+value+"\"}";
	bool isOnway = false;
	bool isMsgPack = true;

	ClientEngine::setQuestTimeout(30);

	FPQWriter qw(method, body, isOnway, isMsgPack?FPMessage::FP_PACK_MSGPACK:FPMessage::FP_PACK_JSON);
	FPQuestPtr quest = qw.take();

	ClientPtr client;
	if (!CommandLineParser::exist("udp"))
		client = TCPClient::createClient(ip, port);
	else
		client = UDPClient::createClient(ip, port);

	try{
		FPAnswerPtr answer = client->sendQuest(quest);
		if (quest->isTwoWay()){
			assert(quest->seqNum() == answer->seqNum());
			cout<<"Return:"<<answer->json()<<endl;
		}
	}
	catch (...)
	{
		cout<<"error occurred when sending"<<endl;
	}

	client->close();
	return 0;
}

