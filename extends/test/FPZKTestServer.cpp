#include "TCPEpollServer.h"
#include "ignoreSignals.h"
#include "IQuestProcessor.h"
#include "FPZKClient.h"

using namespace std;
using namespace fpnn;

class ServicesAlteredCallback: public FPZKClient::ServicesAlteredCallback
{
public:
	virtual ~ServicesAlteredCallback() {}
	virtual void serviceAltered(const std::map<std::string, FPZKClient::ServiceInfosPtr>& serviceInfos,
		const std::vector<std::string>& invalidServices)
	{
		cout<<"[Service Altered Object Callabck] "<<serviceInfos.size()<<" updated, "<<invalidServices.size()<<" invalid."<<endl;
		for (auto& pp: serviceInfos)
		{
			cout<<"[Updated Service]: "<<pp.first<<", revision: "<<pp.second->revision<<", current count: "<<pp.second->nodeMap.size()<<endl;
			for (auto& pp2: pp.second->nodeMap)
				cout<<"    ep: "<<pp2.first<<endl;
		}

		for (auto& serviceName: invalidServices)
			cout<<"[invalid Service]: "<<serviceName<<endl;
	}
};

class ExtraCommitCallback: public FPZKClient::ExtraDataCommitCallback
{
public:
	virtual ~ExtraCommitCallback() {}
	virtual std::shared_ptr<std::string> extraDataCommit()
	{
		std::shared_ptr<std::string> extra(new std::string("Current Timestamp: "));

		extra->append(std::to_string(slack_real_sec()));

		return extra;
	}
};

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
			infoList.push_back(std::string("tcpCount:").append(std::to_string(nodePair.second.tcpCount)));
			infoList.push_back(std::string("udpCount:").append(std::to_string(nodePair.second.udpCount)));
			infoList.push_back(std::string("CPULoad:").append(std::to_string(nodePair.second.CPULoad)));
			infoList.push_back(std::string("loadAvg:").append(std::to_string(nodePair.second.loadAvg)));
			infoList.push_back(std::string("activedTime:").append(std::to_string(nodePair.second.activedTime)));
			infoList.push_back(std::string("registerTime:").append(std::to_string(nodePair.second.registerTime)));
			infoList.push_back(std::string("version:").append(nodePair.second.version));
			
			if (nodePair.second.GPUStatus)
			{
				for (auto& pp: *(nodePair.second.GPUStatus))
					infoList.push_back(std::string("[GPU] idx: ").append(std::to_string(pp.index)).
						append(", usage: ").append(std::to_string(pp.usage)).append("%, memory: ").
						append(std::to_string(pp.memory.usage)).append("%, ").
						append(std::to_string(pp.memory.used)).append("/").
						append(std::to_string(pp.memory.total)));
			}

			if (nodePair.second.extra)
				infoList.push_back(std::string("extra:[").append(*(nodePair.second.extra)).append("]"));
		}

		return FPAWriter(5, quest)("revision", sip->revision)("onlineCount", sip->onlineCount)
			("clusterAlteredMsec", sip->clusterAlteredMsec)("updateMsec", sip->updateMsec)("nodes", infos);
	}
	QuestProcessor()
	{
		registerMethod("show", &QuestProcessor::show);

		_fpzk = FPZKClient::create();	//-- Using default action to fetch parameters value
		_fpzk->registerService();		//-- Using default action to fetch parameters value

		_attentionServer = Setting::getString("FPZK.client.attentionServer");
		bool monitorDetail = Setting::getBool("FPZK.client.attentionServer.monitorDetail", false);
		_fpzk->monitorDetail(monitorDetail);

		if (Setting::getBool("FPZK.clinet.serviceAlteredCallback.functional", false))
		{
			_fpzk->setServiceAlteredCallback([](const std::map<std::string, FPZKClient::ServiceInfosPtr>& serviceInfos,
				const std::vector<std::string>& invalidServices){

					cout<<"[Service Altered Functional Callabck] "<<serviceInfos.size()<<" updated, "<<invalidServices.size()<<" invalid."<<endl;
					for (auto& pp: serviceInfos)
					{
						cout<<"[Updated Service]: "<<pp.first<<", revision: "<<pp.second->revision<<", current count: "<<pp.second->nodeMap.size()<<endl;
						for (auto& pp2: pp.second->nodeMap)
							cout<<"    ep: "<<pp2.first<<endl;
					}

					for (auto& serviceName: invalidServices)
						cout<<"[invalid Service]: "<<serviceName<<endl;
			});
		}
		if (Setting::getBool("FPZK.clinet.serviceAlteredCallback.object", false))
		{
			FPZKClient::ServicesAlteredCallbackPtr cb = std::make_shared<ServicesAlteredCallback>();
			_fpzk->setServiceAlteredCallback(cb);
		}

		if (Setting::getBool("FPZK.clinet.extraCommitCallback.functional", false))
		{
			_fpzk->setExtraDataCommitCallback([]()->std::shared_ptr<std::string>{
				std::shared_ptr<std::string> extra(new std::string("Current Timestamp: "));

				extra->append(std::to_string(slack_real_sec()));

				return extra;
			});
		}

		if (Setting::getBool("FPZK.clinet.extraCommitCallback.object", false))
		{
			FPZKClient::ExtraDataCommitCallbackPtr cb = std::make_shared<ExtraCommitCallback>();
			_fpzk->setExtraDataCommitCallback(cb);
		}
		
	}
	virtual void serverStopped()
	{
		_fpzk->unregisterService();
	}

	QuestProcessorClassBasicPublicFuncs
};

#ifdef SUPPORT_CUDA
#include "nvml.h"
#endif

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

#ifdef SUPPORT_CUDA
	nvmlInit();
#endif

	ServerPtr server = TCPEpollServer::create();
	server->setQuestProcessor(std::make_shared<QuestProcessor>());
	server->startup();
	server->run();

#ifdef SUPPORT_CUDA
	nvmlShutdown();
#endif

	return 0;
}
