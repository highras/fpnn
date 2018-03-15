#include <iostream>
#include <vector>
#include <thread>
#include <assert.h>
#include <stdlib.h>
#include "msec.h"
#include "Setting.h"
#include "StringUtil.h"
#include "TCPCarpProxy.hpp"
#include "TCPRotatoryProxy.hpp"
#include "TCPRandomProxy.hpp"
#include "TCPConsistencyProxy.hpp"
#include "IQuestProcessor.h"

using namespace std;
using namespace fpnn;

std::shared_ptr<TCPRotatoryProxy> equProxy;
std::shared_ptr<TCPCarpProxy> carpProxy;
std::shared_ptr<TCPRandomProxy> randomProxy;
std::shared_ptr<TCPConsistencyProxy> consistencyProxy;

class QuestProcessor: public IQuestProcessor
{
	QuestProcessorClassPrivateFields(QuestProcessor)
public:
	virtual void connected(const ConnectionInfo& ci) { cout<<"client connected. ci: "<<ci.str()<<", self: "<<this<<endl; }
	virtual void connectionClose(const ConnectionInfo& ci) { cout<<"client close event processed. ci: "<<ci.str()<<", self: "<<this<<endl; }
	virtual void connectionErrorAndWillBeClosed(const ConnectionInfo& ci) { cout<<"client error event processed. ci: "<<ci.str()<<", self: "<<this<<endl; }

	FPAnswerPtr serverQuest(const FPReaderPtr args, const FPQuestPtr quest, const ConnectionInfo& ci)
	{
		//cout<<"Tow way method demo is called. ci: "<<ci.str()<<", self: "<<this<<endl;
		//cout<<"quest data: "<<(quest->json())<<endl;
		return FPAWriter(2, quest)("Simple","one")("duplex", 2);
	}

	QuestProcessor()
	{
		registerMethod("duplex quest", &QuestProcessor::serverQuest);
	}
	~QuestProcessor()
	{
		cout<<"destroy quest processor "<<this<<endl;
	}

	QuestProcessorClassBasicPublicFuncs
};

class QuestProcessorFactory: public IProxyQuestProcessorFactory
{
public:
	virtual IQuestProcessorPtr generate(const std::string& host, int port) { return std::make_shared<QuestProcessor>(); }
};

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

int sendCount = 0;
int proxyTypeId = 0;
bool syncQuest = true;
bool duplex = false;
bool sharedDuplex = false;

void syncProcess()
{
	int64_t start = exact_real_msec();
	int64_t resp = 0;

	for (int i = 0; i < sendCount; i++)
	{
		FPQuestPtr quest = QWriter(duplex ? "duplex demo" : "two way demo", false, FPMessage::FP_PACK_MSGPACK);
		int64_t sstart = exact_real_usec();
		try{
			FPAnswerPtr answer;
			if (proxyTypeId == 1)
			{
				answer = carpProxy->sendQuest(slack_real_msec(), quest);
				if (carpProxy->empty())
					cout<<"carpProxy is empty"<<endl;
			}
			else if (proxyTypeId == 2)
			{
				answer = equProxy->sendQuest(quest);
				if (equProxy->empty())
					cout<<"equProxy is empty"<<endl;
			}
			else if (proxyTypeId == 3)
			{
				answer = randomProxy->sendQuest(quest);
				if (randomProxy->empty())
					cout<<"randomProxy is empty"<<endl;
			}
			else if (proxyTypeId == 4)
			{
				answer = consistencyProxy->sendQuest(quest);
				if (consistencyProxy->empty())
					cout<<"consistencyProxy is empty"<<endl;
			}

			if (quest->isTwoWay()){
				assert(quest->seqNum() == answer->seqNum());
				int64_t send = exact_real_usec();
				resp += send - sstart;
			}
		}
		catch (const std::exception &ex)
		{
			cout<<"error occurred when sending. "<<ex.what()<<endl;
		}

		if(i && i % 10000 == 0){
			int64_t end = slack_real_msec();
			cout<<"Speed:"<<(10000*1000/(end-start))<<" Response Time:"<<resp/10000<<endl;
			start = end;
			resp = 0;
		}
	}
}

std::atomic<int64_t> async_resp(0);
std::atomic<int64_t> async_cc(0);
std::atomic<int64_t> async_start(0);

std::atomic<int64_t> async_sendQuest(0);
std::atomic<int64_t> async_recvAnswer(0);
std::atomic<int64_t> async_recvError(0);

class TestCallback: public AnswerCallback
{
	int64_t _send;
	int64_t _questId;

public:
	TestCallback(FPQuestPtr quest): _questId(quest->seqNum())
	{
		_send = exact_real_usec();
	}
	virtual void onAnswer(FPAnswerPtr answer)
	{
		async_recvAnswer++;
		assert(_questId == answer->seqNum());

		int64_t send = exact_real_usec();
		int64_t diff = send - _send;
		async_resp.fetch_add(diff);

		int64_t cc = async_cc.fetch_add(1);
		
		if(cc && cc % 10000 == 0)
		{
			int64_t s = async_sendQuest;
			int64_t a = async_recvAnswer;
			int64_t e = async_recvError;
			cout<<"sent quest: "<<s<<", recv answer: "<<a<<", error: "<<e<<", diff: "<<(s - a - e)<<endl;
		}
		if(cc && cc % 100000 == 0){
			int64_t end = slack_real_msec();
			cout<<"Speed:"<<(100000/(end-async_start))*1000<<" Response Time:"<<async_resp/100000<<endl;
			async_start = end;
			async_resp = 0;
		}
		
	}
	virtual void onException(FPAnswerPtr answer, int errorCode)
	{
		async_recvError++;
		if (errorCode == FPNN_EC_CORE_TIMEOUT)
			cout<<"Timeouted occurred when recving."<<endl;
		else
			cout<<"error occurred when recving."<<endl;
	}
};

void asyncProcess()
{
	for (int i = 0; i < sendCount; i++)
	{
		FPQuestPtr quest = QWriter(duplex ? "duplex demo" : "two way demo", false, FPMessage::FP_PACK_MSGPACK);
		TestCallback* cb = new TestCallback(quest);
		bool stat = false;
		try{

			if (proxyTypeId == 1)
			{
				stat = carpProxy->sendQuest(slack_real_msec(), quest, cb);
				if (carpProxy->empty())
					cout<<"carpProxy is empty"<<endl;
			}
			else if (proxyTypeId == 2)
			{
				stat = equProxy->sendQuest(quest, cb);
				if (equProxy->empty())
					cout<<"equProxy is empty"<<endl;
			}
			else if (proxyTypeId == 3)
			{
				stat = randomProxy->sendQuest(quest, cb);
				if (randomProxy->empty())
					cout<<"randomProxy is empty"<<endl;
			}
			else if (proxyTypeId == 4)
			{
				stat = consistencyProxy->sendQuest(quest, cb);
				if (consistencyProxy->empty())
					cout<<"consistencyProxy is empty"<<endl;
			}

			async_sendQuest++;
		}
		catch (const std::exception &ex)
		{
			cout<<"error occurred when sending. "<<ex.what()<<endl;
		}

		if (!stat)
		{
			delete cb;
			cout<<"send cb failed"<<endl;
		}
	}
}

void prepareProxy()
{
	std::vector<std::string> interest = {"serverTest"};
	std::string serverEndpoints = Setting::getString("FPNN.proxy.serverEndpoints", "");
	std::vector<std::string> endpoints;
	StringUtil::split(serverEndpoints, ", ", endpoints);

	std::string proxyType = Setting::getString("FPNN.proxy.type", "");
	syncQuest = Setting::getBool("FPNN.proxy.sync", true);
	duplex = Setting::getBool("FPNN.proxy.duplex", false);
	sharedDuplex = Setting::getBool("FPNN.proxy.duplex.shared", false);

	
	TCPProxyCore* pbm = NULL;
	if (proxyType == "1" || proxyType == "carpProxy")
	{
		proxyTypeId = 1;
		carpProxy.reset(new TCPCarpProxy);
		pbm = carpProxy.get();
	}
	else if (proxyType == "2" || proxyType == "rotatoryProxy")
	{
		proxyTypeId = 2;
		equProxy.reset(new TCPRotatoryProxy);
		pbm = equProxy.get();
	}
	else if (proxyType == "3" || proxyType == "randomProxy")
	{
		proxyTypeId = 3;
		randomProxy.reset(new TCPRandomProxy);
		pbm = randomProxy.get();
	}
	else if (proxyType == "4" || proxyType == "consistencyProxy")
	{
		proxyTypeId = 4;
		std::string condition = Setting::getString("FPNN.proxy.consistencyProxy.successCondition", "");
		if (condition == "all")
			consistencyProxy.reset(new TCPConsistencyProxy(ConsistencySuccessCondition::AllQuestsSuccess));
		else if (condition == "half")
			consistencyProxy.reset(new TCPConsistencyProxy(ConsistencySuccessCondition::HalfQuestsSuccess));
		else if (condition == "one")
			consistencyProxy.reset(new TCPConsistencyProxy(ConsistencySuccessCondition::OneQuestSuccess));
		else
		{
			cout<<"Bad success condition type!"<<endl;
			exit(1);
		}

		pbm = consistencyProxy.get();
	}
	else
	{
		cout<<"Bad proxy type!"<<endl;
		exit(1);
	}

	if (duplex)
	{
		if (sharedDuplex)
			pbm->setSharedQuestProcessor(std::make_shared<QuestProcessor>());
		else
			pbm->enablePrivateQuestProcessor(std::make_shared<QuestProcessorFactory>());

		ClientEngine::configQuestProcessThreadPool(2, 1, 2, 4, 10000);
	}

	pbm->updateEndpoints(endpoints);
}

int main(int argc, const char* argv[])
{
	if (argc != 2)
	{
		cout<<"Usage: "<<argv[0]<<" proxy.conf"<<endl;
		return 0;
	}
	if(!Setting::load(argv[1])){
		cout<<"Config file error:"<< argv[1]<<endl;
		return 1;
	}

	int thread_num = Setting::getInt("FPNN.test.threadNum", 10);
	sendCount = Setting::getInt("FPNN.test.sendCount", 10000);

	prepareProxy();

	vector<thread> threads;

	if (!syncQuest)
		async_start = exact_real_msec();

	for(int i=0;i<thread_num;++i){
		threads.push_back(thread(syncQuest ? syncProcess : asyncProcess));
	}   

	for(unsigned int i=0; i<threads.size();++i){
		threads[i].join();
	}

	if (!syncQuest)
	{
		cout<<"wait async answer ..."<<endl;
		while (async_sendQuest != async_recvAnswer + async_recvError)
			sleep(1);
	}

	return 0;
}
