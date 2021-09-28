#ifndef __APPLE__
	#include <sys/sysinfo.h>
#endif
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <fstream>
#include "FPLog.h"
#include "Setting.h"
#include "ServerInfo.h"
#include "StringUtil.h"
#include "MachineStatus.h"
#include "NetworkUtility.h"
#include "TCPEpollServer.h"
#include "UDPEpollServer.h"
#include "FPZKClient.h"

using namespace fpnn;

//======================================//
//-          FPZKLinearProxy           -//
//======================================//
TCPClientPtr FPZKClient::FPZKLinearProxy::getClient(int index)
{
	std::string& endpoint = _endpoints[index];

	auto iter = _clients.find(endpoint);
	if (iter != _clients.end())
	{
		TCPClientPtr client = iter->second;
		if (client->connected() || client->reconnect())
			return client;
		else
		{
			_clients.erase(endpoint);
			return nullptr;
		}
	}
	else
		return createTCPClient(endpoint, true);
}

TCPClientPtr FPZKClient::FPZKLinearProxy::getNextClient(int stopIndex)
{
	std::unique_lock<std::mutex> lck(_mutex);

	if (_endpoints.size() == 0)
	{
		LOG_FATAL("Don't configed any FPZK server's endpoint!");
		return nullptr;
	}

	_currClient = nullptr;
	{
		auto iter = _clients.find(_endpoints[_index]);
		if (iter != _clients.end())
		{
			iter->second->close();
			_clients.erase(iter);
		}
	}

	_index++;
	if (_index >= (int)_endpoints.size())
		_index = 0;

	for (int i = _index; i < (int)_endpoints.size(); i++)
	{
		TCPClientPtr client = getClient(i);
		if (client)
		{
			_index = i;
			_currClient = client;
			return client;
		}

		if (i == stopIndex)
		{
			_index = i;
			return nullptr;
		}
	}

	for (int i = 0; i < _index; i++)
	{
		TCPClientPtr client = getClient(i);
		if (client)
		{
			_index = i;
			_currClient = client;
			return client;
		}

		if (i == stopIndex)
		{
			_index = i;
			return nullptr;
		}
	}

	return nullptr;
}

TCPClientPtr FPZKClient::FPZKLinearProxy::getCurrClient(int& index, int& endpointCount)
{
	std::unique_lock<std::mutex> lck(_mutex);
	endpointCount = (int)_endpoints.size();
	index = _index;
	return _currClient;
}

FPAnswerPtr FPZKClient::FPZKLinearProxy::sendQuest(FPQuestPtr quest)
{
	int index, endpointCount;
	TCPClientPtr client = getCurrClient(index, endpointCount);
	if (client == nullptr)
		client = getNextClient(index);

	int count = 0;
	while (client)
	{
		FPAnswerPtr answer = client->sendQuest(quest);
		FPAReader ar(answer);
		if (ar.status() == 0)
			return answer;

		switch (ar.wantInt("code"))
		{
			case FPZKError::ProjectNotFound:
			case FPZKError::ProjectTokenNotMatched:
			case FPNN_EC_CORE_UNKNOWN_METHOD:
				return answer;

			default:
				client = getNextClient(index);

			count += 1;
			if ((count % endpointCount) == 0)
			{
				if (quest->method() != "subscribeServicesChange")
					return answer;

				usleep(200000);			//-- 200ms
				LOG_INFO("------- delay 200 ms for subscribeServicesChange, cycle count: %d", count);
			}
		}
	}

	return nullptr;
}

//======================================//
//-         Class FPZKClient           -//
//======================================//

FPZKClient::FPZKClient(const std::string& fpzkSrvList, const std::string& projectName, const std::string& projectToken):
	_projectName(projectName), _projectToken(projectToken), _online(true), _monitorDetail(false),
	_requireUnregistered(false), _requireSync(false), _supportSubscribe(true)
{
	prepareSelfServiceInfo();
	
	std::vector<std::string> fpzkEndpoints;
	StringUtil::split(fpzkSrvList, "\t ,", fpzkEndpoints);

	_fpzkSrvProxy.reset(new FPZKLinearProxy());
	_fpzkSrvProxy->updateEndpoints(fpzkEndpoints);

	_supportSubscribe = Setting::getBool("FPZK.client.subscribe.enable", true);
	if (_supportSubscribe)
	{
		TaskThreadPoolPtr questProcessPool(new TaskThreadPool());
		questProcessPool->init(1, 0, 1, 1, 200);
		_fpzkSrvProxy->setSharedQuestProcessThreadPool(questProcessPool);
		_questProcessor = std::make_shared<FPZKQuestProcessor>(this);
		_fpzkSrvProxy->setSharedQuestProcessor(_questProcessor);
	}

	_syncPerformanceInfo = Setting::getBool("FPZK.client.sync.syncPerformanceInfo", false);
	_outputClusterChangeInfo = Setting::getBool("FPZK.client.debugInfo.clusterChanged.enable", false);

	_startTime = slack_real_msec();
	_running = true;
	_syncThread = std::thread(&FPZKClient::syncFunc, this);
}

FPZKClient::~FPZKClient()
{
	_questProcessor->stop();
	
	_running = false;
	_syncThread.join();
}

void FPZKClient::prepareSelfServiceInfo()
{
	_externalVisible = Setting::getBool("FPZK.client.sync.externalVisible", true);
	_syncEndpointForPublic = Setting::getBool("FPZK.client.sync.syncEndpoint", true);

	_syncForPublic = Setting::getBool("FPZK.client.sync.syncPublicInfo", false);
	if (_externalVisible && !_syncEndpointForPublic)
		_syncForPublic = true;

	_port = Setting::getInt(std::vector<std::string>{
		"FPNN.server.tcp.ipv4.listening.port",
		"FPNN.server.ipv4.listening.port",
		"FPNN.server.listening.port"}, 0);
	_ipv4 = ServerInfo::getServerLocalIP4();
	_registeredEndpoint.append(_ipv4).append(":").append(std::to_string(_port));
	_cluster = Setting::getString("FPNN.server.cluster.name", "");

	_port6 = Setting::getInt(std::vector<std::string>{
		"FPNN.server.tcp.ipv6.listening.port",
		"FPNN.server.ipv6.listening.port",
		}, 0);

	_uport = Setting::getInt(std::vector<std::string>{
		"FPNN.server.udp.ipv4.listening.port",
		"FPNN.server.ipv4.listening.port",
		"FPNN.server.listening.port"}, 0);

	_uport6 = Setting::getInt(std::vector<std::string>{
		"FPNN.server.udp.ipv6.listening.port",
		"FPNN.server.ipv6.listening.port",
		}, 0);

	_sslport = 0;
	_sslport6 = 0;
	
	if (_syncForPublic)
	{
		_domain = ServerInfo::getServerHostName();
		_ipv4 = ServerInfo::getServerPublicIP4();

		_sslport = Setting::getInt("FPNN.server.tcp.ipv4.ssl.listening.port", 0);
		_sslport6 = Setting::getInt("FPNN.server.tcp.ipv6.ssl.listening.port", 0);

		{
			std::map<enum IPTypes, std::set<std::string>> ipDict;
			if (getIPs(ipDict))
			{
				std::set<std::string>& global = ipDict[IPv6_Global];
				if (global.size() > 0)
					_ipv6 = *(global.begin());
			}
			if (_ipv6.empty())
			{
				_ipv6 = ServerInfo::getServerPublicIP6();
				if (_ipv6 == "unknown")
					_ipv6.clear();
				else if (strncasecmp("64:ff9b::", _ipv6.c_str(), 9) == 0)
				{
					_port6 = _port;
					_uport6 = _uport;
					_sslport6 = _sslport;
				}
			}
		}
	}
}

//---------- [Begin] Copied & reconstituted from old fpzk client version, which write by biao.zhang. ---------//
class CPUUsage
{
	typedef unsigned long long TIC_t;

	int _hertz;
	int _ncpu;
	pid_t _pid;
	struct timeval _oldtv;
	TIC_t  _oldtics;

	static TIC_t getTics(pid_t pid)
	{
		std::stringstream msgstr;
		msgstr<<"/proc/"<<pid<<"/stat";
		std::ifstream fin(msgstr.str());
		if (fin.is_open()) {
			char line[1024];
			fin.getline(line, sizeof(line));
			std::string sLine(line);
			std::vector<std::string> items;
			StringUtil::split(sLine, " ", items);
			fin.close();
			return stoull(items[13]) + stoull(items[14]);		// utime + stime
		}
		return -1;			// cannot open the file
	}

public:
	CPUUsage()
	{
		_hertz = sysconf(_SC_CLK_TCK);
#ifdef __APPLE__
		_ncpu = (int)sysconf(_SC_NPROCESSORS_ONLN);
#else
		_ncpu = get_nprocs();
#endif
		_pid = getpid();
		gettimeofday(&_oldtv, NULL);
		_oldtics = getTics(_pid);
	}

	float getPerCPUUsage()
	{
		struct timeval tv;
		TIC_t  tics;

		// calc process per cpu usage, using an algorithm like top command
		tics = getTics(_pid);
		gettimeofday(&tv, NULL);
		float elapsed = (tv.tv_sec - _oldtv.tv_sec) + (float)(tv.tv_usec - _oldtv.tv_usec) / 1000000.0;
		float perCPUUsage = (tics - _oldtics) * 100.0f / ((float) _hertz * elapsed * _ncpu);
		_oldtics = tics;
		_oldtv = tv;

		return perCPUUsage;
	}
};
//---------- [End] Copied & reconstituted from old fpzk client version, which write by biao.zhang. ---------//

const FPZKClient::ServiceInfosPtr FPZKClient::checkCacheStatus(const std::string& serviceName)
{
	bool needResubscribe = _supportSubscribe && (_questProcessor->isSubscribing() == false);
	if (!needResubscribe)
	{
		bool newInterest = false;
		{
			std::lock_guard<std::mutex> lck(_mutex);

			auto it = _servicesMap.find(serviceName);
			if (it != _servicesMap.end())
				return it->second;

			if (_interestServices.find(serviceName) == _interestServices.end())
			{
				newInterest = true;
				_interestServices.insert(serviceName);
			}
		}

		std::set<std::string> interested{serviceName};
		if (newInterest && _supportSubscribe)
			subscribe(interested);
		else
			fetchInterestServices(interested);
	}
	else
	{
		std::set<std::string> interested;
		{
			std::lock_guard<std::mutex> lck(_mutex);

			if (_interestServices.find(serviceName) == _interestServices.end())
				_interestServices.insert(serviceName);

			interested = _interestServices;
		}

		subscribe(interested);
	}

	std::lock_guard<std::mutex> lck(_mutex);
	{
		auto it = _servicesMap.find(serviceName);
		if (it != _servicesMap.end())
			return it->second;
		else
			return nullptr;
	}
}

void FPZKClient::subscribe(const std::set<std::string>& serviceNames)
{
	if (_supportSubscribe == false || _running == false)
		return;

	FPQWriter qw(3, "subscribeServicesChange");
	qw.param("project", _projectName);
	qw.param("projectToken", _projectToken);
	qw.param("services", serviceNames);
	FPQuestPtr quest = qw.take();

	FPAnswerPtr answer = _fpzkSrvProxy->sendQuest(quest);
	if (answer == nullptr)
	{
		LOG_FATAL("[subscribeServicesChange] Not available FPZK Services!");
		return;
	}

	FPAReader ar(answer);
	if (ar.status())
	{
		int code = ar.getInt("code");
		std::string ex = ar.getString("ex");
		if (code == FPZKError::ProjectNotFound || code == FPZKError::ProjectTokenNotMatched) {
			LOG_FATAL("subscribeServicesChange failed. code: %d, ex: %s", code, ex.c_str());
		} else {
			LOG_ERROR("subscribeServicesChange failed. code: %d, ex: %s", code, ex.c_str());
		}
		return;
	}

	updateServicesMapCache(&ar);
}

void FPZKClient::adjustSelfPortInfo()
{
	if (_port == _uport)
	{
		if (TCPEpollServer::instance() && UDPEpollServer::instance() == nullptr)
			_uport = 0;
		else if (TCPEpollServer::instance() == nullptr && UDPEpollServer::instance())
			_port = 0;
	}

	if (_port6 == _uport6)
	{
		if (TCPEpollServer::instance() && UDPEpollServer::instance() == nullptr)
			_uport6 = 0;
		else if (TCPEpollServer::instance() == nullptr && UDPEpollServer::instance())
			_port6 = 0;
	}
}

bool FPZKClient::syncSelfStatus(float perCPUUsage)
{
	int connNum = 0;
	float perCPULoad = 0.;

	if (_syncPerformanceInfo)
	{
#ifdef __APPLE__
		int cpuCount = (int)sysconf(_SC_NPROCESSORS_ONLN);
#else
		int cpuCount = get_nprocs();
#endif
		connNum = MachineStatus::getConnectionCount();
		perCPULoad = MachineStatus::getCPULoad()/cpuCount;
	}

	FPQuestPtr quest;
	std::set<std::string> queriedServices;
	{
		std::lock_guard<std::mutex> lck(_mutex);

		adjustSelfPortInfo();

		if (_registeredName.length())
		{
			int totalCount = 23;
			if (!_syncForPublic)
				totalCount -= 5;
			if (!_syncPerformanceInfo)
				totalCount -= 3;

			FPQWriter qw(totalCount, "syncServerInfo");
			qw.param("project", _projectName);
			qw.param("projectToken", _projectToken);

			qw.param("serviceName", _registeredName);
			qw.param("cluster", _cluster);
			qw.param("srvVersion", _registeredVersion);
			qw.param("endpoint", _registeredEndpoint);
			
			if (_syncPerformanceInfo)
			{
				qw.param("connNum", connNum);
				qw.param("perCPULoad", perCPULoad);
				qw.param("perCPUUsage", perCPUUsage);
			}

			qw.param("online", _online);
			qw.param("startTime", _startTime);

			qw.param("port", _port);
			qw.param("port6", _port6);
			qw.param("uport", _uport);
			qw.param("uport6", _uport6);

			qw.param("externalVisible", _externalVisible);
			qw.param("publishEndpoint", _syncEndpointForPublic);

			qw.param("interests", _interestServices);

			if (_syncForPublic)
			{
				qw.param("domain", _domain);
				qw.param("ipv4", _ipv4);
				qw.param("ipv6", _ipv6);
				qw.param("sslport", _sslport);
				qw.param("sslport6", _sslport6);
			}

			quest = qw.take();
		}
		else
		{
			if (_interestServices.empty())
				return true;

			FPQWriter qw(3, "syncServerInfo");
			qw.param("project", _projectName);
			qw.param("projectToken", _projectToken);
			qw.param("interests", _interestServices);

			quest = qw.take();
		}

		queriedServices = _interestServices;
	}

	FPAnswerPtr answer = _fpzkSrvProxy->sendQuest(quest);
	if (answer == nullptr)
	{
		LOG_FATAL("[syncServerInfo] Not available FPZK Services!");
		return false;
	}

	FPAReader ar(answer);
	if (ar.status())
	{
		int code = ar.getInt("code");
		std::string ex = ar.getString("ex");
		if (code == FPZKError::ProjectNotFound || code == FPZKError::ProjectTokenNotMatched) {
			LOG_FATAL("syncServerInfo failed. code: %d, ex: %s", code, ex.c_str());
		} else {
			LOG_ERROR("syncServerInfo failed. code: %d, ex: %s", code, ex.c_str());
		}
		return false;
	}

	std::vector<std::string> services = ar.get("services", std::vector<std::string>());
	if (services.size() > 0)
	{
		//-- FPZKServer 3.0.2 and after
		for (auto& service: services)
			queriedServices.erase(service);

		std::vector<int64_t> revisions = ar.get("revisions", std::vector<int64_t>());
		std::vector<int64_t> clusterAlteredTimes = ar.get("clusterAlteredTimes", std::vector<int64_t>());

		updateChangedAndMonitoredServices(queriedServices, services, revisions, clusterAlteredTimes);
	}
	else
	{
		//-- FPZKServer 3.0.1 and before
		std::map<std::string, int64_t> rvMap = ar.get("revisionMap", std::map<std::string, int64_t>());
		
		for (auto& revpair: rvMap)
			queriedServices.erase(revpair.first);

		updateChangedAndMonitoredServices(queriedServices, rvMap);
	}

	return true;
}

void FPZKClient::updateChangedAndMonitoredServices(const std::set<std::string>& invalidServices, const std::map<std::string, int64_t>& rvMap)
{
	std::set<std::string> needUpdateServices;
	{
		std::lock_guard<std::mutex> lck(_mutex);

		for (auto& service: invalidServices)
			_servicesMap.erase(service);

		if (_monitorDetail == false)
		{
			for (auto& revpair: rvMap)
			{
				auto it = _servicesMap.find(revpair.first);
				if (it != _servicesMap.end())
				{
					if (it->second->revision != revpair.second)
						needUpdateServices.insert(revpair.first);
				}
				else
					needUpdateServices.insert(revpair.first);
			}

			needUpdateServices.insert(_detailServices.begin(), _detailServices.end());
		}
		else
			needUpdateServices = _interestServices;
	}

	for (auto& service: invalidServices)
	{
		LOG_INFO("Services %s cluster is empty in FPZK clinet cache!",  service.c_str());
	}

	if(!needUpdateServices.empty())
		fetchInterestServices(needUpdateServices);
}

void FPZKClient::updateChangedAndMonitoredServices(const std::set<std::string>& invalidServices,
	const std::vector<std::string>& services, const std::vector<int64_t>& revisions, const std::vector<int64_t>& clusterAlteredTimes)
{
	std::set<std::string> needUpdateServices;
	{
		std::lock_guard<std::mutex> lck(_mutex);

		for (auto& service: invalidServices)
			_servicesMap.erase(service);

		if (_monitorDetail == false)
		{
			for (size_t i = 0; i < services.size(); i++)
			{
				auto it = _servicesMap.find(services[i]);
				if (it != _servicesMap.end())
				{
					if (it->second->clusterAlteredMsec != clusterAlteredTimes[i] || it->second->revision != revisions[i])
						needUpdateServices.insert(services[i]);
				}
				else
					needUpdateServices.insert(services[i]);
			}

			needUpdateServices.insert(_detailServices.begin(), _detailServices.end());
		}
		else
			needUpdateServices = _interestServices;
	}

	for (auto& service: invalidServices)
	{
		LOG_INFO("Services %s cluster is empty in FPZK clinet cache!",  service.c_str());
	}

	if(!needUpdateServices.empty())
		fetchInterestServices(needUpdateServices);
}

void FPZKClient::resubscribe()
{
	if (_supportSubscribe == false || _running == false)
		return;

	std::set<std::string> interested;
	{
		std::lock_guard<std::mutex> lck(_mutex);
		interested = _interestServices;
	}
	if (interested.empty())
		return;

	subscribe(interested);
}

void FPZKClient::fetchInterestServices(const std::set<std::string>& serviceNames)
{
	if (_running == false)
		return;
	
	FPQWriter qw(3, "getServiceInfo");
	qw.param("project", _projectName);
	qw.param("projectToken", _projectToken);
	qw.param("services", serviceNames);
	FPQuestPtr quest = qw.take();

	FPAnswerPtr answer = _fpzkSrvProxy->sendQuest(quest);
	if (answer == nullptr)
	{
		LOG_FATAL("[getServiceInfo] Not available FPZK Services!");
		return;
	}

	FPAReader ar(answer);
	if (ar.status())
	{
		int code = ar.getInt("code");
		std::string ex = ar.getString("ex");
		if (code == FPZKError::ProjectNotFound || code == FPZKError::ProjectTokenNotMatched) {
			LOG_FATAL("getServiceInfo failed. code: %d, ex: %s", code, ex.c_str());
		} else {
			LOG_ERROR("getServiceInfo failed. code: %d, ex: %s", code, ex.c_str());
		}
		return;
	}

	updateServicesMapCache(&ar);
}

void FPZKClient::logClusterChange(const std::map<std::string, ServiceInfosPtr> &updatedServices, const std::vector<std::string> &invalidServices)
{
	std::string invalidated = StringUtil::join(invalidServices, ",");
	LOG_DEBUG("[Cluster Changed][Invalid Services] %s", invalidated.c_str());

	for (auto& updatedPair: updatedServices)
	{
		std::string info("service: ");
		info.append(updatedPair.first);
		info.append(", version: ").append(std::to_string(updatedPair.second->revision));
		info.append(", onlineCount: ").append(std::to_string(updatedPair.second->onlineCount));
		info.append(", memberCount: ").append(std::to_string(updatedPair.second->nodeMap.size()));

		int i = 0;
		info.append(", cluster: [");
		for (auto& nodePair: updatedPair.second->nodeMap)
		{
			if (i)
			{
				info.append(", ");
			}

			info.append(nodePair.first);
			i += 1;
		}
		info.append("]");

		LOG_DEBUG("[Cluster Changed][Updated Service] %s", info.c_str());
	}
}

void FPZKClient::updateServicesMapCache(FPReader* reader)
{
	std::vector<std::string> services = reader->want("services", std::vector<std::string>());
	std::vector<int64_t> revisions = reader->want("revisions", std::vector<int64_t>());
	std::vector<int64_t> clusterAlteredTimes = reader->get("clusterAlteredTimes", std::vector<int64_t>());
	std::vector<std::string> nodeInfoFields = reader->want("nodeInfoFields", std::vector<std::string>());
	std::vector<std::vector<std::vector<std::string>>> srvNodes = reader->want("srvNodes", std::vector<std::vector<std::vector<std::string>>>());
	std::vector<std::string> invalidServices = reader->get("invalidServices", std::vector<std::string>());

	bool epsExist = false;
	for (auto& str: nodeInfoFields)
	{
		if (str == "endpoint")
		{
			epsExist = true;
			break;
		}
	}
	if (!epsExist)
	{
		LOG_ERROR("Recrive invalidated services update without endpoint field!");
		return;
	}

	std::map<std::string, ServiceInfosPtr> updatedServices;
	for (int i = 0; i < (int)services.size(); i++)
	{
		const std::string& service = services[i];
		ServiceInfosPtr sip(new ServiceInfos());

		sip->revision = revisions[i];
		sip->updateMsec = slack_real_msec();
		
		if (clusterAlteredTimes.size())
			sip->clusterAlteredMsec = clusterAlteredTimes[i];
		else
			sip->clusterAlteredMsec = sip->updateMsec;		//-- compatible with old FPZK server.

		for(int j = 0; j < (int)srvNodes[i].size(); j++)
		{
			const std::vector<std::string>& nodeInfos = srvNodes[i][j];
			std::string endpoint;
			ServiceNode sn;

			/*
			  Fields：
				"endpoint", "region", "srvVersion", "registerTime", "lastMTime", "online",
				"connNum", "loadAvg", "cpuUsage",
				"ipv4", "ipv6", "domain", 
				"port", "port6", "sslport", "sslport6", "uport", "uport6"
			*/

			for (int k = 0; k < (int)nodeInfos.size(); k++)
			{
				if(nodeInfoFields[k] =="endpoint")
					endpoint = nodeInfos[k];
				else if(nodeInfoFields[k] =="region")
					sn.region = nodeInfos[k];
				else if(nodeInfoFields[k] =="srvVersion")
					sn.version = nodeInfos[k];

				else if(nodeInfoFields[k] =="registerTime")
					sn.registerTime = stoll(nodeInfos[k]);
				else if(nodeInfoFields[k] =="lastMTime")
					sn.activedTime = stoll(nodeInfos[k]);
				else if(nodeInfoFields[k] =="online")
					sn.online = (nodeInfos[k] == "true"? true : false);

				else if(nodeInfoFields[k] =="connNum")
					sn.connCount = stoi(nodeInfos[k]);
				else if(nodeInfoFields[k] =="loadAvg")
					sn.loadAvg = stof(nodeInfos[k]);
				else if(nodeInfoFields[k] =="cpuUsage")
					sn.CPULoad = stof(nodeInfos[k]);
				
				else if(nodeInfoFields[k] =="ipv4")
					sn.ipv4 = nodeInfos[k];
				else if(nodeInfoFields[k] =="ipv6")
					sn.ipv6 = nodeInfos[k];
				else if(nodeInfoFields[k] =="domain")
					sn.domain = nodeInfos[k];
				

				else if(nodeInfoFields[k] =="port")
					sn.port = stoi(nodeInfos[k]);
				else if(nodeInfoFields[k] =="port6")
					sn.port6 = stoi(nodeInfos[k]);
				
				else if(nodeInfoFields[k] =="sslport")
					sn.sslport = stoi(nodeInfos[k]);
				else if(nodeInfoFields[k] =="sslport6")
					sn.sslport6 = stoi(nodeInfos[k]);

				else if(nodeInfoFields[k] =="uport")
					sn.uport = stoi(nodeInfos[k]);
				else if(nodeInfoFields[k] =="uport6")
					sn.uport6 = stoi(nodeInfos[k]);
			}

			if (endpoint.length())
			{
				sip->nodeMap[endpoint] = sn;
				if (sn.online) sip->onlineCount += 1;
			}
		}

		updatedServices[service] = sip;
	}

	ServicesAlteredCallbackPtr alteredCb;
	std::map<std::string, ServiceInfosPtr> alteredServices;
	{
		std::lock_guard<std::mutex> lck(_mutex);
		{
			//-- prepare services altered notification.
			if (_serviceAlteredCallback)
			{
				alteredCb = _serviceAlteredCallback;

				for (auto& service: invalidServices)
					alteredServices[service] = std::make_shared<ServiceInfos>();

				for (auto& updatedPair: updatedServices)
				{
					auto it = _servicesMap.find(updatedPair.first);
					if (it == _servicesMap.end() || it->second->revision != updatedPair.second->revision)
						alteredServices[updatedPair.first] = updatedPair.second;
				}
			}

			//-- Normal update FPZK client's caches
			for (auto& service: invalidServices)
			{
				_servicesMap.erase(service);
				LOG_INFO("Service %s cluster is empty now!",  service.c_str());
			}

			for (auto& updatedPair: updatedServices)
				_servicesMap[updatedPair.first] = updatedPair.second;
		}
	}

	if (alteredCb && alteredServices.size())
	{
		ServicesAlteredCallbackTaskPtr t(new ServicesAlteredCallbackTask);

		t->_callback = alteredCb;
		t->_alteredServices.swap(alteredServices);
		ClientEngine::wakeUpAnswerCallbackThreadPool(t);
	}

	if (_outputClusterChangeInfo)
		logClusterChange(updatedServices, invalidServices);
}
void FPZKClient::unregisterSelf()
{
	FPQWriter qw(5, "unregisterService");
	qw.param("project", _projectName);
	qw.param("projectToken", _projectToken);
	{
		std::lock_guard<std::mutex> lck(_mutex);

		if (_registeredName.empty())
		{
			_requireUnregistered = false;
			return;
		}
		qw.param("serviceName", _registeredName);
		qw.param("endpoint", _registeredEndpoint);
	}
	qw.param("cluster", _cluster);

	FPQuestPtr quest = qw.take();

	FPAnswerPtr answer = _fpzkSrvProxy->sendQuest(quest);
	if (answer == nullptr)
	{
		LOG_FATAL("[unregisterService] Not available FPZK Services!");
		return;
	}

	FPAReader ar(answer);
	if (ar.status())
	{
		int code = ar.getInt("code");
		std::string ex = ar.getString("ex");
		if (code == FPZKError::ProjectNotFound || code == FPZKError::ProjectTokenNotMatched) {
			LOG_FATAL("unregisterService failed. code: %d, ex: %s", code, ex.c_str());
		} else {
			LOG_ERROR("unregisterService failed. code: %d, ex: %s", code, ex.c_str());
		}
		return;
	}

	std::lock_guard<std::mutex> lck(_mutex);
	_requireUnregistered = false;
	_registeredName.clear();
}

void FPZKClient::syncFunc()
{
	CPUUsage cpuUsage;

	while (_running)
	{
		int cyc = 10 * 2;		//- 2 seconds
		while (!_requireSync && _running && cyc--)
			usleep(100*1000);

		_requireSync = false;
		if (_requireUnregistered)
			unregisterSelf();

		if (!_running)
			return;

		float perCPUUsage = cpuUsage.getPerCPUUsage();
		syncSelfStatus(perCPUUsage);
	}
}

FPZKClientPtr FPZKClient::create(const std::string& fpzkSrvList, const std::string& projectName, const std::string& projectToken)
{
	std::string servList(fpzkSrvList), name(projectName), token(projectToken);
	if (servList == "") servList = Setting::getString("FPZK.client.fpzkserver_list");
	if (name == "") name = Setting::getString("FPZK.client.project_name");
	if (token == "") token = Setting::getString("FPZK.client.project_token");

	return FPZKClientPtr(new FPZKClient(servList, name, token));
}

bool FPZKClient::registerService(const std::string& serviceName, const std::string& cluster, const std::string& version, const std::string& endpoint, bool online)
{
	std::string registeredName = serviceName;
	if (registeredName.empty())
	{
		registeredName = Setting::getString("FPNN.server.name", "");
		if(registeredName.empty())
			return false;
	}

	if (endpoint.length())
	{
		std::string host;
		int port;

		if(!parseAddress(endpoint, host, port))
			return false;			
	}

	std::lock_guard<std::mutex> lck(_mutex);
	{
		_registeredName = registeredName;
		_registeredVersion = version;
		_online = online;

		if (!cluster.empty())
			_cluster = cluster;

		if (!endpoint.empty())
			_registeredEndpoint = endpoint;
	}
	
	_requireUnregistered = false;
	_requireSync = true;
	return true;
}

bool FPZKClient::registerServiceSync(const std::string& serviceName, const std::string& cluster, const std::string& version, const std::string& endpoint, bool online)
{
	if (!registerService(serviceName, cluster, version, endpoint, online))
		return false;

	CPUUsage cpuUsage;

	for (int i = 0; i < 3; i++)
	{
		float perCPUUsage = cpuUsage.getPerCPUUsage();
		if (syncSelfStatus(perCPUUsage))
			return true;

		usleep((i + 2) * 100 * 1000);
	}

	return false;
}

int64_t FPZKClient::getServiceRevision(const std::string& serviceName, const std::string& cluster)
{
	const ServiceInfosPtr sip = checkCacheStatus(clusteredServiceName(serviceName, cluster));
	if (sip)
		return sip->revision;
	else
		return 0;
}
int64_t FPZKClient::getServiceChangedMSec(const std::string& serviceName, const std::string& cluster)
{
	const ServiceInfosPtr sip = checkCacheStatus(clusteredServiceName(serviceName, cluster));
	if (sip)
		return sip->clusterAlteredMsec;
	else
		return 0;
}
const FPZKClient::ServiceInfosPtr FPZKClient::getServiceInfos(const std::string& serviceName, const std::string& cluster, const std::string& version, bool onlineOnly)
{
	const ServiceInfosPtr sip = checkCacheStatus(clusteredServiceName(serviceName, cluster));
	if (!sip)
		return nullptr;
	if ((sip->onlineCount == (int)(sip->nodeMap.size()) || onlineOnly == false) && version == "")
		return sip;

	ServiceInfosPtr revSip(new ServiceInfos());
	revSip->revision = sip->revision;
	revSip->onlineCount = sip->onlineCount;
	revSip->updateMsec = sip->updateMsec;
	revSip->clusterAlteredMsec = sip->clusterAlteredMsec;

	for (auto& nodePair: sip->nodeMap)
	{
		if (onlineOnly && nodePair.second.online == false)
			continue;

		if (version.length() && version != nodePair.second.version)
		{
			if (nodePair.second.online)
				revSip->onlineCount -= 1;

			continue;
		}

		revSip->nodeMap[nodePair.first] = nodePair.second;
	}
	
	return revSip;
}
std::vector<std::string> FPZKClient::getServiceEndpoints(const std::string& serviceName, const std::string& cluster, const std::string& version, bool onlineOnly)
{
	std::vector<std::string> revc;
	const ServiceInfosPtr sip = checkCacheStatus(clusteredServiceName(serviceName, cluster));
	if (sip)
	{
		for (auto& nodePair: sip->nodeMap)
		{
			if (onlineOnly && nodePair.second.online == false)
				continue;

			if (version.length() && version != nodePair.second.version)
				continue;

			revc.push_back(nodePair.first);
		}
	}
	return revc;
}

std::vector<std::string> FPZKClient::getServiceEndpointsWithoutMyself(const std::string& serviceName, const std::string& cluster, const std::string& version, bool onlineOnly) {
	std::vector<std::string> revc = getServiceEndpoints(serviceName, cluster, version, onlineOnly);
	for (size_t i = 0; i < revc.size(); ++i) {
		if (revc[i] == _registeredEndpoint) {
			revc.erase(revc.begin() + i);
			break;
		}
	}
	return revc;
}

std::string FPZKClient::getOldestServiceEndpoint(const std::string& serviceName, const std::string& cluster, const std::string& version, bool onlineOnly) {
	std::string endpoint;
	int32_t cur = slack_real_sec() + 1000;
	const ServiceInfosPtr sip = checkCacheStatus(clusteredServiceName(serviceName, cluster));
	if (sip)
	{
		for (auto& nodePair: sip->nodeMap)
		{
			if (onlineOnly && nodePair.second.online == false)
				continue;

			if (version.length() && version != nodePair.second.version)
				continue;

			if (nodePair.second.registerTime < cur) {
				cur = nodePair.second.registerTime;
				endpoint = nodePair.first;
			}
		}
	}
	return endpoint;
}
