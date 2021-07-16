#include <iostream>
#include "FPZKClient.h"
#include "TCPEpollServer.h"
#include "IQuestProcessor.h"
#include "TaskThreadPool.h"
#include "msec.h"
#include "Setting.h"
#include "ServerInfo.h"

using namespace std;
using namespace fpnn;

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

int64_t statCount = 100 * 10000;
std::atomic<int64_t> processedQuest(0);

class QuestProcessor: public IQuestProcessor
{

	QuestProcessorClassPrivateFields(QuestProcessor)

	FPZKClientPtr _fpzk;
	TaskThreadPool _delayAnswerPool;

public:
	virtual void connected(const ConnectionInfo& info) { cout<<"server connected.socket ID:"<<info.socket<<endl; }
	virtual void connectionWillClose(const ConnectionInfo& ci, bool closeByError)
	{
		if (!closeByError)
			cout<<"client close event processed. ci: "<<ci.str()<<", self: "<<this<<endl;
		else
			cout<<"client error event processed. ci: "<<ci.str()<<", self: "<<this<<endl;
	}
	virtual FPAnswerPtr unknownMethod(const std::string& method_name, const FPReaderPtr args, const FPQuestPtr quest, const ConnectionInfo& ci)
	{
		cout<<"received a unknown method"<<endl;
		return IQuestProcessor::unknownMethod(method_name, args, quest, ci);
	}

	//-- oneway
	FPAnswerPtr demoOneway(const FPReaderPtr args, const FPQuestPtr quest, const ConnectionInfo& ci)
	{
		/*cout<<"One way method demo is called. socket: "<<ci.socket<<", address: "<<ci.ip<<":"<<ci.port<<endl;
		cout<<"quest data: "<<(quest->json())<<endl;*/

		static int64_t iCount = 0;
		iCount++;
		static int64_t start = exact_real_usec();
		if(iCount % statCount == 0){
			int64_t end = exact_real_usec();
			std::cout<<"Recv "<<statCount<<", Speed:"<<statCount*1000*1000/(end-start)<<std::endl;
			start = end;
		}
		
		return NULL;
	}
	//-- towway
	FPAnswerPtr demoTowway(const FPReaderPtr args, const FPQuestPtr quest, const ConnectionInfo& ci)
	{
		static int64_t iCount = 0;
		iCount++;
		static int64_t start = exact_real_usec();
		if(iCount % statCount == 0){
			int64_t end = exact_real_usec();
			std::cout<<"Recv "<<statCount<<", Speed:"<<statCount*1000*1000/(end-start)<<std::endl;
			start = end;
		}

		processedQuest++;
		return FPAWriter(2, quest)("Simple","one")("Simple2", 2);
	}

	FPAnswerPtr duplexDemo(const FPReaderPtr args, const FPQuestPtr quest, const ConnectionInfo& ci)
	{
		//cout<<"Duplex demo is called. "<<endl;
		//cout<<"quest data: "<<(quest->json())<<endl;

		FPQuestPtr quest2 = QWriter("duplex quest", false, FPMessage::FP_PACK_MSGPACK);
		FPAnswerPtr answer = sendQuest(quest2);

		//cout<<"recv client answer: "<<(answer->json())<<endl;

		processedQuest++;
		return FPAWriter(2, quest)("Simple","one")("Simple2", 2);
	}

	FPAnswerPtr delayDuplexDemo(const FPReaderPtr args, const FPQuestPtr quest, const ConnectionInfo& ci)
	{
		int delay = 2;

		cout<<"Delay duplex demo is called. "<<endl;
		cout<<"quest data: "<<(quest->json())<<endl;

		QuestSenderPtr sender = genQuestSender(ci);

		_delayAnswerPool.wakeUp([delay, sender](){
				sleep(delay);

				FPQuestPtr quest2 = QWriter("duplex quest", false, FPMessage::FP_PACK_MSGPACK);
				FPAnswerPtr answer = sender->sendQuest(quest2);
				cout<<"delay duplex server recv client answer: "<<(answer->json())<<endl;
				});
		;
		return FPAWriter(2, quest)("Simple","one")("Simple2", 2);
	}

	QuestProcessor()
	{
		registerMethod("one way demo", &QuestProcessor::demoOneway);
		registerMethod("two way demo", &QuestProcessor::demoTowway);
		registerMethod("duplex demo", &QuestProcessor::duplexDemo);
		registerMethod("delay duplex demo", &QuestProcessor::delayDuplexDemo);

		_delayAnswerPool.init(1, 1, 1, 1);

		std::string fpzkSrvs = Setting::getString("FPNN.fpzk.servers");
		std::string project = Setting::getString("FPNN.fpzk.project");
                std::string prjToken = Setting::getString("FPNN.fpzk.project_token");
                std::string service = Setting::getString("FPNN.fpzk.service_name");
                std::string version = Setting::getString("FPNN.fpzk.version");
                std::string cluster = Setting::getString("FPNN.fpzk.cluster");
                std::string endpoint = Setting::getString("FPNN.fpzk.endpoint");

                _fpzk = FPZKClient::create(fpzkSrvs, project, prjToken);
                _fpzk->registerService(service, cluster, version, endpoint);
	}

	QuestProcessorClassBasicPublicFuncs
};

bool running = true;
void status_loop(ServerPtr server)
{
	int64_t pq = processedQuest;

	while (running)
	{
		int64_t start = exact_real_usec();

		sleep(3);
		//continue;

		int64_t pq2 = processedQuest;
		int64_t ent = exact_real_usec();

		int64_t dpq = pq2 - pq;
		pq = pq2;

		int64_t real_time = ent - start;
		dpq = dpq * 1000 * 1000 / real_time;

		cout<<"[QPS] : "<<dpq<<endl;


		cout<<"[io poll] : "<<GlobalIOPool::nakedInstance()->ioPoolStatus()<<endl;

		cout<<"[work poll] : "<<server->workerPoolStatus()<<endl;

		cout<<"[duplex poll] : "<<server->answerCallbackPoolStatus()<<endl;

		cout<<"--------------------"<<endl;
	}
}
int main(int argc, char* argv[])
{
	try{
		if (argc != 2)
		{
			cout<<"Usage: "<<argv[0]<<" config"<<endl;
			return 0;
		}
		if(!Setting::load(argv[1])){
			cout<<"Config file error:"<< argv[1]<<endl;
			return 1;
		}

		{
			std::string key = "StatCount";
			if(Setting::setted(key))
				statCount = Setting::getInt(key);
		}

		ServerPtr server = TCPEpollServer::create();
		server->setQuestProcessor(std::make_shared<QuestProcessor>());
		server->startup();
		std::thread(&status_loop, server).detach();
		server->run();

		running = false;
		sleep(1);
	}
	catch (const FpnnError& ex){
		cout<<"Exception:("<<ex.code()<<")"<<ex.what()<<endl;
	}   
	catch (...)
	{   
		cout<<"Unknown error"<<endl;
	}   

	return 0;
}
