#include <sys/sysinfo.h>
#include <iostream>
#include <vector>
#include <thread>
#include "msec.h"
#include "TCPClient.h"
#include "IQuestProcessor.h"
#include "CommandLineUtil.h"

using namespace std;
using namespace fpnn;

void showSignDesc()
{
	cout<<"Sign:"<<endl;
	cout<<"    +: establish connection"<<endl;
	cout<<"    ~: close connection"<<endl;
	cout<<"    #: connection error"<<endl;

	cout<<"    *: send sync quest"<<endl;
	cout<<"    &: send async quest"<<endl;

	cout<<"    ^: sync answer Ok"<<endl;
	cout<<"    ?: sync answer exception"<<endl;
	cout<<"    |: sync answer exception by connection closed"<<endl;
	cout<<"    (: sync operation fpnn exception"<<endl;
	cout<<"    ): sync operation unknown exception"<<endl;

	cout<<"    $: async answer Ok"<<endl;
	cout<<"    @: async answer exception"<<endl;
	cout<<"    ;: async answer exception by connection closed"<<endl;
	cout<<"    {: async operation fpnn exception"<<endl;
	cout<<"    }: async operation unknown exception"<<endl;

	cout<<"    !: close operation"<<endl;
	cout<<"    [: close operation fpnn exception"<<endl;
	cout<<"    ]: close operation unknown exception"<<endl;
}

class Processor: public IQuestProcessor
{
	QuestProcessorClassPrivateFields(Processor)

public:
	virtual void connected(const ConnectionInfo&) { cout<<"+"; }
	virtual void connectionWillClose(const ConnectionInfo& connInfo, bool closeByError)
	{
		closeByError ? cout<<"#" : cout<<"~";
	}

	FPAnswerPtr duplexPush(const FPReaderPtr args, const FPQuestPtr quest, const ConnectionInfo& ci)
	{
		return FPAWriter(2, quest)("Simple","one")("duplex", 2);
	}

	Processor()
	{
		registerMethod("duplexPush", &Processor::duplexPush);
	}

	QuestProcessorClassBasicPublicFuncs
};

FPQuestPtr genQuest(){
	FPQWriter qw(3, "duplex");
	qw.param("quest", "one");
	qw.param("int", 2); 
	qw.param("double", 3.3);
	return qw.take();
}

void testThread(TCPClientPtr client, int count)
{
	int act = 0;
	for (int i = 0; i < count; i++)
	{
		int64_t index = (slack_real_msec() + i + ((int64_t)(&i) >> 16)) % 64;
		if (i >= 10)
		{
			if (index < 6)
				act = 2;	//-- close operation
			else if (index < 32)
				act = 1;	//-- async quest
			else
				act = 0;	//-- sync quest
		}
		else
			act = index & 0x1;
		
		try
		{
			switch (act)
			{
				case 0:
				{
					cout<<"*";
					FPAnswerPtr answer = client->sendQuest(genQuest());
					if (answer)
					{
						if (answer->status() == 0)
							cout<<"^";
						else
						{
							FPAReader ar(answer);
							if (ar.wantInt("code") == FPNN_EC_CORE_CONNECTION_CLOSED || ar.wantInt("code") == FPNN_EC_CORE_INVALID_CONNECTION)
								cout<<"|";
							else
								cout<<"?";
						}
					}
					else
						cout<<"?";

					break;
				}
				case 1:
				{
					cout<<"&";
					bool status = client->sendQuest(genQuest(), [](FPAnswerPtr answer, int errorCode){
						if (errorCode == 0)
							cout<<"$";
						else if (errorCode == FPNN_EC_CORE_CONNECTION_CLOSED || errorCode == FPNN_EC_CORE_INVALID_CONNECTION)
							cout<<";";
						else
							cout<<"@";
					});
					if (status == false)
						cout<<"@";

					break;
				}
				case 2:
				{
					cout<<"!";
					client->close();
					break;
				}
			}
		}
		catch (const FpnnError& ex){
			switch (act)
			{
				case 0: cout<<"("; break;
				case 1: cout<<"{"; break;
				case 2: cout<<"["; break;
			}
		}
		catch (...)
		{
			switch (act)
			{
				case 0: cout<<")"; break;
				case 1: cout<<"}"; break;
				case 2: cout<<"]"; break;
			}
		}
	}
}

void test(TCPClientPtr client, int threadCount, int questCount)
{
	cout<<"========[ Test: thread "<<threadCount<<", per thread quest: "<<questCount<<" ]=========="<<endl;

	std::vector<std::thread> _threads;

	for(int i = 0 ; i < threadCount; i++)
		_threads.push_back(std::thread(testThread, client, questCount));

	sleep(5);

	for(size_t i = 0; i < _threads.size(); i++)
		_threads[i].join();

	cout<<endl<<endl;
}

void processEncrypt(TCPClientPtr client)
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
	cout<<"Usage: "<<appName<<" ip port [-ssl]"<<endl;
	cout<<"Usage: "<<appName<<" ip port [-ecc-pem ecc-pem-file [-package|-stream] [-128bits|-256bits]]"<<endl;
}

int main(int argc, char* argv[])
{
	CommandLineParser::init(argc, argv);
	std::vector<std::string> mainParams = CommandLineParser::getRestParams();
	if (mainParams.size() != 2)
	{
		showUsage(argv[0]);
		return 0;
	}

	int cpuCount = get_nprocs();
	ClientEngine::configQuestProcessThreadPool(cpuCount, 0, cpuCount, cpuCount, 0);

	TCPClientPtr client = TCPClient::createClient(mainParams[0], std::stoi(mainParams[1]));
	client->setQuestProcessor(std::make_shared<Processor>());
	FPLog::init("std::cout", "FPNN.TEST", "FATAL", "SingleClientConcurrentTest");

	processEncrypt(client);
	showSignDesc();
	
	test(client, 10, 30000);
	test(client, 20, 30000);
	test(client, 30, 30000);
	test(client, 40, 30000);
	test(client, 50, 30000);
	test(client, 60, 30000);

	cout<<endl<<endl<<"All test completed."<<endl;

	return 0;
}