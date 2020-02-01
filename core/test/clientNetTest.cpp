#include <iostream>
#include <vector>
#include <thread>
#include <assert.h>
#include "TCPClient.h"
#include "IQuestProcessor.h"

using namespace std;
using namespace fpnn;

FPQuestPtr QWriter(int32_t datalen){
	char* tt = (char*)malloc(datalen);
	string data(tt, datalen);
	free(tt);
    FPQWriter qw(2, "two way demo");
    qw.param("key", "only test");
    qw.param("data", data); 
	return qw.take();
}

int main(int argc, char* argv[])
{
	if (argc != 3)
	{
		cout<<"Usage: "<<argv[0]<<" ip:port count"<<endl;
		return 0;
	}

	string host = argv[1];
	int sendCount = atoi(argv[2]);

	std::shared_ptr<TCPClient> client = TCPClient::createClient(host);

	set<int32_t> datalens = {100, 500, 1000, 5000, 10000, 50000, 100000, 200000, 400000, 600000};
	for(auto& datalen : datalens){
		FPQuestPtr quest = QWriter(datalen);
		int64_t start = slack_real_msec();
		for(int32_t i = 0; i < sendCount; ++i){
			client->sendQuest(quest);
		}
		int64_t end = slack_real_msec();
		cout<<datalen<<"="<<int((end-start)/sendCount)<<endl;
	}

	return 0;
}

