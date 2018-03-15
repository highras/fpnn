#include <iostream>
#include "FPWriter.h"
#include "TCPClient.h"
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
	if ((argc != 5) && (argc != 6))
	{
		cout<<"Usage: "<<argv[0]<<" ip port delay_seconds engine_quest_timeout [client_quest_timeout]"<<endl;
		return 0;
	}

	ClientEngine::setQuestTimeout(atoi(argv[4]));
	
	std::shared_ptr<TCPClient> client = TCPClient::createClient(argv[1], atoi(argv[2]));
	client->setQuestProcessor(std::make_shared<QuestProcessor>());
	if (argc == 6)
		client->setQuestTimeout(atoi(argv[5]));

	FPQWriter qw(1, "custom delay");
	qw.param("delaySeconds", atoi(argv[3]));

	try
	{
		client->sendQuest(qw.take());
		cout << "send a quest" << endl;
	}
	catch (...)
	{
		cout<<"error occurred when sending"<<endl;
	}

	client->close();
	return 0;
}

