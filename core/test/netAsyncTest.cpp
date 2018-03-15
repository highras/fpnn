#include <iostream>
#include <assert.h>
#include <string>
#include <mutex>
#include <map>
#include "TCPClient.h"
#include "net.h"

using namespace fpnn;
using namespace std;

typedef std::map<uint32_t,FPQuestPtr> QuestMap;

std::mutex mut;
QuestMap questMap;

std::atomic<int64_t> resp(0);
std::atomic<int64_t> cc(0);
std::atomic<int64_t> start(0);

int sock_fd = -1;

FPQuestPtr QWriter(const char* method, bool oneway, FPMessage::FP_Pack_Type def_ptype){
	FPQWriter qw(6,method, oneway, def_ptype);
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


void writeData(){
	int count = 0;
	while(1){
		FPQuestPtr quest = QWriter("two way demo", false, FPMessage::FP_PACK_MSGPACK);
		{
			std::lock_guard<std::mutex> lck (mut);
			questMap.insert(make_pair(quest->seqNum(), quest));
		}
		string* raw = quest->raw();
		net_write_n(sock_fd, raw->data(), raw->size());
		delete raw;
		++count;
		if(count % 100000 == 0){
			cout<<"Send:"<<count<<"  Map:"<<questMap.size()<<endl;
			usleep(400*1000);
		}
	}

}

void readData(){
	FPMessage::Header hdr;
	int count = 0;
	while(1){
		net_read_n(sock_fd, &hdr, sizeof(hdr));
		size_t total_len = sizeof(FPMessage::Header) + FPMessage::BodyLen((const char*)&hdr);

		char* buf = (char*)malloc(total_len);
		memcpy(buf, &hdr, sizeof(hdr));

		net_read_n(sock_fd, buf+sizeof(hdr), total_len-sizeof(hdr));

		FPAnswerPtr answer(new FPAnswer(buf,total_len));
		free(buf);

		FPQuestPtr quest;
		{
			std::lock_guard<std::mutex> lck (mut);
			QuestMap::iterator it = questMap.find(answer->seqNum());
			if(it!=questMap.end()){
				quest = it->second;
				questMap.erase(answer->seqNum());
				resp += answer->ctime() - quest->ctime();
			}
			else{
				cout<<"ERROR answer seqID:"<<answer->seqNum()<<endl;
				continue;
			}
		}
		++count;
		if(count % 100000 == 0){
			cout<<"Recv:"<<count<<"  Map:"<<questMap.size()<<" Resp:"<<resp/100000<<endl;
			resp = 0;
		}
	}

}

int main(int argc, char* argv[])
{
	if (argc != 3)
	{
		cout<<"Usage: "<<argv[0]<<" ip port"<<endl;
		return 0;
	}

	sock_fd = net_tcp_connect(argv[1], atoi(argv[2]));

	//int64_t start = slack_real_msec();
	//int64_t resp = 0;
	start = slack_real_msec();

	vector<thread> threads;
	threads.push_back(thread(writeData));
	threads.push_back(thread(readData));

	for(unsigned int i=0; i<threads.size();++i){
		threads[i].join();
	}

	close(sock_fd);

	return 0;
}
