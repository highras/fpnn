#include <iostream>
#include <vector>
#include <thread>
#include <atomic>
#include "Setting.h"
#include "UDPClient.h"
#include "IQuestProcessor.h"

using std::cout;
using std::cerr;
using std::endl;
using namespace fpnn;

class QuestProcessor: public IQuestProcessor
{
	QuestProcessorClassPrivateFields(QuestProcessor)
	
public:
	virtual void connected(const ConnectionInfo& ci)
	{
		cout<<"client "<<ci.token<<" socket "<<ci.socket<<" connected."<<endl;
	}
	virtual void connectionWillClose(const ConnectionInfo& ci, bool closeByError)
	{
		cout<<"client "<<ci.token<<" socket "<<ci.socket<<" closed by "<<(closeByError ? "error" : "normal")<<"."<<endl;
	}

	QuestProcessorClassBasicPublicFuncs
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

struct EncryptInfo
{
	std::string publicKeyPath;
	bool packageReinforce;
	bool dataReinforce;
	bool dataEnhance;
};

class Tester
{
	std::string _ip;
	int _port;
	int _thread_num;
	int _qps;

	std::atomic<int64_t> _send;
	std::atomic<int64_t> _recv;
	std::atomic<int64_t> _sendError;
	std::atomic<int64_t> _recvError;
	std::atomic<int64_t> _timecost;

	std::vector<std::thread> _threads;
	EncryptInfo* _encryptInfo;

	void test_worker(int qps);

public:
	Tester(const char* ip, int port, int thread_num, int qps): _ip(ip), _port(port), _thread_num(thread_num), _qps(qps),
		_send(0), _recv(0), _sendError(0), _recvError(0), _timecost(0)
	{
		_encryptInfo = new EncryptInfo;
		_encryptInfo->publicKeyPath = Setting::getString("FPNN.client.security.serverPublicKey.path");

		if (_encryptInfo->publicKeyPath.empty())
		{
			delete _encryptInfo;
			_encryptInfo = NULL;
		}
		else
		{
			_encryptInfo->packageReinforce = Setting::getBool("FPNN.client.security.packageReinforce", false);
			_encryptInfo->dataReinforce = Setting::getBool("FPNN.client.security.dataReinforce", false);
			_encryptInfo->dataEnhance = Setting::getBool("FPNN.client.security.dataEnhance", false);
		}
	}

	~Tester()
	{
		stop();

		if (_encryptInfo)
			delete _encryptInfo;
	}

	inline void incRecv() { _recv++; }
	inline void incRecvError() { _recvError++; }
	inline void addTimecost( int64_t cost) { _timecost.fetch_add(cost); }

	void launch()
	{
		int pqps = _qps / _thread_num;
		if (pqps == 0)
			pqps = 1;
		int remain = _qps - pqps * _thread_num;
		if (remain < 0)
			remain = 0;

		for(int i = 0 ; i < _thread_num; i++)
			_threads.push_back(std::thread(&Tester::test_worker, this, pqps));

		if (remain)
			_threads.push_back(std::thread(&Tester::test_worker, this, remain));
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

		ClientEnginePtr ce;


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

			cout<<"\n====[send diff] "<<ds<<", [recv diff] "<<dr<<endl;

			ds = ds * 1000 * 1000 / real_time;
			dr = dr * 1000 * 1000 / real_time;
			//dse = dse * 1000 * 1000 / real_time;
			//dre = dre * 1000 * 1000 / real_time;

			cout<<"time interval: "<<(real_time / 1000.0)<<" ms, send error: "<<dse<<", recv error: "<<dre<<endl;
			cout<<"[QPS] send: "<<ds<<", recv: "<<dr<<", per quest time cost: "<<dtc<<" usec"<<endl;

			if (!ce)
				ce = ClientEngine::instance();

			cout<<"[Conn Count] "<<ce->count()<<endl;
		}
	}
};

void Tester::test_worker(int qps)
{
	int usec = 1000 * 1000 / qps;
	Tester* ins = this;

	cout<<"-- qps: "<<qps<<", usec: "<<usec<<endl;

	UDPClientPtr client = UDPClient::createClient(_ip, _port);
	client->setQuestProcessor(std::make_shared<QuestProcessor>());

	if (_encryptInfo)
	{
		bool ss = client->enableEncryptorByPemFile(_encryptInfo->publicKeyPath.c_str(), _encryptInfo->packageReinforce,
			_encryptInfo->dataEnhance, _encryptInfo->dataReinforce);
		if (!ss)
			cout<<"Enable encryption failed."<<endl;
	}
	//client->setMTU(1500);
	//client->connect();

	while (true)
	{
		FPQuestPtr quest = QWriter("two way demo", false, FPMessage::FP_PACK_MSGPACK);
		int64_t send_time = exact_real_usec();
		try{
			client->sendQuest(quest, [send_time, ins](FPAnswerPtr answer, int errorCode)
				{
					if (errorCode != FPNN_EC_OK)
					{
						ins->incRecvError();
					//	if (errorCode == FPNN_EC_CORE_TIMEOUT)
					//		cout<<"Timeouted occurred when recving."<<endl;
					//	else
					//		cout<<"error occurred when recving."<<endl;
						return;
					}

					ins->incRecv();
					
					int64_t recv_time = exact_real_usec();
					int64_t diff = recv_time - send_time;
					ins->addTimecost(diff);
				});
			_send++;
		}
		catch (...)
		{
			_sendError++;
			cerr<<"error occurred when sending"<<endl;
		}

		int64_t sent_time = exact_real_usec();
		int64_t real_usec = usec - (sent_time - send_time);
		if (real_usec > 0)
			usleep(real_usec);
	}
	//client->close();
}

/*
	Extera Config Item:

	FPNN.client.security.serverPublicKey.path = 
	FPNN.client.security.packageReinforce = 
	FPNN.client.security.dataReinforce = 
	FPNN.client.security.dataEnhance = 
*/

int main(int argc, char* argv[])
{
	if (argc < 5 || argc > 6)
	{
		//cout<<"Usage: "<<argv[0]<<" ip port connections qps [client_work_thread]"<<endl;
		cout<<"Usage: "<<argv[0]<<" ip port connections qps [config-file]"<<endl;
		return 0;
	}

	if (argc == 6)
		if(!Setting::load(argv[5])){
			cout<<"Config file error:"<< argv[5]<<endl;
			return 1;
		}

	ClientEngine::setQuestTimeout(300);
	// if (argc == 6)
	// {
	// 	int count = atoi(argv[5]);
	// 	ClientEngine::configAnswerCallbackThreadPool(count, 1, count, count);
	// }

	Tester tester(argv[1], atoi(argv[2]), atoi(argv[3]), atoi(argv[4]));

	tester.launch();
	tester.showStatistics();

	return 0;
}
