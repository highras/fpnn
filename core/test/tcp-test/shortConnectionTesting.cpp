#include <iostream>
#include <vector>
#include <thread>
#include <assert.h>
#include <atomic>
#include "TCPClient.h"
#include "IQuestProcessor.h"
#include "ClientEncryptConfig.h"

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

int sendCount = 0;
int thread_num = 0;
std::atomic<int> readyClient(0);
ClientEncryptConfig encryptConfig;

void sProcess(const char* ip, int port){
	std::shared_ptr<TCPClient> client = TCPClient::createClient(ip, port);
	encryptConfig.process(client);

	readyClient++;
	while (readyClient != thread_num)
		usleep(5000);

	int64_t start = exact_real_msec();
	int64_t resp = 0;
	for (int i = 1; i<=sendCount; i++)
	{
		client->connect();
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
		client->close();
	}
}

int main(int argc, char* argv[])
{
	if (argc != 5 && argc != 6)
	{
		cout<<"Usage: "<<argv[0]<<" ip port threadNum sendCount [encryptConfigFile]"<<endl;
		return 0;
	}

	ClientEngine::setQuestTimeout(300);

	if (argc == 6 && !encryptConfig.enableEncryption(argv[5]))
		return 1;

	thread_num = atoi(argv[3]);
	sendCount = atoi(argv[4]);
	vector<thread> threads;
	for(int i=0;i<thread_num;++i){
		threads.push_back(thread(sProcess, argv[1], atoi(argv[2])));
		if (i%100 == 0)
			cout<<"init thread "<<i<<endl;
	}   


	for(unsigned int i=0; i<threads.size();++i){
		threads[i].join();
	} 

	return 0;
}

