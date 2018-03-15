#include <iostream>
#include "TCPEpollServer.h"
#include "IQuestProcessor.h"
#include "TaskThreadPool.h"
#include "msec.h"
#include "Setting.h"

using namespace std;
using namespace fpnn;


class QuestProcessor: public IQuestProcessor
{

	QuestProcessorClassPrivateFields(QuestProcessor)

public:

	FPAnswerPtr CloneAnswer(const FPReaderPtr args, const FPQuestPtr quest, const ConnectionInfo& ci)
	{
		cout<<"socket: "<<ci.socket<<", address: "<<ci.ip<<":"<<ci.port<<endl;
		cout<<"quest data: "<<(quest->json())<<endl;

		//return
		FPAWriter aw(6, quest);
		aw.param("answer", "one");
		aw.param("int", 2);
		aw.param("double", 3.3);
		aw.param("boolean", true);
		aw.paramArray("ARRAY",2);
		aw.param(make_tuple ("tuple1", 3.1, 14, false));
		aw.param(make_tuple ("tuple2", 5.7, 9, true));
		aw.paramMap("MAP",5);
		aw.param("m1","first_map");
		aw.param("m2",true);
		aw.param("m3",5);
		aw.param("m4",5.7);
		aw.param("m5","中文2");	 
		return aw.take();
	}

	QuestProcessor()
	{
		registerMethod("CloneAnswer", &QuestProcessor::CloneAnswer);

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
