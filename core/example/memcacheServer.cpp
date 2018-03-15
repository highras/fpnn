#include <iostream>
#include "TCPEpollServer.h"
#include "IQuestProcessor.h"
#include "TaskThreadPool.h"
#include "msec.h"
#include "Setting.h"
#include "LruHashMap.h"

using namespace std;
using namespace fpnn;

typedef LruHashMap<string, string> CACHEMAP;

class QuestProcessor: public IQuestProcessor
{

	QuestProcessorClassPrivateFields(QuestProcessor)

	CACHEMAP *_cache;	
	mutex _mCache;

public:

	FPAnswerPtr set(const FPReaderPtr args, const FPQuestPtr quest, const ConnectionInfo& ci)
	{
		string key = args->want("k",string(""));
		string value = args->want("v",string(""));

		{
			lock_guard<mutex> lck(_mCache);
			_cache->replace(key,value);
		}

		return FPAWriter(1, quest)("status",true);
	}

	FPAnswerPtr get(const FPReaderPtr args, const FPQuestPtr quest, const ConnectionInfo& ci)
	{
		string key = args->want("k",string(""));
		string value;
		bool found = false;
		CACHEMAP::node_type* node = NULL;
		{
			lock_guard<mutex> lck(_mCache);
			node = _cache->find(key);
			if(node){
				value = node->data;
				found = true;
			}
		}

		return FPAWriter(2, quest)("status",found)("v", value);
	}

	FPAnswerPtr del(const FPReaderPtr args, const FPQuestPtr quest, const ConnectionInfo& ci)
	{
		string key = args->want("k",string(""));

		{
			lock_guard<mutex> lck(_mCache);
			_cache->remove(key);
		}

		return FPAWriter(1, quest)("status",true);
	}

	FPAnswerPtr add(const FPReaderPtr args, const FPQuestPtr quest, const ConnectionInfo& ci)
	{
		string key = args->want("k",string(""));
		string value = args->want("v",string(""));

		bool found = false;
		CACHEMAP::node_type* node = NULL;
		{
			lock_guard<mutex> lck(_mCache);
			node = _cache->find(key);
			if(node){
				found = true;
			}
			else{
				_cache->insert(key, value);
			}
		}

		return FPAWriter(1, quest)("status",!found);
	}


	FPAnswerPtr replace(const FPReaderPtr args, const FPQuestPtr quest, const ConnectionInfo& ci)
	{
		string key = args->want("k",string(""));
		string value = args->want("v",string(""));

		bool found = false;
		CACHEMAP::node_type* node = NULL;
		{
			lock_guard<mutex> lck(_mCache);
			node = _cache->find(key);
			if(node){
				_cache->replace(key, value);
				found = true;
			}
		}

		return FPAWriter(1, quest)("status",found);
	}

	QuestProcessor()
	{
		registerMethod("set", &QuestProcessor::set);
		registerMethod("get", &QuestProcessor::get);
		registerMethod("del", &QuestProcessor::del);
		registerMethod("add", &QuestProcessor::add);
		registerMethod("replace", &QuestProcessor::replace);
		
		 _cache = new CACHEMAP(1024000);
	}

	~QuestProcessor(){
		delete _cache;
	}

	QuestProcessorClassBasicPublicFuncs
};

int main(int argc, char* argv[])
{
	if (argc != 2)
	{
		cout<<"Usage: "<<argv[0]<<" config"<<endl;
		return 0;
	}
	if(!Setting::load(argv[1])){
		cout<<"Config file error:"<< argv[1]<<endl;
		return 1;
	}

	ServerPtr server = TCPEpollServer::create();
	server->setQuestProcessor(std::make_shared<QuestProcessor>());
	server->startup();
	server->run();

	return 0;
}
