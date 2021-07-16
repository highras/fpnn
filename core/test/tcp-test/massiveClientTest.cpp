#include <sys/resource.h>
#include <iostream>
#include <vector>
#include <thread>
#include <atomic>
#include "TCPClient.h"
#include "IQuestProcessor.h"

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

int sleepSec = 1;
int sleepUsec = 1000;
bool sleepBySecond = false;
int printUnit = 500;

int readyClient = 0;
std::mutex _mutex;
std::condition_variable _condition;

void sProcess(const char* ip, int port, int count, int idx)
{
	vector<std::shared_ptr<TCPClient>> clients;
	for (int i = 0; i < count; i++)
	{
		std::shared_ptr<TCPClient> client = TCPClient::createClient(ip, port);
		clients.push_back(client);
	}

	if (idx)
	{
		std::unique_lock<std::mutex> lck(_mutex);
		while (readyClient != idx * count)
			_condition.wait(lck);
	}

	for (auto client: clients)
		client->connect();

	{
		std::unique_lock<std::mutex> lck(_mutex);
		readyClient += count;
		_condition.notify_all();
	}

	cout<<"-------------------------"<<endl;
	
	int64_t start = exact_real_msec();
	int64_t resp = 0;
	int64_t i = 0;
	while (true)
	{
		for (auto client: clients)
		{
			FPQuestPtr quest = QWriter("two way demo", false, FPMessage::FP_PACK_MSGPACK);
			try{
				int64_t sstart = exact_real_usec();
				FPAnswerPtr answer = client->sendQuest(quest);
				int64_t send = exact_real_usec();
				resp += send - sstart;
			}
			catch (...)
			{
				cout<<"error occurred when sending"<<endl;
			}
			i++;

			if(i == printUnit)
			{
				int64_t end = slack_real_msec();
				cout<<"[Per thread] Speed(QPS): "<<(printUnit*1000/(end-start))<<", Response Time:"<<resp/printUnit<<" (usec)"<<endl;
				start = end;
				resp = 0;
				i = 0;
			}

			sleepBySecond ? sleep(sleepSec) : usleep(sleepUsec);
		}
	}
}

int main(int argc, char* argv[])
{
	if (argc < 6 || argc > 8)
	{
		cout<<"Usage: "<<argv[0]<<" <ip> <port> <threadNum> <clientCount> <perClientQPS> [timeout] [worker_threads]"<<endl;
		return 0;
	}

	int thread_num = atoi(argv[3]);
	int clientCount = atoi(argv[4]);
	double perClientQPS = atof(argv[5]);

	if (argc >= 7)
		ClientEngine::setQuestTimeout(atoi(argv[6]));

	if (argc == 8)
	{
		int count = atoi(argv[7]);
		ClientEngine::configAnswerCallbackThreadPool(count, 1, count, count);
	}

	struct rlimit rlim;
	rlim.rlim_cur = 100000;
    rlim.rlim_max = 100000;
    setrlimit(RLIMIT_NOFILE, &rlim);

    if (clientCount == 0 || thread_num == 0)
	{
		cout<<"clientCount or threadNum is zero!"<<endl;
		return 1;
	}

	int perThreadClients = clientCount / thread_num;
	if (perThreadClients == 0)
	{
		cout<<"clientCount less than threadNum! threadNum will change to same as clientCount."<<endl;
		thread_num = clientCount;
		perThreadClients = 1;
	}

	double perThreadQPS = perClientQPS * perThreadClients;
	int sleepUsec = 1000 * 1000 / perThreadQPS;

	cout<<"perThreadClients: "<<perThreadClients<<", sleep time: ";

	if (sleepUsec >= 1000 * 1000)
	{
		sleepBySecond = true;
		sleepSec = sleepUsec / 1000000;
		printUnit = 10;

		cout<<sleepSec<<" sec, print uint: "<<printUnit<<", print interval: "<<(printUnit * sleepUsec)<<" sec"<<endl;
	}
	else
	{
		sleepBySecond = false;

		if (sleepUsec >= 1000 * 1000)
			printUnit = 100;
		else if (sleepUsec >= 500 * 1000)
			printUnit = 500;
		else if (sleepUsec >= 100 * 1000)
			printUnit = 1000;
		else
			printUnit = 1000;

		cout<<sleepUsec<<" usec, print uint: "<<printUnit<<", print interval: "<<(printUnit * 1.0 /(1000 * 1000 / sleepUsec))<<" sec"<<endl;
	}

	vector<thread> threads;
	for(int i=0;i<thread_num;++i){
		threads.push_back(thread(sProcess, argv[1], atoi(argv[2]), perThreadClients, i));
		clientCount -= perThreadClients;

		if (i%100 == 0)
			cout<<"init thread "<<i<<endl;
	}

	if (clientCount)
		threads.push_back(thread(sProcess, argv[1], atoi(argv[2]), clientCount, thread_num));

	for(unsigned int i=0; i<threads.size();++i){
		threads[i].join();
	} 

	return 0;
}
