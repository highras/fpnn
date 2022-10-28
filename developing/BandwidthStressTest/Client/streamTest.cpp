#include <iostream>
#include <vector>
#include <thread>
#include <atomic>
#include "TCPClient.h"
#include "UDPClient.h"
#include "CommandLineUtil.h"
#include "FormattedPrint.h"

using std::cout;
using std::cerr;
using std::endl;
using namespace fpnn;

class Tester
{
	std::string _endpoint;
	int _thread_num;
	int _qps;
	int _blockSize;
	int _taskBaseId;

	std::atomic<int64_t> _send;
	std::atomic<int64_t> _recv;
	std::atomic<int64_t> _sendError;
	std::atomic<int64_t> _recvError;
	std::atomic<int64_t> _timecost;

	std::vector<std::thread> _threads;

	void test_worker(int qps, int idx);

public:
	Tester(const std::string& endpoint, int thread_num, int qps, int KBSize): _endpoint(endpoint), _thread_num(thread_num), _qps(qps),
		_blockSize(KBSize * 1024), _send(0), _recv(0), _sendError(0), _recvError(0), _timecost(0)
	{
		_taskBaseId = (int)slack_real_sec();
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
			_threads.push_back(std::thread(&Tester::test_worker, this, pqps, i));

		if (remain)
			_threads.push_back(std::thread(&Tester::test_worker, this, remain, _thread_num));
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

			cout<<"------"<<endl;
			cout<<"Total: send: "<<formatBytesQuantity(_send * _blockSize)<<", trans: "<<formatBytesQuantity(_recv * _blockSize)<<endl;
			cout<<"Period: send: "<<formatBytesQuantity(ds * _blockSize)<<", trans: "<<formatBytesQuantity(dr * _blockSize)<<endl;
		}
	}
};

void Tester::test_worker(int qps, int idx)
{
	int usec = 1000 * 1000 / qps;
	int seq = 0;
	void* data = malloc(_blockSize);
	Tester* ins = this;

	cout<<"-- qps: "<<qps<<", usec: "<<usec<<endl;

	ClientPtr client;
	if (CommandLineParser::exist("udp"))
		client = UDPClient::createClient(_endpoint);
	else
		client = TCPClient::createClient(_endpoint);

	client->connect();

	while (true)
	{
		FPQWriter qw(3, "stream");
		qw.param("taskId", _taskBaseId + idx);
		qw.param("seq", seq++);
		qw.paramBinary("data", data, _blockSize);

		FPQuestPtr quest = qw.take();
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
	free(data);
}

int main(int argc, char* argv[])
{
	CommandLineParser::init(argc, argv);
	std::vector<std::string> mainParams = CommandLineParser::getRestParams();
	if (mainParams.size() != 4)
	{
		cout<<"Usage: "<<argv[0]<<" <endpoint> <connections> <totalQPS> <data-KB-size> [-udp]"<<endl;
		return 1;
	}

	ClientEngine::setQuestTimeout(300);
	Tester tester(mainParams[0], std::stoi(mainParams[1]), std::stoi(mainParams[2]), std::stoi(mainParams[3]));

	tester.launch();
	tester.showStatistics();

	return 0;
}
