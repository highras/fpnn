#include <iostream>
#include "TCPClient.h"
#include "ignoreSignals.h"
#include "CommandLineUtil.h"

using namespace std;
using namespace fpnn;

FPQuestPtr buildQuest()
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

class QuestProcessor: public IQuestProcessor
{
	QuestProcessorClassPrivateFields(QuestProcessor)

	std::atomic<int64_t> _establishCount;
	std::atomic<int64_t> _closeCount;
	std::atomic<int64_t> _duplexCount;

public:
	virtual void connected(const ConnectionInfo&) { _establishCount++; }
	virtual void connectionWillClose(const ConnectionInfo&, bool closeByError) { _closeCount++; }

	FPAnswerPtr oneWayDuplexQuest(const FPReaderPtr args, const FPQuestPtr quest, const ConnectionInfo& ci)
	{
		_duplexCount++;
		return nullptr;
	}

	QuestProcessor(): _establishCount(0), _closeCount(0), _duplexCount(0)
	{
		registerMethod("one way duplex quest", &QuestProcessor::oneWayDuplexQuest);
	}

	void showStatic()
	{
		cout<<"Connections' establish count "<<_establishCount<<", close count "<<_closeCount<<", duplex cout "<<_duplexCount<<endl;
	}

	QuestProcessorClassBasicPublicFuncs
};
typedef std::shared_ptr<QuestProcessor> QuestProcessorPtr;


std::atomic<int> timeouted(0);
std::atomic<int> connClosed(0);
std::atomic<int> errorCount(0);
std::atomic<int> successCount(0);

std::vector<bool> completedSigns;
std::mutex gc_mutex;
int factor = 500;

void executeTest(const std::string& host, int port, QuestProcessorPtr processor, bool duplex, int idx)
{
	int64_t random = slack_real_msec();
	random = ((random >> (random & 0xf)) * (random & 0xf)) % 10;
	random *= factor;

	TCPClientPtr client = TCPClient::createClient(host, port);
	client->setQuestProcessor(processor);
	client->connect();

	while (random--)
	{
		client->sendQuest(buildQuest(), [random, idx](FPAnswerPtr answer, int errorCode){
			if (errorCode != FPNN_EC_OK && completedSigns[idx] == false)
			{
				std::unique_lock<std::mutex> lck(gc_mutex);
				cout<<" -- idx "<<idx<<" remain "<<random<<" error code "<<errorCode<<endl;
			}
		});

		usleep(1000);
	}

	completedSigns[idx] = true;
	sleep(1);

	FPQuestPtr quest;
	if (duplex)
	{
		FPQWriter qw(1, "close after sent");
		qw.param("duplex", true);

		quest = qw.take();
	}
	else
		quest = FPQWriter::emptyQuest("close after sent");
	
	client->sendQuest(quest, [](FPAnswerPtr answer, int errorCode){
		if (errorCode == FPNN_EC_OK)
			successCount++;
		else
		{
			errorCount++;

			if (errorCode == FPNN_EC_CORE_TIMEOUT)
				timeouted++;
			else if (errorCode == FPNN_EC_CORE_CONNECTION_CLOSED)
				connClosed++;
			else
				cout<<" -- [FIN] error code "<<errorCode<<endl;
		}
	});

	while (client->connected())
		sleep(1);

	sleep(1);
}


int main(int argc, const char* argv[])
{
	ignoreSignals();
	CommandLineParser::init(argc, argv);
	std::vector<std::string> params = CommandLineParser::getRestParams();
	if (params.size() != 2)
	{
		cout<<"Usage: "<<argv[0]<<" ip port [--connections 1000] [--duplex] [--factor 500]"<<endl;
		return 0;
	}

	int connections = CommandLineParser::getInt("connections", 1000);
	if (connections < 0)
		connections = -connections;

	int port = std::stoi(params[1]);
	bool duplex = CommandLineParser::exist("duplex");

	factor = CommandLineParser::getInt("factor", 500);

	ClientEngine::setQuestTimeout(60);
	ClientEngine::configQuestProcessThreadPool(2, 1, 4, 8, 10000*100);

	QuestProcessorPtr processor(new QuestProcessor());

	std::vector<std::thread> threads;
	for (int i = 0; i < connections; i++)
	{
		completedSigns.push_back(false);
		threads.push_back(std::thread(&executeTest, params[0], port, processor, duplex, i));
	}
	
	cout<<"--------- launch finished. ----------------"<<endl;

	for(size_t i = 0; i < threads.size(); i++)
		threads[i].join();

	cout<<"--------- testing finished. ----------------"<<endl;

	cout<<" Summary: connections "<<connections<<", duplex is "<<(duplex ? "enable" : "disable")<<endl;
	cout<<"Total error "<<errorCount<<", timeouted "<<timeouted<<", conn closed error "<<connClosed<<endl;
	cout<<"Successed "<<successCount<<endl;
	
	processor->showStatic();

	return 0;
}