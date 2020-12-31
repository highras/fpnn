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

	FPZK.client.sync.externalVisible = true
	是否同步到其他 region。即，本实例其他 region 是否可见。
	FPZK Server 低版本将忽略该参数。

	FPZK.client.sync.syncPublicInfo = false (默认)
	如果为 true，将额外汇报 ipv4、ipv6、domain、sslport、sslport6 (如果存在)

	FPZK.client.sync.syncPerformanceInfo = false (默认)
	如果为 true，将额外汇报当前机器连接总数，当前系统按 CPU 平均的负载和使用率。

	FPZK.client.sync.syncEndpoint = true
	配合 FPZK Server v3 使用。当 FPZK.client.sync.externalVisible = true 时，将在各个 region 之间同步服务
	所在 region 内部使用的内网 endpoint。如果内网没有打通，而是通过公网访问，请配置为 false。
	如果该参数为 false，且 FPZK.client.sync.externalVisible = true，则 FPZK.client.sync.syncPublicInfo 将被强制开启。
	FPZK Server 低版本将忽略该参数。

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
	const int ServerWarmUp = ErrorBase + 400;
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
		std::string region;

		int port;
		int port6;
		int sslport;
		int sslport6;
		int uport;
		int uport6;
		std::string domain;
		std::string ipv4;
		std::string ipv6;

		ServiceNode(): online(true), connCount(0), CPULoad(0.), loadAvg(0.),
			registerTime(0), activedTime(0), port(0), port6(0), sslport(0), sslport6(0), uport(0), uport6(0) {}
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
		std::atomic<int> _connectionCount;
		std::atomic<bool> _running;
		FPZKClient* _fpzk;

	public:
		FPAnswerPtr servicesChange(const FPReaderPtr args, const FPQuestPtr quest, const ConnectionInfo& ci)
		{
			if (!_running)
				return FPAWriter::emptyAnswer(quest);

			_fpzk->updateServicesMapCache(args.get());
			return FPAWriter::emptyAnswer(quest);
		}

		bool isSubscribing() { return _connectionCount > 0; }
		void stop() { _running = false; }
		void connectionClosed()
		{
			if (!_running)
				return;

			//-- atomic.fetch_sub() return the value before the operation.
			//-- Refer: http://www.cplusplus.com/reference/atomic/atomic/fetch_sub/

			if (_connectionCount.fetch_sub(1) == 1)
				_fpzk->resubscribe();
		}

		virtual void connected(const ConnectionInfo&) { _connectionCount++; }
		virtual void connectionWillClose(const ConnectionInfo&, bool) { connectionClosed(); }

		FPZKQuestProcessor(FPZKClient* fpzk): _connectionCount(0), _running(true), _fpzk(fpzk)
		{
			registerMethod("servicesChange", &FPZKQuestProcessor::servicesChange);
		}

		QuestProcessorClassBasicPublicFuncs
	};
	typedef std::shared_ptr<FPZKQuestProcessor> FPZKQuestProcessorPtr;

	//=================================================================//
	//- Service Altered Callbacks
	//=================================================================//
	class ServicesAlteredCallback
	{
	public:
		virtual ~ServicesAlteredCallback() {}
		virtual void serviceAltered(std::map<std::string, ServiceInfosPtr>& serviceInfos) = 0;
	};
	typedef std::shared_ptr<ServicesAlteredCallback> ServicesAlteredCallbackPtr;

	class FunctionServicesAlteredCallback: public ServicesAlteredCallback
	{
		std::function<void (std::map<std::string, ServiceInfosPtr>& serviceInfos)> _function;

	public:
		explicit FunctionServicesAlteredCallback(std::function<void (std::map<std::string, ServiceInfosPtr>& serviceInfos)> function): _function(function) {}
		virtual ~FunctionServicesAlteredCallback() {}
		virtual void serviceAltered(std::map<std::string, ServiceInfosPtr>& serviceInfos)
		{
			_function(serviceInfos);
		}
	};

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

	class ServicesAlteredCallbackTask: public ITaskThreadPool::ITask
	{
		friend class FPZKClient;

		std::map<std::string, ServiceInfosPtr> _alteredServices;
		ServicesAlteredCallbackPtr _callback;

	public:
		~ServicesAlteredCallbackTask() {}
		virtual void run()
		{
			_callback->serviceAltered(_alteredServices);
		}
	};
	typedef std::shared_ptr<ServicesAlteredCallbackTask> ServicesAlteredCallbackTaskPtr;

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
	int _sslport;
	int _sslport6;
	int _uport;
	int _uport6;
	std::thread _syncThread;
	bool _online;
	bool _monitorDetail;
	bool _syncForPublic;
	bool _requireUnregistered;
	std::atomic<bool> _requireSync;
	std::atomic<bool> _supportSubscribe;
	std::atomic<bool> _running;
	int64_t _startTime;
	bool _externalVisible;
	bool _syncPerformanceInfo;
	bool _syncEndpointForPublic;
	bool _outputClusterChangeInfo;
	ServicesAlteredCallbackPtr _serviceAlteredCallback;

	std::mutex _mutex;

	void logClusterChange(const std::map<std::string, ServiceInfosPtr> &updatedServices, const std::vector<std::string> &invalidServices);
	FPZKClient(const std::string& fpzkSrvList, const std::string& projectName, const std::string& projectToken);
	const ServiceInfosPtr checkCacheStatus(const std::string& serviceName);
	void subscribe(const std::set<std::string>& serviceNames);
	bool syncSelfStatus(float perCPUUsage);
	void prepareSelfServiceInfo();
	void adjustSelfPortInfo();
	void resubscribe();
	void updateChangedAndMonitoredServices(const std::set<std::string>& invalidServices, const std::map<std::string, int64_t>& rvMap);
	void updateChangedAndMonitoredServices(const std::set<std::string>& invalidServices, const std::vector<std::string>& services, const std::vector<int64_t>& revisions, const std::vector<int64_t>& clusterAlteredTimes);
	void fetchInterestServices(const std::set<std::string>& serviceNames);
	void updateServicesMapCache(FPReader* reader);
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

	bool registerService(const std::string& serviceName = "", const std::string& cluster = "", const std::string& version = "", const std::string& endpoint = "", bool online = true);
	bool registerServiceSync(const std::string& serviceName = "", const std::string& cluster = "", const std::string& version = "", const std::string& endpoint = "", bool online = true);
	int64_t getServiceRevision(const std::string& serviceName, const std::string& cluster = "");
	const ServiceInfosPtr getServiceInfos(const std::string& serviceName, const std::string& cluster = "", const std::string& version = "", bool onlineOnly = true);
	std::vector<std::string> getServiceEndpoints(const std::string& serviceName, const std::string& cluster = "", const std::string& version = "", bool onlineOnly = true);
	std::vector<std::string> getServiceEndpointsWithoutMyself(const std::string& serviceName, const std::string& cluster = "", const std::string& version = "", bool onlineOnly = true);
	int64_t getServiceChangedMSec(const std::string& serviceName, const std::string& cluster = "");
	std::string getOldestServiceEndpoint(const std::string& serviceName, const std::string& cluster = "", const std::string& version = "", bool onlineOnly = true);
	
	inline std::string registeredName() const { return _registeredName; }
	inline std::string registeredCluster() const { return _cluster; }
	inline std::string registeredEndpoint() const { return _registeredEndpoint; }

	inline void setServiceAlteredCallback(ServicesAlteredCallbackPtr callback)
	{
		std::lock_guard<std::mutex> lck(_mutex);
		_serviceAlteredCallback = callback;
	}

	inline void setServiceAlteredCallback(std::function<void (std::map<std::string, ServiceInfosPtr>& serviceInfos)> function)
	{
		ServicesAlteredCallbackPtr cb(new FunctionServicesAlteredCallback(std::move(function)));
		setServiceAlteredCallback(cb);
	}

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
	inline void monitorDetail(bool monitor, const std::string& service)
	{
		std::lock_guard<std::mutex> lck(_mutex);
		if (monitor)
			_detailServices.insert(service);
		else
			_detailServices.erase(service);
		
		_requireSync = true;
	}
	inline void monitorDetail(bool monitor, const std::set<std::string>& detailServices)
	{
		std::lock_guard<std::mutex> lck(_mutex);
		if (monitor)
			_detailServices.insert(detailServices.begin(), detailServices.end());
		else
			_detailServices.erase(detailServices.begin(), detailServices.end());

		_requireSync = true;
	}
	inline void monitorDetail(bool monitor, const std::vector<std::string>& detailServices)
	{
		std::lock_guard<std::mutex> lck(_mutex);
		if (monitor)
		{
			for (auto& service: detailServices)
				_detailServices.insert(service);
		}
		else
		{
			for (auto& service: detailServices)
				_detailServices.erase(service);
		}

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
		unregisterSelf();
	}
};

}

#endif
