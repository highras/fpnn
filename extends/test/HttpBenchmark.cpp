#include <iostream>
#include <atomic>
#include <memory>
#include <string>
#include "msec.h"
#include "ClientEngine.h"
#include "MultipleURLEngine.h"

using namespace fpnn;
using namespace std;

std::atomic<bool> running(true);

class Tester: public std::enable_shared_from_this<Tester>
{
public:
	std::string url;
	int clientCount;
	int perThreadClients;

private:
	MultipleURLEnginePtr _multiEngine;

	std::atomic<int64_t> _send;
	std::atomic<int64_t> _recv;
	std::atomic<int64_t> _sendError;
	std::atomic<int64_t> _recvError;
	std::atomic<int64_t> _timecost;

public:
	Tester(): clientCount(1000), perThreadClients(100),
		_send(0), _recv(0), _sendError(0), _recvError(0), _timecost(0)
	{
		MultipleURLEngine::init();
	}

	~Tester()
	{
		_multiEngine.reset();
		MultipleURLEngine::cleanup();
	}

	inline void incRecv() { _recv++; }
	inline void incRecvError() { _recvError++; }
	inline void addTimecost( int64_t cost) { _timecost.fetch_add(cost); }

	void pushNewTask();

	MultipleURLEngine::ResultCallbackPtr buildCallback()
	{
		class Callback: public MultipleURLEngine::ResultCallback
		{
			Tester* _tester;
			int64_t _send_time;
		public:
			Callback(Tester *tester): _tester(tester)
			{
				_send_time = exact_real_usec();
			}
			~Callback() {}

			virtual void onCompleted(MultipleURLEngine::Result &result)
			{
				_tester->incRecv();
					
				int64_t recv_time = exact_real_usec();
				int64_t diff = recv_time - _send_time;
				_tester->addTimecost(diff);

				_tester->pushNewTask();
			}
			virtual void onExpired(MultipleURLEngine::CURLPtr curl_unique_ptr)
			{
				_tester->incRecvError();
				cout<<"Task expired."<<endl;
				
				_tester->pushNewTask();
			}
			virtual void onTerminated(MultipleURLEngine::CURLPtr curl_unique_ptr)
			{
				_tester->incRecvError();
				cout<<"Task terminated."<<endl;
				
				_tester->pushNewTask();
			}
			virtual void onException(MultipleURLEngine::CURLPtr curl_unique_ptr, enum MultipleURLEngine::VisitStateCode errorCode, const char *errorInfo)
			{
				_tester->incRecvError();
				cout<<"Task excpetion."<<endl;
				
				_tester->pushNewTask();
			}
		};
	
		return MultipleURLEngine::ResultCallbackPtr(new Callback(this));
	}

	void launch()
	{
		int threadCount = clientCount / perThreadClients;
		if (threadCount * perThreadClients < clientCount)
			threadCount += 1;

		cout<<"threadCount "<<threadCount<<", clientCount: "<<clientCount<<", perThreadClients: "<<perThreadClients<<endl;
		_multiEngine.reset(new MultipleURLEngine(perThreadClients, threadCount, threadCount + 2, threadCount + 10, threadCount + 10));

		cout<<_multiEngine->status()<<endl;

		for (int i = 0; i < clientCount; i++)
		{
			bool status = _multiEngine->addToBatch(url, buildCallback());
			if (status)
				_send++;
			else
				_sendError++;
		}

		_multiEngine->commitBatch();
		cout<<_multiEngine->status()<<endl;
		sleep(1);
		cout<<_multiEngine->status()<<endl;
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
			cout<<_multiEngine->status()<<endl;
		}
	}
};

void Tester::pushNewTask()
{
	bool status = _multiEngine->visit(url, buildCallback());
	if (status)
		_send++;
	else
		_sendError++;
}

void showUsgae(const char* appName)
{
	cout<<"Usage: "<<appName<<" [options] <url>"<<endl;
	cout<<"Options:"<<endl;
	cout<<"  --clientCount          total client count. Default 1000."<<endl;
	cout<<"  --perThreadClients     how many clients share a MultipleURLEngine thread. Default 100."<<endl;
	cout<<"  --callbackThreadCount  How many threads to process http reponses. Default count is same as CPU count."<<endl;
}

bool processParams(int argc, const char** argv, Tester &tester)
{
	if (argc % 2)
	{
		cout<<"Parameters count error."<<endl;
		return false;
	}

	for (int i = 1; i < argc - 1; i += 2)
	{
		if (strcmp(argv[i], "--clientCount") == 0)
			tester.clientCount = atoi(argv[i+1]);
		else if (strcmp(argv[i], "--perThreadClients") == 0)
			tester.perThreadClients = atoi(argv[i+1]);
		else if (strcmp(argv[i], "--callbackThreadCount") == 0)
		{
			int count = atoi(argv[i+1]);
			ClientEngine::configAnswerCallbackThreadPool(count, 1, count, count);
		}
		else
		{
			cout<<"Invalid parameter. "<<argv[i]<<endl;
			return false;
		}
	}

	tester.url = argv[argc - 1];

	return true;
}

int main(int argc, const char** argv)
{
	Tester tester;
	if (processParams(argc, argv, tester) == false)
	{
		showUsgae(argv[0]);
		return 1;
	}

	tester.launch();
	tester.showStatistics();

	return 0;
}
