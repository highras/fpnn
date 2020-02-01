#include <sys/sysinfo.h>
#include <iostream>
#include <vector>
#include <thread>
#include <atomic>
#include "TCPClient.h"
#include "CommandLineUtil.h"

using std::cout;
using std::cerr;
using std::endl;
using namespace fpnn;

FPQuestPtr genQuest(){
    FPQWriter qw(2, "duplex");
    qw.param("quest", "one");
    qw.param("int", 2); 
	return qw.take();
}

class QuestProcessor: public IQuestProcessor
{
	QuestProcessorClassPrivateFields(QuestProcessor)
public:
	FPAnswerPtr duplexPush(const FPReaderPtr args, const FPQuestPtr quest, const ConnectionInfo& ci)
	{
		return FPAWriter(2, quest)("Simple","one")("duplex", 2);
	}

	QuestProcessor()
	{
		registerMethod("duplexPush", &QuestProcessor::duplexPush);
	}

	QuestProcessorClassBasicPublicFuncs
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

	void processEncrypt(TCPClientPtr client);
	void test_worker(int qps);

public:
	Tester(const std::string& ip, int port, int thread_num, int qps): _ip(ip), _port(port), _thread_num(thread_num), _qps(qps),
		_send(0), _recv(0), _sendError(0), _recvError(0), _timecost(0)
	{
	}

	~Tester()
	{
		stop();
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
			//dse = dse * 1000 * 1000 / real_time;
			//dre = dre * 1000 * 1000 / real_time;

			cout<<"time interval: "<<(real_time / 1000.0)<<" ms, send error: "<<dse<<", recv error: "<<dre<<endl;
			cout<<"[QPS] send: "<<ds<<", recv: "<<dr<<", per quest time cost: "<<dtc<<" usec"<<endl;
		}
	}
};

void Tester::test_worker(int qps)
{
	int usec = 1000 * 1000 / qps;
	Tester* ins = this;

	cout<<"-- qps: "<<qps<<", usec: "<<usec<<endl;

	int closeFrequence = CommandLineParser::getInt("close-frequence", 1000);
	int questCount = 0;

	TCPClientPtr client = TCPClient::createClient(_ip, _port);
	client->setQuestProcessor(std::make_shared<QuestProcessor>());
	processEncrypt(client);
	client->connect();

	while (true)
	{
		FPQuestPtr quest = genQuest();
		int64_t send_time = exact_real_usec();
		try{
			client->sendQuest(quest, [send_time, ins](FPAnswerPtr answer, int errorCode)
				{
					if (errorCode != FPNN_EC_OK)
					{
						ins->incRecvError();
						if (errorCode == FPNN_EC_CORE_TIMEOUT)
							cout<<"Timeouted occurred when recving."<<endl;
						else
							cout<<"error occurred when recving."<<endl;
						return;
					}

					ins->incRecv();
					
					int64_t recv_time = exact_real_usec();
					int64_t diff = recv_time - send_time;
					ins->addTimecost(diff);
				});
			_send++;

			questCount++;
			if (questCount >= closeFrequence)
			{
				questCount = 0;
				client->close();
				client = nullptr;

				client = TCPClient::createClient(_ip, _port);
				client->setQuestProcessor(std::make_shared<QuestProcessor>());
				processEncrypt(client);
			}
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
	client->close();
}

void Tester::processEncrypt(TCPClientPtr client)
{
	if (CommandLineParser::exist("ssl"))
		client->enableSSL();
	else if (CommandLineParser::exist("ecc-pem"))
	{
		bool packageMode = CommandLineParser::exist("package");
		bool reinforce = CommandLineParser::exist("256bits");
		std::string pemFile = CommandLineParser::getString("ecc-pem");
		client->enableEncryptorByPemFile(pemFile.c_str(), packageMode, reinforce);
	}
}

void showUsage(const char* appName)
{
	cout<<"Usage: "<<appName<<" ip port connections totalQPS [-ssl] [-thread client-work-thread-count] [-close-frequence quest-count]"<<endl;
	cout<<"Usage: "<<appName<<" ip port connections totalQPS [-ecc-pem ecc-pem-file [-package|-stream] [-128bits|-256bits]] [-thread client-work-thread-count] [-close-frequence quest-count]"<<endl;
}

int main(int argc, char* argv[])
{
	CommandLineParser::init(argc, argv);
	std::vector<std::string> mainParams = CommandLineParser::getRestParams();
	if (mainParams.size() != 4)
	{
		showUsage(argv[0]);
		return 0;
	}

	ClientEngine::setQuestTimeout(300);
	if (CommandLineParser::exist("thread"))
	{
		int count = CommandLineParser::getInt("thread");
		ClientEngine::configAnswerCallbackThreadPool(count, 1, count, count);
	}

	int cpuCount = get_nprocs();
	ClientEngine::configQuestProcessThreadPool(cpuCount, 0, cpuCount, cpuCount, 0);

	Tester tester(mainParams[0], std::stoi(mainParams[1]), std::stoi(mainParams[2]), std::stoi(mainParams[3]));

	tester.launch();
	tester.showStatistics();

	return 0;
}
