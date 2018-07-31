#include "StressSource.h"

FPQuestPtr genQuest()
{
	FPQWriter qw(6, "two way demo");
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

bool StressSource::launch(int threadCount, int clientCount, double perClientQPS)
{
	int perThreadClients = clientCount / threadCount;
	if (perThreadClients == 0)
	{
		threadCount = clientCount;
		perThreadClients = 1;
	}

	double perThreadQPS = perClientQPS * perThreadClients;
	_sleepUsec = 1000 * 1000 / perThreadQPS;
	if (_sleepUsec >= 1000 * 1000)
	{
		_sleepBySecond = true;
		_sleepSec = _sleepUsec / 1000000;
	}
	else
		_sleepBySecond = false;

	_running = true;

	for (int i = 0; i < threadCount; i++)
	{
		_clients.push_back(std::vector<TCPClientPtr>());
		
		std::vector<TCPClientPtr>& vec = _clients[i];
		for (int k = 0; k < perThreadClients; k++)
		{
			std::shared_ptr<TCPClient> client = TCPClient::createClient(_endpoint);
			vec.push_back(client);
			client->connect();
		}

		_threads.push_back(std::thread(&StressSource::test_worker, this, i));
	}

	_threads.push_back(std::thread(&StressSource::reportStatistics, this, perThreadClients * threadCount));

	return true;
}

void StressSource::adjustStress(int64_t &waitingMsec)
{
	if (!_decSleepMsec)
		return;

	if (!_sleepBySecond && (_sleepUsec <= _minSleepMsec * 1000))
		return;

	if (_sleepBySecond)
		waitingMsec -= _sleepSec * 1000;
	else
		waitingMsec -= _sleepUsec / 1000;

	if (waitingMsec <= 0)
	{
		waitingMsec = _intervalMinute * 60 * 1000;
		if (_sleepBySecond)
		{
			if (_sleepSec > 2)
				_sleepSec -= 1;
			else
			{
				_sleepBySecond = false;
				_sleepUsec = (1000 - _decSleepMsec) * 1000;
			}
		}
		else
		{
			_sleepUsec -= _decSleepMsec * 1000;
			if (_sleepUsec < _minSleepMsec * 1000)
				_sleepUsec = _minSleepMsec * 1000;
		}
	}
}

void StressSource::test_worker(int idx)
{
	int64_t waitingMsec = _firstWaitMinute * 60 * 1000;

	while (_running)
	{
		for (auto client: _clients[idx])
		{
			FPQuestPtr quest = genQuest();
			{
				int64_t send_time = exact_real_usec();
				FPAnswerPtr answer = client->sendQuest(quest);
				int64_t recv_time = exact_real_usec();

				if (answer)
				{
					_send++;
					if (answer->status() == 0)
					{
						int64_t diff = recv_time - send_time;
						addTimecost(diff);
						_recv++;
					}
					else
						_recvError++;
				}
				else
					_sendError++;
			}

			_sleepBySecond ? sleep(_sleepSec) : usleep(_sleepUsec);
			adjustStress(waitingMsec);
		}
	}
}

void StressSource::reportStatistics(int clientCount)
{
	const int sleepSeconds = 10;

	int64_t send = _send;
	int64_t recv = _recv;
	int64_t sendError = _sendError;
	int64_t recvError = _recvError;
	int64_t timecost = _timecost;


	while (_running)
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

		//ds = ds * 1000 * 1000 / real_time;
		//dr = dr * 1000 * 1000 / real_time;

		FPWriter pw(7);
		pw.param("connections", clientCount);
		pw.param("period", real_time);
		pw.param("send", ds);
		pw.param("recv", dr);
		pw.param("serror", dse);
		pw.param("rerror", dre);
		pw.param("allcost", dtc);

		FPQWriter qw(3, "actorStatus");
		qw.param("taskId", _taskId);
		qw.param("region", _region);
		qw.param("payload", pw.raw());

		ControlCenter::sendQuest(qw.take(), [](FPAnswerPtr answer, int errorCode){}, 0);
	}
}