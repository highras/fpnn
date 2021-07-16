#include <iostream>
#include <unistd.h>
#include "ServerInfo.h"
#include "ignoreSignals.h"
#include "CommandLineUtil.h"
#include "TCPClient.h"
#include "IQuestProcessor.h"
#include "ExecutiveActor.h"

using namespace std;
using namespace fpnn;

class ActorQuestProcessor: public IQuestProcessor
{
	QuestProcessorClassPrivateFields(ActorQuestProcessor)

	ExecutiveActor* _actor;

public:
	ActorQuestProcessor(ExecutiveActor* actor): _actor(actor)
	{
		registerMethod("action", &ActorQuestProcessor::action);
	}

	FPAnswerPtr action(const FPReaderPtr args, const FPQuestPtr quest, const ConnectionInfo& ci)
	{
		int taskId = args->wantInt("taskId");
		std::string method = args->wantString("method");
		std::string payload = args->wantString("payload");

		FPReaderPtr reader(new FPReader(payload));
		try
		{
			_actor->action(taskId, method, reader);
			return FPAWriter::emptyAnswer(quest);
		}
		catch (const FpnnError& ex) {
			return FpnnErrorAnswer(quest, ex.code(), ex.message());
		}
		catch (...) {
			return FpnnErrorAnswer(quest, FPNN_EC_CORE_UNKNOWN_ERROR, "Unknown Error.");
		}
	}

	QuestProcessorClassBasicPublicFuncs
};

class Actor
{
	std::mutex _mutex;
	std::string _region;
	TCPClientPtr _client;
	ExecutiveActor _actor;
	bool _taskChanged;
	std::map<int, std::vector<std::string>> _taskMap;

	bool registerActor();

public:
	Actor(): _taskChanged(false) {}

	bool init()
	{
		if (!_actor.globalInit())
			return false;
		
		std::vector<std::string> restParams = CommandLineParser::getRestParams();

		if (restParams.empty() || restParams.size() > 2)
			return false;

		std::string endpoint = restParams[0];
		if (restParams.size() == 2)
			endpoint.append(":").append(restParams[1]);

		_region = ServerInfo::getServerRegionName();
		_client = TCPClient::createClient(endpoint);
		if (!_client)
			return false;

		_actor.setRegion(_region);
		_client->setQuestProcessor(std::make_shared<ActorQuestProcessor>(&_actor));
		return true;
	}

	void loop()
	{
		int pingTicket = 0;

		while (_actor.actorStopped() == false)
		{
			sleep(2);
			pingTicket += 2;

			if (!_client->connected())
			{
				_client->connect();
				_taskChanged = !registerActor();
			}

			if (_taskChanged)
			{
				_taskChanged = !registerActor();
			}

			if (pingTicket >= 15)
			{
				_client->sendQuest(FPQWriter::emptyQuest("ping"), [](FPAnswerPtr, int){});
				pingTicket = 0;
			}
		}
	}

	~Actor()
	{
		_client = nullptr;
	}

	void beginTask(int taskId, const std::string& method, const std::string& desc)
	{
		std::unique_lock<std::mutex> lck(_mutex);
		_taskMap[taskId].push_back(method);
		_taskMap[taskId].push_back(desc);
	}
	void finishTask(int taskId)
	{
		std::unique_lock<std::mutex> lck(_mutex);
		_taskMap.erase(taskId);
		_taskChanged = true;
	}

	FPAnswerPtr sendQuest(FPQuestPtr quest, int timeout)
	{
		return _client->sendQuest(quest, timeout);
	}
	bool sendQuest(FPQuestPtr quest, AnswerCallback* callback, int timeout)
	{
		return _client->sendQuest(quest, callback, timeout);
	}
	bool sendQuest(FPQuestPtr quest, std::function<void (FPAnswerPtr answer, int errorCode)> task, int timeout)
	{
		return _client->sendQuest(quest, std::move(task), timeout);
	}
};

bool Actor::registerActor()
{
	FPQWriter qw(4, "registerActor");
	qw.param("region", _region);
	qw.param("name", _actor.actorName());
	qw.param("pid", (int64_t)getpid());
	{
		std::unique_lock<std::mutex> lck(_mutex);
		qw.param("executingTasks", _taskMap);
	}

	TCPClientPtr client = _client;
	return _client->sendQuest(qw.take(), [client](FPAnswerPtr answer, int errorCode){
		if (errorCode != FPNN_EC_OK)
		{
			cout<<"[Error] Register actor exception. error code: "<<errorCode<<endl;
			client->close();
		}
	});
}

Actor gc_Actor;

void ControlCenter::beginTask(int taskId, const std::string& method, const std::string& desc)
{
	gc_Actor.beginTask(taskId, method, desc);
}
void ControlCenter::finishTask(int taskId)
{
	gc_Actor.finishTask(taskId);
}

FPAnswerPtr ControlCenter::sendQuest(FPQuestPtr quest, int timeout)
{
	return gc_Actor.sendQuest(quest, timeout);
}
bool ControlCenter::sendQuest(FPQuestPtr quest, AnswerCallback* callback, int timeout)
{
	return gc_Actor.sendQuest(quest, callback, timeout);
}
bool ControlCenter::sendQuest(FPQuestPtr quest, std::function<void (FPAnswerPtr answer, int errorCode)> task, int timeout)
{
	return gc_Actor.sendQuest(quest, std::move(task), timeout);
}

int showUsage(const char* appName)
{
	cout<<"Usage:"<<endl;
	cout<<"\t"<<appName<<" endpoint"<<ExecutiveActor::customParamsUsage()<<endl;
	cout<<"\t"<<appName<<" host port"<<ExecutiveActor::customParamsUsage()<<endl;
	return -1;
}

int main(int argc, const char* argv[])
{
	ignoreSignals();
	CommandLineParser::init(argc, argv);

	if (!gc_Actor.init())
		return showUsage(argv[0]);

	gc_Actor.loop();

	return 0;
}