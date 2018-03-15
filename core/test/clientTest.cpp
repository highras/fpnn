#include <iostream>
#include <vector>
#include <thread>
#include <assert.h>
#include "TCPClient.h"
#include "IQuestProcessor.h"

using namespace std;
using namespace fpnn;


class QuestProcessor: public IQuestProcessor
{
	QuestProcessorClassPrivateFields(QuestProcessor)
public:
	virtual void connected(const ConnectionInfo&) { cout<<"client connected."<<endl; }
	virtual void connectionWillClose(const ConnectionInfo&, bool closeByError)
	{
		if (!closeByError)
			cout<<"client close event processed."<<endl;
		else
			cout<<"client error event processed."<<endl;
	}

	//-- oneway
	FPAnswerPtr demoOneway(const FPReaderPtr args, const FPQuestPtr quest, const ConnectionInfo& ci)
	{
		cout<<"One way method demo is called. socket: "<<ci.socket<<", address: "<<ci.ip<<":"<<ci.port<<endl;
		cout<<"quest data: "<<(quest->json())<<endl;
		return NULL;
	}
	//-- towway
	FPAnswerPtr demoTowway(const FPReaderPtr args, const FPQuestPtr quest, const ConnectionInfo& ci)
	{
		cout<<"Tow way method demo is called. socket: "<<ci.socket<<", address: "<<ci.ip<<":"<<ci.port<<endl;
		cout<<"quest data: "<<(quest->json())<<endl;
		//std::string* data = new std::string("tow way quest answer");
		return FPAWriter::emptyAnswer(quest);
	}

	QuestProcessor()
	{
		registerMethod("one way demo", &QuestProcessor::demoOneway);
		registerMethod("tow way demo", &QuestProcessor::demoTowway);
	}

	QuestProcessorClassBasicPublicFuncs
};

const char* methodNames[] = {"one way demo", "tow way demo", "unkonwn method", "advance answer demo", "delay answer demo"};

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

void sProcess(const char* ip, int port){
	std::shared_ptr<TCPClient> client = TCPClient::createClient(ip, port);
	client->setQuestProcessor(std::make_shared<QuestProcessor>());
	int64_t start = exact_real_msec();
	int64_t resp = 0;
	for (int i = 1; i<=sendCount; i++)
	{
		FPQuestPtr quest = QWriter("two way demo", false, FPMessage::FP_PACK_MSGPACK);
		int64_t sstart = exact_real_usec();
		try{
			FPAnswerPtr answer = client->sendQuest(quest);
			if (quest->isTwoWay()){
				assert(quest->seqNum() == answer->seqNum());
				int64_t send = exact_real_usec();
				resp += send - sstart;
			}
		}
		catch (...)
		{
			cout<<"error occurred when sending"<<endl;
		}

		if(i % 10000 == 0){
			int64_t end = slack_real_msec();
			cout<<"Speed:"<<(10000*1000/(end-start))<<" Response Time:"<<resp/10000<<endl;
			start = end;
			resp = 0;
		}
	}
	client->close();
}

int main(int argc, char* argv[])
{
	if (argc != 5)
	{
		cout<<"Usage: "<<argv[0]<<" ip port threadNum sendCount"<<endl;
		return 0;
	}

	int thread_num = atoi(argv[3]);
	sendCount = atoi(argv[4]);
	vector<thread> threads;
	for(int i=0;i<thread_num;++i){
		threads.push_back(thread(sProcess, argv[1], atoi(argv[2])));
	}   


	for(unsigned int i=0; i<threads.size();++i){
		threads[i].join();
	} 

	return 0;
}

