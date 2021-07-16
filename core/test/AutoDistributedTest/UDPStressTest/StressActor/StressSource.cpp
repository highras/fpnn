#include "UDPClient.h"
#include "PEM_DER_SAX.h"
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

void StressSource::processEncrypt(TCPClientPtr client)
{
	if (_encryptInfo.curveName.size())
		client->enableEncryptor(_encryptInfo.curveName, _encryptInfo.publicKey, _encryptInfo.packageMode, _encryptInfo.reinforce);
}

void StressSource::checkEncryptInfo(const FPReaderPtr payload)
{
	{
		std::string eccPem = payload->getString("eccPem");
		if (eccPem.size())
		{
			EccKeyReader reader;

			PemSAX pemSAX;
			if (pemSAX.parse(eccPem, &reader))
			{
				_encryptInfo.curveName = reader.curveName();
				_encryptInfo.publicKey = reader.rawPublicKey();
				_encryptInfo.packageMode = payload->getBool("packageMode", true);
				_encryptInfo.reinforce = payload->getBool("reinforce", false);
			}
		}
	}
}

bool StressSource::launch(int connections, int totalQPS)
{
	int perQPS = totalQPS / connections;
	if (perQPS == 0)
		perQPS = 1;
	int remainQPS = totalQPS - perQPS * connections;
	if (remainQPS < 0)
		remainQPS = 0;

	_running = true;
	
	for(int i = 0 ; i < connections; i++)
		_threads.push_back(std::thread(&StressSource::test_worker, this, perQPS));

	if (remainQPS)
		_threads.push_back(std::thread(&StressSource::test_worker, this, remainQPS));

	int clientCount = (int)_threads.size();
	_threads.push_back(std::thread(&StressSource::reportStatistics, this, clientCount));

	return true;
}

void StressSource::test_worker(int qps)
{
	int usec = 1000 * 1000 / qps;
	StressSource* ins = this;

	UDPClientPtr client = UDPClient::createClient(_endpoint);
	if (_mtu > 0)
		client->setMTU(_mtu);

	// processEncrypt(client);
	// client->connect();

	while (_running)
	{
		FPQuestPtr quest = genQuest();
		int64_t send_time = exact_real_usec();
		{
			bool status = client->sendQuest(quest, [send_time, ins](FPAnswerPtr answer, int errorCode)
				{
					if (errorCode != FPNN_EC_OK)
					{
						ins->incRecvError();
						return;
					}

					ins->incRecv();
					
					int64_t recv_time = exact_real_usec();
					int64_t diff = recv_time - send_time;
					ins->addTimecost(diff);
				});
			if (status)
				_send++;
			else
				_sendError++;
		}

		int64_t sent_time = exact_real_usec();
		int64_t real_usec = usec - (sent_time - send_time);
		if (real_usec > 0)
			usleep(real_usec);
	}
}

void StressSource::reportStatistics(int clientCount)
{
	const int sleepSeconds = 3;

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