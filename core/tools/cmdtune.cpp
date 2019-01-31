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
	if (argc != 4 && argc != 5)
	{   
		cout<<"Usage: "<<argv[0]<<" [localhost] port key value"<<endl;
		return 0;
	}   
	string ip = "localhost";
	int port = 0;
	string key;
	string value;
	if(argc == 4){
		port = atoi(argv[1]);
		key = argv[2];
		value = argv[3];
	}
	else{
		ip = argv[1];
		port = atoi(argv[2]);
		key = argv[3];
		value = argv[4];
	}
	string method = "*tune";
	string body = "{\"key\":\""+key+"\",\"value\":\""+value+"\"}";
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

