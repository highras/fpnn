#include <iostream>
#include "TCPEpollServer.h"
#include "IQuestProcessor.h"
#include "TaskThreadPool.h"
#include "msec.h"
#include "Setting.h"
#include "NetworkUtility.h"

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

	TaskThreadPool _delayAnswerPool;

public:
	//will be exec after server startup
	virtual void start() { /* Start User Thread here*/ }

	//called before server exit
	virtual void serverWillStop() {}
	
	//call after server exit
	virtual void serverStopped() {}

	//
	virtual void connected(const ConnectionInfo& info) { cout<<"server connected.socket ID:"<<info.socket<<" address:"<<NetworkUtil::getPeerName(info.socket)<<endl; }
	virtual void connectionWillClose(const ConnectionInfo& info, bool closeByError)
	{
		if (!closeByError)
			cout<<"server close event processed.socket ID:"<<info.socket<<" address:"<<NetworkUtil::getPeerName(info.socket)<<endl;
		else
			cout<<"server error event processed.socket ID:"<<info.socket<<" address:"<<NetworkUtil::getPeerName(info.socket)<<endl;
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
	FPAnswerPtr demoTwoway(const FPReaderPtr args, const FPQuestPtr quest, const ConnectionInfo& ci)
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

	FPAnswerPtr httpDemo(const FPReaderPtr args, const FPQuestPtr quest, const ConnectionInfo& ci)
	{
		cout<<"quest data: "<<(quest->json())<<endl;
		cout<<"QUEST:"<<endl;
		quest->printHttpInfo();
		return FPAWriter(2, quest)("HTTP","ok")("TEST", 2);
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
		return NULL;
	}

	FPAnswerPtr customDelayAnswer(const FPReaderPtr args, const FPQuestPtr quest, const ConnectionInfo& ci)
	{
		int delay = args->wantInt("delaySeconds");

		cout<<"Delay answer demo is called. will answer after "<<delay<<" seconds. address: "<<ci.str()<<endl;
		cout<<"quest data: "<<(quest->json())<<endl;

		std::shared_ptr<IAsyncAnswer> async = genAsyncAnswer(quest);
		_delayAnswerPool.wakeUp([delay, async](){
				sleep(delay);

				//std::string* data = new std::string("Delay answer quest answer");
				FPAnswerPtr answer = FPAWriter::emptyAnswer(async->getQuest()); 
				async->sendAnswer(answer);
			});
		return NULL;
	}

	FPAnswerPtr duplexDemo(const FPReaderPtr args, const FPQuestPtr quest, const ConnectionInfo& ci)
	{
		cout<<"Duplex demo is called. "<<endl;
		cout<<"quest data: "<<(quest->json())<<endl;

		std::string duplexMethod = args->getString("duplex method", "duplex quest");
		bool duplexInOneWay = args->getBool("duplex in one way", false);

		FPQuestPtr quest2 = QWriter(duplexMethod.c_str(), duplexInOneWay, FPMessage::FP_PACK_MSGPACK);
		FPAnswerPtr answer = sendQuest(ci, quest2);
		
		if (!duplexInOneWay)	
			cout<<"recv client answer: "<<(answer->json())<<endl;
		

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

	FPAnswerPtr closeAfterSent(const FPReaderPtr args, const FPQuestPtr quest, const ConnectionInfo& ci)
	{
		FPAnswerPtr answer = FPAWriter::emptyAnswer(quest);
		sendAnswer(quest, answer);

		bool duplex = args->getBool("duplex", false);
		if (duplex)
		{
			FPQuestPtr quest2 = QWriter("one way duplex quest", true, FPMessage::FP_PACK_MSGPACK);
			sendQuest(ci, quest2);
		}
		TCPEpollServer::instance()->closeConnectionAfterSent(ci);
		return nullptr;
	}

	QuestProcessor()
	{
		registerMethod("one way demo", &QuestProcessor::demoOneway);
		registerMethod("two way demo", &QuestProcessor::demoTwoway);
		registerMethod("advance answer demo", &QuestProcessor::demoAdvanceAnswer);
		registerMethod("delay answer demo", &QuestProcessor::demoDelayAnswer);
		registerMethod("custom delay", &QuestProcessor::customDelayAnswer);
		registerMethod("httpDemo", &QuestProcessor::httpDemo);
		registerMethod("duplex demo", &QuestProcessor::duplexDemo);
		registerMethod("delay duplex demo", &QuestProcessor::delayDuplexDemo);
		registerMethod("close after sent", &QuestProcessor::closeAfterSent);

		_delayAnswerPool.init(1, 1, 1, 1);
	}

	QuestProcessorClassBasicPublicFuncs
};

bool running = true;
int statusInfoPeriod = 3;
void status_loop(ServerPtr server)
{
	int64_t pq = processedQuest;

	while (running)
	{
		int64_t start = exact_real_usec();

		sleep(statusInfoPeriod);
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
		if (argc < 2 || argc > 3)
		{
			cout<<"Usage: "<<argv[0]<<" config [status-info-print-period]"<<endl;
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

		if (argc == 3)
			statusInfoPeriod = atoi(argv[2]);

		ServerPtr server = TCPEpollServer::create();
		server->setQuestProcessor(std::make_shared<QuestProcessor>());
		server->startup();
		if (statusInfoPeriod > 0)
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
