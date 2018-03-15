#include <iostream>
#include "TCPEpollServer.h"
#include "IQuestProcessor.h"
#include "TCPClient.h"
#include "TaskThreadPool.h"
#include "msec.h"
#include "Setting.h"

using namespace std;
using namespace fpnn;


class QuestProcessor: public IQuestProcessor
{

	QuestProcessorClassPrivateFields(QuestProcessor)

public:

	FPAnswerPtr CloneQuest(const FPReaderPtr args, const FPQuestPtr quest, const ConnectionInfo& ci)
	{
		cout<<"socket: "<<ci.socket<<", address: "<<ci.ip<<":"<<ci.port<<endl;
		cout<<"quest data: "<<(quest->json())<<endl;

		std::shared_ptr<TCPClient> client = TCPClient::createClient("127.0.0.1:13133");
		FPQuestPtr q = FPQWriter::CloneQuest("CloneAnswer", quest);
		cout<<"cloned quest data: "<<(q->json())<<endl;
		FPAnswerPtr answer = client->sendQuest(q);
		cout<<"answer data: "<<(answer->json())<<endl;

		FPAnswerPtr an = FPAWriter::CloneAnswer(answer, quest);
		cout<<"cloned answer data: "<<(an->json())<<endl;

		return an;
	}

	QuestProcessor()
	{
		registerMethod("CloneQuest", &QuestProcessor::CloneQuest);

	}

	~QuestProcessor(){
	}

	QuestProcessorClassBasicPublicFuncs
};

int main(int argc, char* argv[])
{
	if (argc != 2)
	{
		cout<<"Usage: "<<argv[0]<<" config"<<endl;
		return 0;
	}
	if(!Setting::load(argv[1])){
		cout<<"Config file error:"<< argv[1]<<endl;
		return 1;
	}

	ServerPtr server = TCPEpollServer::create();
	server->setQuestProcessor(std::make_shared<QuestProcessor>());
	server->startup();
	server->run();

	return 0;
}
