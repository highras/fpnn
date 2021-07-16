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

class Tester;

class MassiveCallback: public AnswerCallback
{
public:
	virtual void onAnswer(FPAnswerPtr);
	virtual void onException(FPAnswerPtr answer, int errorCode);

	int64_t send_time;
	Tester* tester;
};

class Tester
{
	std::string _ip;
	int _port;

	std::atomic<int64_t> _send;
	std::atomic<int64_t> _recv;
	std::atomic<int64_t> _sendError;
	std::atomic<int64_t> _recvError;
	std::atomic<int64_t> _timecost;

	int _readyClient;
	std::mutex _mutex;
	std::condition_variable _condition;
	std::vector<std::thread> _threads;

	void test_worker(int wid, int clientCount, double perClientQPS);

	void calcThreadIntervalParams(int clientCount, double perClientQPS, int &sleepSec, int &sleepUsec, bool &sleepBySecond)
	{
		double threadQPS = perClientQPS * clientCount;
		sleepUsec = 1000 * 1000 / threadQPS;

		if (sleepUsec >= 1000 * 1000)
		{
			sleepBySecond = true;
			sleepSec = sleepUsec / 1000000;
		}
		else
			sleepBySecond = false;
	}

public:
	Tester(): _send(0), _recv(0), _sendError(0), _recvError(0), _timecost(0), _readyClient(0)
	{
	}

	~Tester()
	{
		stop();
	}

	inline void incRecv() { _recv++; }
	inline void incRecvError() { _recvError++; }
	inline void addTimecost( int64_t cost) { _timecost.fetch_add(cost); }

	bool launch(int argc, char* argv[])
	{
		if (argc < 6 || argc > 8)
		{
			cout<<"Usage: "<<argv[0]<<" <ip> <port> <threadNum> <clientCount> <perClientQPS> [timeout] [worker_threads]"<<endl;
			return false;
		}

		_ip = argv[1];
		_port = atoi(argv[2]);

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

	    if (clientCount == 0 || thread_num == 0)
		{
			cout<<"clientCount or threadNum is zero!"<<endl;
			return false;
		}

		int perThreadClients = clientCount / thread_num;
		if (perThreadClients == 0)
		{
			cout<<"clientCount less than threadNum! threadNum will change to same as clientCount."<<endl;
			thread_num = clientCount;
			perThreadClients = 1;
		}

		for(int i = 0 ; i < thread_num; i++)
		{
			_threads.push_back(std::thread(&Tester::test_worker, this, i, perThreadClients, perClientQPS));
			clientCount -= perThreadClients;

			if (i%100 == 0)
				cout<<"init thread "<<i<<endl;
		}

		if (clientCount)
			_threads.push_back(std::thread(&Tester::test_worker, this, thread_num, clientCount, perClientQPS));

		return true;
	}

	void stop()
	{
		for(size_t i = 0; i < _threads.size(); i++)
			_threads[i].join();
	}

	void showStatistics()
	{
		const int sleepSeconds = 3;

		int64_t send = _send;
		int64_t recv = _recv;
		int64_t sendError = _sendError;
		int64_t recvError = _recvError;
		int64_t timecost = _timecost;


		while (true)
		{
			int64_t start = exact_real_usec();

			sleep(sleepSeconds);

			int64_t s = _send;
			int64_t r = _recv;
			int64_t se = _sendError;
			int64_t re = _recvError;
			int64_t tc = _timecost;

			int64_t ent = exact_real_usec();

			int64_t ds = s - send;
			int64_t dr = r - recv;
			int64_t dse = se - sendError;
			int64_t dre = re - recvError;
			int64_t dtc = tc - timecost;

			send = s;
			recv = r;
			sendError = se;
			recvError = re;
			timecost = tc;

			int64_t real_time = ent - start;

			if (dr)
				dtc = dtc / dr;

			ds = ds * 1000 * 1000 / real_time;
			dr = dr * 1000 * 1000 / real_time;

			cout<<"time interval: "<<(real_time / 1000.0)<<" ms, send error: "<<dse<<", recv error: "<<dre<<endl;
			cout<<"[QPS] send: "<<ds<<", recv: "<<dr<<", per quest time cost: "<<dtc<<" usec"<<endl;
		}
	}
};

void Tester::test_worker(int wid, int clientCount, double perClientQPS)
{
	bool sleepBySecond;
	int sleepSec = 0, sleepUsec = 0;
	calcThreadIntervalParams(clientCount, perClientQPS, sleepSec, sleepUsec, sleepBySecond);

	vector<std::shared_ptr<TCPClient>> clients;
	for (int i = 0; i < clientCount; i++)
	{
		std::shared_ptr<TCPClient> client = TCPClient::createClient(_ip, _port);
		clients.push_back(client);
	}

	if (wid)
	{
		std::unique_lock<std::mutex> lck(_mutex);
		while (_readyClient != wid * clientCount)
			_condition.wait(lck);
	}

	for (auto client: clients)
		client->connect();

	{
		std::unique_lock<std::mutex> lck(_mutex);
		_readyClient += clientCount;
		_condition.notify_all();
	}

	cout<<"-- qps: "<<(perClientQPS * clientCount)<<", sleep ";
	sleepBySecond ? cout<<sleepSec<<" sec"<<endl : cout<<sleepUsec<<" usec"<<endl;
	
	while (true)
	{
		for (auto client: clients)
		{
			int64_t begin_time = exact_real_usec();

			MassiveCallback* callback = new MassiveCallback;
			FPQuestPtr quest = QWriter("two way demo", false, FPMessage::FP_PACK_MSGPACK);

			callback->tester = this;
			callback->send_time = exact_real_usec();

			if (client->sendQuest(quest, callback))
				_send++;
			else
			{
				delete callback;
				_sendError++;
			}

			if (sleepBySecond)
				sleep(sleepSec);
			else
			{
				int64_t sent_time = exact_real_usec();
				int64_t real_usec = sleepUsec - (sent_time - begin_time);
				if (real_usec > 0)
					usleep(real_usec);
			}
		}
	}
};

void MassiveCallback::onAnswer(FPAnswerPtr answer)
{
	int64_t recv_time = exact_real_usec();
	int64_t diff = recv_time - send_time;
	
	tester->incRecv();
	tester->addTimecost(diff);
}

void MassiveCallback::onException(FPAnswerPtr answer, int errorCode)
{
	tester->incRecvError();
	if (errorCode == FPNN_EC_CORE_TIMEOUT)
		cout<<"Timeouted occurred when recving."<<endl;
	else
		cout<<"error occurred when recving."<<endl;
}

int main(int argc, char* argv[])
{
	struct rlimit rlim;
	rlim.rlim_cur = 100000;
	rlim.rlim_max = 100000;
	setrlimit(RLIMIT_NOFILE, &rlim);

	Tester tester;
	if (tester.launch(argc, argv) == false)
		return 1;

	tester.showStatistics();

	return 0;
}
