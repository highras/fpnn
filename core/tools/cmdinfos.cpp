#include <iostream>
#include <vector>
#include <thread>
#include <assert.h>
#include "TCPClient.h"
#include "IQuestProcessor.h"

using namespace std;
using namespace fpnn;

int main(int argc, char* argv[])
{
	if (argc != 2 && argc != 3)
	{   
		cout<<"Usage: "<<argv[0]<<" [localhost] port"<<endl;
		return 0;
	}   
	string ip = "localhost";
	int port = 0;
	if(argc == 3){
		ip = argv[1];
		port = atoi(argv[2]);
	}
	else{
		port = atoi(argv[1]);
	}
	
	string method = "*infos";
	string body = "{}";
	bool isOnway = false;
	bool isMsgPack = true;

	ClientEngine::setQuestTimeout(30);

	FPQWriter qw(method, body, isOnway, isMsgPack?FPMessage::FP_PACK_MSGPACK:FPMessage::FP_PACK_JSON);
	FPQuestPtr quest = qw.take();

	std::shared_ptr<TCPClient> client = TCPClient::createClient(ip, port);

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

