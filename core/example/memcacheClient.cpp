#include <iostream>
#include <assert.h>
#include "TCPClient.h"

using namespace std;
using namespace fpnn;

int main(int argc, char* argv[])
{
	if (argc != 3)
	{
		cout<<"Usage: "<<argv[0]<<" ip port"<<endl;
		return 0;
	}

	std::shared_ptr<TCPClient> client = TCPClient::createClient(argv[1], atoi(argv[2]));

	//set cmd
	{
		FPQWriter qw(2,"set");
		qw.param("k", "key");
		qw.param("v", "value"); 
		FPQuestPtr quest = qw.take();
		FPAnswerPtr answer = client->sendQuest(quest);
		cout<<answer->json()<<endl;
	}

	//get cmd
	{
		FPQWriter qw(1,"get");
		qw.param("k", "key");
		FPQuestPtr quest = qw.take();
		FPAnswerPtr answer = client->sendQuest(quest);
		cout<<answer->json()<<endl;
	}

	//del
	{
		FPQWriter qw(1,"del");
		qw.param("k", "key");
		FPQuestPtr quest = qw.take();
		FPAnswerPtr answer = client->sendQuest(quest);
		cout<<answer->json()<<endl;
	}

	//add cmd
	{
		FPQWriter qw(2,"add");
		qw.param("k", "key");
		qw.param("v", "value"); 
		FPQuestPtr quest = qw.take();
		FPAnswerPtr answer = client->sendQuest(quest);
		cout<<answer->json()<<endl;
	}

	//replace cmd
	{
		FPQWriter qw(2,"replace");
		qw.param("k", "key");
		qw.param("v", "value"); 
		FPQuestPtr quest = qw.take();
		FPAnswerPtr answer = client->sendQuest(quest);
		cout<<answer->json()<<endl;
	}

	return 0;
}

