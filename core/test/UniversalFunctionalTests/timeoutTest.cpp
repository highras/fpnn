#include <iostream>
#include "FPWriter.h"
#include "TCPClient.h"
#include "UDPClient.h"
#include "CommandLineUtil.h"
#include "IQuestProcessor.h"

using namespace std;
using namespace fpnn;

class QuestProcessor: public IQuestProcessor
{
	QuestProcessorClassPrivateFields(QuestProcessor)
public:
	virtual void connected(const ConnectionInfo&) { cout<<"client connected."<<endl; }
	virtual void connectionWillClose(const ConnectionInfo&, bool closeByError)
	{
		if (!closeByError)
			cout<<"client close event processed."<<endl;
		else
			cout<<"client error event processed."<<endl;
	}

	QuestProcessorClassBasicPublicFuncs
};

int main(int argc, char* argv[])
{
	CommandLineParser::init(argc, argv);
	std::vector<std::string> mainParams = CommandLineParser::getRestParams();

	if (mainParams.size() != 3)
	{
		cout<<"Usage: "<<argv[0]<<" ip port delay_seconds [-e engine_quest_timeout] [-c client_quest_timeout] [-q single_quest_timeout] [-udp]"<<endl;
		return 0;
	}

	int clientEngineQuestTimeout = CommandLineParser::getInt("e", 0);
	int clientQuestTimeout = CommandLineParser::getInt("c", 0);
	int questTimeout = CommandLineParser::getInt("q", 0);

	if (clientEngineQuestTimeout > 0)
		ClientEngine::setQuestTimeout(clientEngineQuestTimeout);
	
	std::shared_ptr<Client> client;
	if (CommandLineParser::exist("udp"))
		client = Client::createUDPClient(mainParams[0], atoi(mainParams[1].c_str()));
	else
		client = Client::createTCPClient(mainParams[0], atoi(mainParams[1].c_str()));

	client->setQuestProcessor(std::make_shared<QuestProcessor>());
	if (clientQuestTimeout > 0)
		client->setQuestTimeout(clientQuestTimeout);

	FPQWriter qw(1, "custom delay");
	qw.param("delaySeconds", atoi(argv[3]));

	try
	{
		FPAnswerPtr answer = client->sendQuest(qw.take(), questTimeout);
		cout << "send a quest, answer:" << endl;
		cout <<  answer->json() << endl;
	}
	catch (...)
	{
		cout<<"error occurred when sending"<<endl;
	}

	client->close();
	return 0;
}

