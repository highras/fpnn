#include <iostream>
#include <assert.h>
#include "TCPClient.h"

using std::cout;
using std::endl;
using namespace fpnn;

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

std::atomic<int64_t> resp(0);
std::atomic<int64_t> cc(0);
std::atomic<int64_t> start(0);

std::atomic<int64_t> sendQuest(0);
std::atomic<int64_t> recvAnswer(0);
std::atomic<int64_t> recvError(0);

int main(int argc, char* argv[])
{
	if (argc != 3 && argc != 4)
	{
		cout<<"Usage: "<<argv[0]<<" ip port [client_work_thread]"<<endl;
		return 0;
	}

	ClientEngine::setQuestTimeout(300);
	if (argc == 4)
	{
		int count = atoi(argv[3]);
		ClientEngine::configAnswerCallbackThreadPool(count, 1, count, count);
	}

	std::shared_ptr<TCPClient> client = TCPClient::createClient(argv[1], atoi(argv[2]));

	//int64_t start = slack_real_msec();
	//int64_t resp = 0;
	start = slack_real_msec();

	for (int i = 1; i; i++)
	{
		int64_t ssend = exact_real_usec();
		FPQuestPtr quest = QWriter("two way demo", false, FPMessage::FP_PACK_MSGPACK);
		try{
			client->sendQuest(quest, [ssend, quest](FPAnswerPtr answer, int errorCode)
				{
					if (errorCode != FPNN_EC_OK)
					{
						recvError++;
						if (errorCode == FPNN_EC_CORE_TIMEOUT)
							cout<<"Timeouted occurred when recving."<<endl;
						else
							cout<<"error occurred when recving."<<endl;
						return;
					}

					recvAnswer++;
					if (quest->isTwoWay()){
						assert(quest->seqNum() == answer->seqNum());
						//cout<<(answer->json())<<endl;
						int64_t send = exact_real_usec();
						int64_t diff = send - ssend;
						resp.fetch_add(diff);

						cc++;
						
						if(cc % 10000 == 0)
						{
							int64_t s = sendQuest;
							int64_t a = recvAnswer;
							int64_t e = recvError;
							cout<<"sent quest: "<<s<<", recv answer: "<<a<<", error: "<<e<<", diff: "<<(s - a - e)<<endl;
						}
						if(cc % 100000 == 0){
							int64_t end = slack_real_msec();
							cout<<"Speed:"<<(100000/(end-start))*1000<<" Response Time:"<<resp/100000<<endl;
							start = end;
							resp = 0;
						}
					}
				});
			sendQuest++;
		}
		catch (...)
		{
			cout<<"error occurred when sending"<<endl;
		}
	}
	client->close();
	return 0;
}
