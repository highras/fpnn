#include <iostream>
#include <thread>
#include <vector>
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

	TaskThreadPool _delayAnswerPool;

	std::atomic<bool> _running;
	vector<thread> threads;

public:

	//will be exec after server startup
	virtual void start() { 
		/* Start User Thread here*/ 
		threads.push_back(thread(&QuestProcessor::myThread2,this));
	}

	//called before server exit
	virtual void serverWillStop() { _running = false; }
	
	//call after server exit
	virtual void serverStopped() { }

	//
	virtual void connected(const ConnectionInfo&) { cout<<"server connected."<<endl; }
	virtual void connectionWillClose(const ConnectionInfo&, bool closeByError)
	{
		if (!closeByError)
			cout<<"server close event processed."<<endl;
		else
			cout<<"server error event processed."<<endl;
	}
	virtual FPAnswerPtr unknownMethod(const std::string& method_name, const FPReaderPtr args, const FPQuestPtr quest, const ConnectionInfo& ci)
	{
		cout<<"received a unknown method"<<endl;
		return IQuestProcessor::unknownMethod(method_name, args, quest, ci);
	}

	//-- oneway
	FPAnswerPtr demoOneway(const FPReaderPtr args, const FPQuestPtr quest, const ConnectionInfo& ci)
	{
		cout<<"One way method demo is called. socket: "<<ci.socket<<", address: "<<ci.ip<<":"<<ci.port<<endl;
		cout<<"quest data: "<<(quest->json())<<endl;
		
		//do something
		
		return NULL;
	}
	//-- twoway
	FPAnswerPtr demoTwoway(const FPReaderPtr args, const FPQuestPtr quest, const ConnectionInfo& ci)
	{
		cout<<"Two way method demo is called. socket: "<<ci.socket<<", address: "<<ci.ip<<":"<<ci.port<<endl;
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

	FPAnswerPtr demoAdvanceAnswer(const FPReaderPtr args, const FPQuestPtr quest, const ConnectionInfo& ci)
	{
		//std::string* data = new std::string("advance answer quest answer");
		//FPAnswer *answer = new FPAnswer(data, quest.seqNumLE());
		FPAnswerPtr answer = FPAWriter::emptyAnswer(quest);
		bool sent = sendAnswer(quest, answer);
		if (sent)
			cout<<"answer advance quest success. socket: "<<ci.socket<<", address: "<<ci.ip<<":"<<ci.port<<endl;
		else
			cout<<"answer advance quest failed. socket: "<<ci.socket<<", address: "<<ci.ip<<":"<<ci.port<<endl;
		cout<<"quest data: "<<(quest->json())<<endl;

		return NULL;
	}

	FPAnswerPtr demoDelayAnswer(const FPReaderPtr args, const FPQuestPtr quest, const ConnectionInfo& ci)
	{
		int delay = 5;

		cout<<"Delay answer demo is called. will answer after "<<delay<<" seconds. socket: "<<ci.socket<<", address: "<<ci.ip<<":"<<ci.port<<endl;
		cout<<"quest data: "<<(quest->json())<<endl;

		std::shared_ptr<IAsyncAnswer> async = genAsyncAnswer(quest);
		_delayAnswerPool.wakeUp([delay, async](){
				sleep(delay);

				//std::string* data = new std::string("Delay answer quest answer");
				FPAnswerPtr answer = FPAWriter::emptyAnswer(async->getQuest()); 
				async->sendAnswer(answer);
				});
		;		return NULL;
	}

	void myThread1(){
		cout<<"Start myThread1"<<endl;
		while(_running){
			sleep(1);
		}
	}

	void myThread2(){
		cout<<"Start myThread2"<<endl;
		while(_running){
			sleep(1);
		}
	}

	QuestProcessor():_running(true)
	{
		registerMethod("one way demo", &QuestProcessor::demoOneway);
		registerMethod("two way demo", &QuestProcessor::demoTwoway);
		registerMethod("advance answer demo", &QuestProcessor::demoAdvanceAnswer);
		registerMethod("delay answer demo", &QuestProcessor::demoDelayAnswer);

		_delayAnswerPool.init(5, 1, 10, 20);

		threads.push_back(thread(&QuestProcessor::myThread1,this));
	}

	~QuestProcessor(){
		for(unsigned int i=0; i<threads.size();++i){
			threads[i].join();
		}   
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
