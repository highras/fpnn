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

	//-- oneway
	FPAnswerPtr demoOneway(const FPReaderPtr args, const FPQuestPtr quest, const ConnectionInfo& ci)
	{
		cout<<"One way method demo is called. socket: "<<ci.socket<<", address: "<<ci.ip<<":"<<ci.port<<endl;
		cout<<"quest data: "<<(quest->json())<<endl;
		
		//do something
		
		return NULL;
	}
	//-- towway
	FPAnswerPtr demoTowway(const FPReaderPtr args, const FPQuestPtr quest, const ConnectionInfo& ci)
	{
		cout<<"Tow way method demo is called. socket: "<<ci.socket<<", address: "<<ci.ip<<":"<<ci.port<<endl;
		cout<<"quest data: "<<(quest->json())<<endl;
        cout<<"get=>quest:"<<args->get("quest", string(""))<<endl;
        cout<<"get=>int:"<<args->get("int", (int)0)<<endl;
        cout<<"get=>double:"<<args->get("double", (double)0)<<endl;
        cout<<"get=>boolean:"<<args->get("boolean", (bool)0)<<endl;
        tuple<string, int> tup;
        tup = args->get("ARRAY", tup);
        cout<<"get=>array_tuple:"<<std::get<0>(tup)<<"  "<<std::get<1>(tup)<<endl;
		
		OBJECT obj = args->getObject("MAP");
		FPReader fpr(obj);
		cout<<"map1=>"<<fpr.get("map1",string(""))<<endl;
		cout<<"map2=>"<<fpr.get("map2",(bool)false)<<endl;
		cout<<"map3=>"<<fpr.get("map3",(int)0)<<endl;
		cout<<"map4=>"<<fpr.get("map4",(double)0.0)<<endl;
		cout<<"map5=>"<<fpr.get("map5",string(""))<<endl;
		try{
			cout<<"WANT:"<<fpr.want("map4", (double)0.0)<<endl;
			cout<<"WANT:"<<fpr.want("map4", string(""))<<endl;
		}
		catch(...){
			cout<<"EXECPTION: double value, but want string value"<<endl;
		}
		

		//Do some thing
		//

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
		registerMethod("one way demo", &QuestProcessor::demoOneway);
		registerMethod("two way demo", &QuestProcessor::demoTowway);

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
