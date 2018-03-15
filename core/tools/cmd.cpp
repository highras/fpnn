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
	if (argc != 7 && argc != 8)
	{   
		cout<<"Usage: "<<argv[0]<<" ip port method body(json) isTwoWay isMsgPack [timeoutInSecond]"<<endl;
		return 0;
	}   
	string ip = argv[1];
	int port = atoi(argv[2]);
	string method = argv[3];
	string body = argv[4];
	bool isTwoWay = !atoi(argv[5]);
	bool isMsgPack = atoi(argv[6]);

	if (argc == 8)
		ClientEngine::setQuestTimeout(atoi(argv[7]));

	FPQWriter qw(method, body, isTwoWay, isMsgPack?FPMessage::FP_PACK_MSGPACK:FPMessage::FP_PACK_JSON);
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

