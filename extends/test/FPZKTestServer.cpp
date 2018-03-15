#include "TCPEpollServer.h"
#include "ignoreSignals.h"
#include "IQuestProcessor.h"
#include "FPZKClient.h"

using namespace std;
using namespace fpnn;

class QuestProcessor: public IQuestProcessor
{
	FPZKClientPtr _fpzk;
	std::string _attentionServer;
	QuestProcessorClassPrivateFields(QuestProcessor)

public:
	FPAnswerPtr show(const FPReaderPtr args, const FPQuestPtr quest, const ConnectionInfo& ci)
	{
		cout<<"quest data: "<<(quest->json())<<endl;
		if (_attentionServer.empty())
			return FPAWriter(1, quest)("servers", "no attention servers.");

		const FPZKClient::ServiceInfosPtr sip = _fpzk->getServiceInfos(_attentionServer);
		if (!sip)
			return FPAWriter(1, quest)("servers", "Attention servers is empty.");

		std::map<std::string, std::list<std::string>> infos;
		for (auto& nodePair: sip->nodeMap)
		{
			std::list<std::string> &infoList = infos[nodePair.first];
			infoList.push_back(std::string("online:").append(nodePair.second.online?"online":"offline"));
			infoList.push_back(std::string("connCount:").append(std::to_string(nodePair.second.connCount)));
			infoList.push_back(std::string("CPULoad:").append(std::to_string(nodePair.second.CPULoad)));
			infoList.push_back(std::string("loadAvg:").append(std::to_string(nodePair.second.loadAvg)));
			infoList.push_back(std::string("activedTime:").append(std::to_string(nodePair.second.activedTime)));
			infoList.push_back(std::string("registerTime:").append(std::to_string(nodePair.second.registerTime)));
			infoList.push_back(std::string("version:").append(nodePair.second.version));
		}

		return FPAWriter(5, quest)("revision", sip->revision)("onlineCount", sip->onlineCount)
			("clusterAlteredMsec", sip->clusterAlteredMsec)("updateMsec", sip->updateMsec)("nodes", infos);
	}
	QuestProcessor()
	{
		_fpzk = FPZKClient::create();	//-- Using default action to fetch parameters value
		_fpzk->registerService();		//-- Using default action to fetch parameters value

		_attentionServer = Setting::getString("FPZK.client.attentionServer");
		bool monitorDetail = Setting::getBool("FPZK.client.attentionServer.monitorDetail", false);
		_fpzk->monitorDetail(monitorDetail);

		registerMethod("show", &QuestProcessor::show);
	}
	virtual void serverStopped()
	{
		_fpzk->unregisterService();
	}

	QuestProcessorClassBasicPublicFuncs
};

int main(int argc, char* argv[])
{
	if (argc != 2){
		std::cout<<"Usage: "<<argv[0]<<" config"<<std::endl;
		return 0;
	}
	if(!Setting::load(argv[1])){
		std::cout<<"Config file error:"<< argv[1]<<std::endl;
		return 1;
	}

	ServerPtr server = TCPEpollServer::create();
	server->setQuestProcessor(std::make_shared<QuestProcessor>());
	server->startup();
	server->run();
	return 0;
}
