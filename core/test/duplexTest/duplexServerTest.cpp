#include <iostream>
#include <sys/sysinfo.h>
#include "TCPEpollServer.h"
#include "IQuestProcessor.h"
#include "TaskThreadPool.h"
#include "msec.h"
#include "Setting.h"

using namespace std;
using namespace fpnn;

class QuestProcessor: public IQuestProcessor
{
	QuestProcessorClassPrivateFields(QuestProcessor)

	struct SenderInfo
	{
		QuestSenderPtr sender;
		int64_t connectedMsec;
	};

	std::mutex _mutex;
	volatile bool _running;
	std::thread _duplexThread;
	std::map<uint64_t, SenderInfo> _senderMap;

	void duplexThread()
	{
		while (_running)
		{
			usleep(50*1000);
			int64_t msec = slack_real_msec();

			std::lock_guard<std::mutex> lck(_mutex);
			for (auto& pp: _senderMap)
			{
				if (msec - pp.second.connectedMsec < 100)
					continue;

				FPQWriter qw(2, "duplexPush");
				qw.param("123", "xsxdd");
				qw.param("asd", 789);

				pp.second.sender->sendQuest(qw.take(), [](FPAnswerPtr answer, int errorCode){});
			}
		}
	}

public:

	virtual void connected(const ConnectionInfo& ci)
	{
		cout<<"server connected.socket ID:"<<ci.socket<<endl;

		std::lock_guard<std::mutex> lck(_mutex);

		_senderMap[ci.token] = { genQuestSender(ci), slack_real_msec() };
	}
	virtual void connectionWillClose(const ConnectionInfo& ci, bool closeByError)
	{
		cout<<"server "<<(closeByError ? "error" : "close")<<" event processed.socket ID:"<<ci.socket<<endl;

		std::lock_guard<std::mutex> lck(_mutex);
		_senderMap.erase(ci.token);
	}

	FPAnswerPtr duplexDemo(const FPReaderPtr args, const FPQuestPtr quest, const ConnectionInfo& ci)
	{
		return FPAWriter(2, quest)("Simple","one")("Simple2", 2);
	}

	QuestProcessor()
	{
		registerMethod("duplex", &QuestProcessor::duplexDemo);

		_running = true;
		_duplexThread = std::thread(&QuestProcessor::duplexThread, this);
	}
	~QuestProcessor()
	{
		_running = false;
		_duplexThread.join();
	}

	QuestProcessorClassBasicPublicFuncs
};

int main(int argc, char* argv[])
{
	if (argc != 2)
	{
		std::cout<<"Usage: "<<argv[0]<<" config"<<std::endl;
		return 0;
	}
	if(!Setting::load(argv[1])){
		std::cout<<"Config file error:"<< argv[1]<<std::endl;
		return 1;
	}

	int cpuCount = get_nprocs();
	TCPServerPtr server = TCPEpollServer::create();
	server->enableAnswerCallbackThreadPool(cpuCount, 0, cpuCount, cpuCount);
	server->setQuestProcessor(std::make_shared<QuestProcessor>());
	if (server->startup())
		server->run();

	return 0;
}