#ifndef FPZK_Client_H
#define FPZK_Client_H

/*
===================
	特殊注意事项
===================
* 以下所有时间相关参数，请注意时间同步服务的精度。然后再对接收到的参数做出修正。
* 以下所有时间相关参数，请注意网络波动导致的时间同步服务异常，导致的次生异常。并对接收到的参数，做出修正。

===================
	配置文件参数
===================
	配置文件默认参数：
	如果 create 函数 fpzkSrvList 参数为空，将默认读取参数：FPZK.client.fpzkserver_list
	如果 create 函数 projectName 参数为空，将默认读取参数：FPZK.client.project_name
	如果 create 函数 projectToken 参数为空，将默认读取参数：FPZK.client.project_token

	其他默认参数：
	FPNN.server.cluster.name
	服务器集群分组名称。若无，留为空。

	FPZK.client.subscribe.enable = true (默认)
	如果 FPZK.client.subscribe.enable 为 true，支持服务器变动实时通知，但 FPZKClient 会启动两个线程。
	如果 FPZK.client.subscribe.enable 为 false，2秒与 FPZK Server 同步一次，但 FPZKClient 只会启动一个线程。

	FPZK.client.sync.syncPublicInfo = false (默认)
	如果为 true，将额外汇报 domain、ipv4、ipv6、port、port6 (如果存在)

	FPZK.client.debugInfo.clusterChanged.enable = false
	如果为 true，将输出集群变动的 debug 信息。
*/

#include <atomic>
#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <map>
#include <set>
#include <thread>
#include "TaskThreadPool.h"
#include "TCPClient.h"
#include "TCPProxyCore.hpp"

namespace fpnn {

class FPZKClient;
typedef std::shared_ptr<FPZKClient> FPZKClientPtr;

namespace FPZKError
{
	const int NoError = 0;
	const int ErrorBase = 30000;
	const int ProjectNotFound = ErrorBase + 301;
	const int ProjectTokenNotMatched = ErrorBase + 302;
	const int ProjectPermissionDenied = ErrorBase + 303;
	const int ServerWarmUp = ErrorBase + 400;		//-- Deprecate. Keep for compatible with the old FPZK Server.
}

class FPZKClient
{
public:
	struct ServiceNode
	{
		bool online;
		int connCount;
		float CPULoad;
		float loadAvg;  //-- per CPU Usage
		int64_t registerTime;	//-- in seconds
		int64_t activedTime;	//-- in seconds
		std::string version;

		int port;
		int port6;
		std::string domain;
		std::string ipv4;
		std::string ipv6;

		ServiceNode(): online(true), connCount(0), CPULoad(0.), loadAvg(0.),
			registerTime(0), activedTime(0), port(0), port6(0) {}
	};

	struct ServiceInfos
	{
		int onlineCount;
		int64_t revision;
		int64_t updateMsec;
		int64_t clusterAlteredMsec;
		std::map<std::string, ServiceNode> nodeMap;

		ServiceInfos(): onlineCount(0), revision(0), updateMsec(0), clusterAlteredMsec(0) {}
	};
	typedef std::shared_ptr<ServiceInfos> ServiceInfosPtr;

	class FPZKQuestProcessor: public IQuestProcessor
	{
		QuestProcessorClassPrivateFields(FPZKQuestProcessor)
		std::atomic<bool> _running;
		FPZKClient* _fpzk;

	public:
		FPAnswerPtr servicesChange(const FPReaderPtr args, const FPQuestPtr quest, const ConnectionInfo& ci)
		{
			if (!_running)
				return FPAWriter::emptyAnswer(quest);

			//-- LOG_INFO("------- servicesChange called ----------");
			_fpzk->updateServicesMapCache(args.get());
			return FPAWriter::emptyAnswer(quest);
		}

		void stop() { _running = false; }
		void connectionClosed()
		{
			if (!_running)
				return;

			//-- LOG_INFO("------- connection closed ----------");
			_fpzk->resubscribe();
		}

		//virtual void connectionClose(const ConnectionInfo&) { connectionClosed(); }
		//virtual void connectionErrorAndWillBeClosed(const ConnectionInfo&) { connectionClosed(); }
		virtual void connectionWillClose(const ConnectionInfo&, bool) { connectionClosed(); }

		FPZKQuestProcessor(FPZKClient* fpzk): _running(true), _fpzk(fpzk)
		{
			registerMethod("servicesChange", &FPZKQuestProcessor::servicesChange);
		}

		QuestProcessorClassBasicPublicFuncs
	};
	typedef std::shared_ptr<FPZKQuestProcessor> FPZKQuestProcessorPtr;

private:
	class FPZKLinearProxy: public TCPProxyCore
	{
		int _index;
		TCPClientPtr _currClient;
		
		TCPClientPtr getClient(int index);
		TCPClientPtr getNextClient(int stopIndex);
		TCPClientPtr getCurrClient(int& index, int& endpointCount);

	public:
		//-- If questTimeoutSeconds less then zero, mean using global settings.
		FPZKLinearProxy(int64_t questTimeoutSeconds = -1): TCPProxyCore(questTimeoutSeconds), _index(0), _currClient(nullptr)
		{
		}
		FPAnswerPtr sendQuest(FPQuestPtr quest);
	};
	typedef std::shared_ptr<FPZKLinearProxy> FPZKLinearProxyPtr;

private:
	FPZKLinearProxyPtr _fpzkSrvProxy;
	FPZKQuestProcessorPtr _questProcessor;
	std::set<std::string> _detailServices;
	std::set<std::string> _interestServices;
	std::map<std::string, ServiceInfosPtr> _servicesMap;
	std::string _projectName;
	std::string _projectToken;
	std::string _registeredEndpoint;
	std::string _registeredName;
	std::string _registeredVersion;
	std::string _domain;
	std::string _ipv4;
	std::string _ipv6;
	std::string _cluster;
	int _port;
	int _port6;
	std::thread _syncThread;
	bool _online;
	bool _monitorDetail;
	bool _syncForPublic;
	bool _supportUnregister;
	bool _requireUnregistered;
	std::atomic<bool> _requireSync;
	std::atomic<bool> _supportSubscribe;
	std::atomic<bool> _running;
	int64_t _startTime;

	bool _outputClusterChangeInfo;

	std::mutex _mutex;

	void logClusterChange(const std::map<std::string, ServiceInfosPtr> &updatedServices, const std::vector<std::string> &invalidServices);
	FPZKClient(const std::string& fpzkSrvList, const std::string& projectName, const std::string& projectToken);
	const ServiceInfosPtr checkCacheStatus(const std::string& serviceName);
	void subscribe(const std::set<std::string>& serviceNames);
	bool syncSelfStatus(float perCPUUsage);
	void resubscribe();
	void fetchInterestServices(const std::set<std::string>& serviceNames);
	void updateServicesMapCache(FPReader* reader);
	void doUnregisterService();
	void unregisterSelf();
	void syncFunc();

	inline std::string clusteredServiceName(const std::string& serviceName, const std::string& clusterName)
	{
		if (clusterName.empty())
			return serviceName;
		else
			return std::string(serviceName).append("@").append(clusterName);
	}

public:
	static FPZKClientPtr create(const std::string& fpzkSrvList = "", const std::string& projectName = "", const std::string& projectToken = "");
	~FPZKClient();

	bool registerService(const std::string& serviceName = "", const std::string& version = "", const std::string& endpoint = "", bool online = true);
	bool registerServiceSync(const std::string& serviceName = "", const std::string& version = "", const std::string& endpoint = "", bool online = true);
	int64_t getServiceRevision(const std::string& serviceName, const std::string& cluster = "");
	const ServiceInfosPtr getServiceInfos(const std::string& serviceName, const std::string& cluster = "", const std::string& version = "", bool onlineOnly = true);
	std::vector<std::string> getServiceEndpoints(const std::string& serviceName, const std::string& cluster = "", const std::string& version = "", bool onlineOnly = true);
	std::vector<std::string> getServiceEndpointsWithoutMyself(const std::string& version = "", bool onlineOnly = true);
	int64_t getServiceChangedMSec(const std::string& serviceName, const std::string& cluster = "");
	std::string getOldestServiceEndpoint(const std::string& version = "", bool onlineOnly = true);
	
	inline std::string registeredName() const { return _registeredName; }
	inline std::string registeredCluster() const { return _cluster; }
	inline std::string registeredEndpoint() const { return _registeredEndpoint; }

	inline void setOnline(bool online)
	{
		std::lock_guard<std::mutex> lck(_mutex);
		_online = online;
		_requireSync = true;
	}
	inline void monitorDetail(bool monitor)
	{
		std::lock_guard<std::mutex> lck(_mutex);
		_monitorDetail = monitor;
		_requireSync = true;
	}
	inline void monitorDetail(bool monitor, const std::string& service)	//-- 暂不支持 清除监控。等有需求再说
	{
		std::lock_guard<std::mutex> lck(_mutex);
		_detailServices.insert(service);
		_monitorDetail = monitor;
		_requireSync = true;
	}
	inline void monitorDetail(bool monitor, const std::set<std::string>& detailServices)	//-- 暂不支持 清除监控。等有需求再说
	{
		std::lock_guard<std::mutex> lck(_mutex);
		_detailServices.insert(detailServices.begin(), detailServices.end());
		_monitorDetail = monitor;
		_requireSync = true;
	}
	inline void monitorDetail(bool monitor, const std::vector<std::string>& detailServices)	//-- 暂不支持 清除监控。等有需求再说
	{
		std::lock_guard<std::mutex> lck(_mutex);
		_detailServices.insert(detailServices.begin(), detailServices.end());
		_monitorDetail = monitor;
		_requireSync = true;
	}
	inline void unregisterService()
	{
		std::lock_guard<std::mutex> lck(_mutex);
		_online = false;
		_requireUnregistered = true;
		_requireSync = true;
	}
	inline void unregisterServiceSync()
	{
		{
			std::lock_guard<std::mutex> lck(_mutex);
			_online = false;
		}

		doUnregisterService();
	}
};

}

#endif
