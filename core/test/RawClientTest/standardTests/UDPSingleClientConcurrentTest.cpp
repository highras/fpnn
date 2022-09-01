#include <iostream>
#include <vector>
#include <thread>
#include "msec.h"
#include "../../../IQuestProcessor.h"
#include "../ProtocolClient/FpnnUDPClient.h"

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

	QuestProcessorClassBasicPublicFuncs
};

FPQuestPtr genQuest(){
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

void testThread(FpnnUDPClient* client, int count)
{
	int act = 0;
	for (int i = 0; i < count; i++)
	{
		if (i % 200 == 0)
			cout<<endl;
		
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

void test(FpnnUDPClient& client, int threadCount, int questCount)
{
	cout<<"========[ Test: thread "<<threadCount<<", per thread quest: "<<questCount<<" ]=========="<<endl;

	std::vector<std::thread> _threads;

	for(int i = 0 ; i < threadCount; i++)
		_threads.push_back(std::thread(testThread, &client, questCount));

	sleep(5);

	for(size_t i = 0; i < _threads.size(); i++)
		_threads[i].join();

	cout<<endl<<endl;
}

int main(int argc, char* argv[])
{
	if (argc != 3)
	{
		cout<<"Usage: "<<argv[0]<<" ip port"<<endl;
		return 0;
	}

	std::string endpoint(argv[1]);
	endpoint.append(":").append(argv[2]);

	FpnnUDPClient client(endpoint);
	client.setQuestProcessor(std::make_shared<Processor>());
	FPLog::init("std::cout", "FPNN.TEST", "FATAL", "SingleClientConcurrentTest");

	showSignDesc();
	
	test(client, 10, 30000);
	test(client, 20, 30000);
	test(client, 30, 30000);
	test(client, 40, 30000);
	test(client, 50, 30000);
	test(client, 60, 30000);

	cout<<endl<<"Single Concurrent Test done."<<endl;

	return 0;
}