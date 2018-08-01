#include <sys/sysinfo.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <fstream>
#include "FPLog.h"
#include "Setting.h"
#include "ServerInfo.h"
#include "StringUtil.h"
#include "NetworkUtility.h"
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
	//_clients.erase(_endpoints[_index]);

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
			case FPZKError::ProjectPermissionDenied:
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
				LOG_INFO("------- delay 200 ms for subscribeServicesChange, count: %d", count);
			}
		}
	}

	return nullptr;
}

//======================================//
//-         Class FPZKClient           -//
//======================================//
//std::mutex FPZKClient::_mutex;
//FPZKClientPtr FPZKClient::_instance = nullptr;

FPZKClient::FPZKClient(const std::string& fpzkSrvList, const std::string& projectName, const std::string& projectToken):
	_projectName(projectName), _projectToken(projectToken), _online(true), _monitorDetail(false),
	_supportUnregister(true), _requireUnregistered(false), _requireSync(false), _supportSubscribe(true)
{
	_syncForPublic = Setting::getBool("FPZK.client.sync.syncPublicInfo", false);

	_domain = ServerInfo::getServerDomain();
	_port = Setting::getInt(std::vector<std::string>{
		"FPNN.server.tcp.ipv4.listening.port",
		"FPNN.server.ipv4.listening.port",
		"FPNN.server.listening.port"}, 0);
	_ipv4 = ServerInfo::getServerLocalIP4();
	_registeredEndpoint.append(_ipv4).append(":").append(std::to_string(_port));
	_cluster = Setting::getString("FPNN.server.cluster.name", "");

	_port6 = 0;
	if (_syncForPublic)
	{
		_ipv4 = ServerInfo::getServerPublicIP4();
		_port6 = Setting::getInt(std::vector<std::string>{
			"FPNN.server.tcp.ipv6.listening.port",
			"FPNN.server.ipv6.listening.port",
			}, 0);
		{
			std::map<enum IPTypes, std::set<std::string>> ipDict;
			if (getIPs(ipDict))
			{
				std::set<std::string>& global = ipDict[IPv6_Global];
				if (global.size() > 0)
					_ipv6 = *(global.begin());
			}
			if (_ipv6.empty())
				_ipv6 = ServerInfo::getServerPublicIP6();
		}
	}

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

	_outputClusterChangeInfo = Setting::getBool("FPZK.client.debugInfo.clusterChanged.enable", false);

	_startTime = slack_real_msec();
	_running = true;
	_syncThread = std::thread(&FPZKClient::syncFunc, this);

	//-- for resource holder
	//_engine = ClientEngine::instance();
	//_heldLogger = FPLog::instance();
}

FPZKClient::~FPZKClient()
{
	_questProcessor->stop();
	
	_running = false;
	_syncThread.join();
}

//---------- [Begin] Copied & reconstituted from old fpzk client version, which write by biao.zhang. ---------//
/*
 * 取整个系统中正在使用的TCP连接数(只包含IPv4的，不包含IPv6的)，
 * 取一个进程对应的TCP连接数效率太低，只好用全局连接数代替
 */
int getConnNum()
{
	std::ifstream fin("/proc/net/sockstat");
	if (fin.is_open()) {
		char line[1024];
		while(fin.getline(line, sizeof(line))){
			if (strncmp("TCP:", line, 4))
				continue;

			std::string sLine(line);
			std::vector<std::string> items;
			StringUtil::split(sLine, " ", items);
			if(items.size() > 2)
			{
				fin.close();
				return stoi(items[2]);			// number of inused TCP connections
			}
		}
		fin.close();
	}
	return -1;
}

float getCPULoad()
{
	std::stringstream msgstr;
	std::ifstream fin("/proc/loadavg");
	if (fin.is_open()) {
		char line[1024];
		fin.getline(line, sizeof(line));
		std::string sLine(line);
		std::vector<std::string> items;
		StringUtil::split(sLine, " ", items);
		fin.close();
		try {
			float res = stof(items[0]);		// load average within 1 miniute
			return res;
		} catch(std::exception& e) {
			LOG_ERROR("failed to convert string to float, the string: %s", items[0].c_str());
			return -1;
		}
	}
	return -1;			// cannot open the file
}

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
		_ncpu = get_nprocs();
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
	if (newInterest)
	{
		subscribe(interested);
		if (_supportSubscribe == false)
			fetchInterestServices(interested);
	}
	else
		fetchInterestServices(interested);

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
		if (code == FPNN_EC_CORE_UNKNOWN_METHOD)
		{
			_supportSubscribe = false;
			return;
		}

		std::string ex = ar.getString("ex");
		std::string raiser = ar.getString("raiser");
		if (code == FPZKError::ProjectNotFound || code == FPZKError::ProjectTokenNotMatched) {
			LOG_FATAL("subscribeServicesChange failed. code: %d, ex: %s, raiser: %s", code, ex.c_str(), raiser.c_str());
		} else {
			LOG_ERROR("subscribeServicesChange failed. code: %d, ex: %s, raiser: %s", code, ex.c_str(), raiser.c_str());
		}
		return;
	}

	updateServicesMapCache(&ar);
}

bool FPZKClient::syncSelfStatus(float perCPUUsage)
{
	int cpuCount = get_nprocs();
	int connNum = getConnNum();
	float perCPULoad = getCPULoad()/cpuCount;

	FPQuestPtr quest;
	bool onlineStatus = true;
	std::set<std::string> queriedServices;
	{
		std::lock_guard<std::mutex> lck(_mutex);
		if (_registeredName.length())
		{
			int addedInfoCount = _syncForPublic ? 5 : 0;

			FPQWriter qw(12 + addedInfoCount, "syncServerInfo");
			qw.param("project", _projectName);
			qw.param("projectToken", _projectToken);

			qw.param("serviceName", _registeredName);
			qw.param("srvVersion", _registeredVersion);
			qw.param("endpoint", _registeredEndpoint);
			qw.param("cluster", _cluster);

			qw.param("connNum", connNum);
			qw.param("perCPULoad", perCPULoad);
			qw.param("perCPUUsage", perCPUUsage);

			qw.param("interests", _interestServices);
			qw.param("online", _online);

			qw.param("startTime", _startTime);

			if (_syncForPublic)
			{
				qw.param("port", _port);
				qw.param("port6", _port6);
				qw.param("domain", _domain);
				qw.param("ipv4", _ipv4);
				qw.param("ipv6", _ipv6);
			}

			quest = qw.take();
			onlineStatus = _online;
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
		std::string raiser = ar.getString("raiser");
		if (code == FPZKError::ProjectNotFound || code == FPZKError::ProjectTokenNotMatched) {
			LOG_FATAL("syncServerInfo failed. code: %d, ex: %s, raiser: %s", code, ex.c_str(), raiser.c_str());
		} else {
			LOG_ERROR("syncServerInfo failed. code: %d, ex: %s, raiser: %s", code, ex.c_str(), raiser.c_str());
		}
		return false;
	}

	std::map<std::string, int64_t> rvMap = ar.get("revisionMap", std::map<std::string, int64_t>());
	std::set<std::string> needUpdateServices;
	{
		std::lock_guard<std::mutex> lck(_mutex);
		if (_requireUnregistered && !_supportUnregister && !onlineStatus)
		{
			_requireUnregistered = false;
			_registeredName.clear();
		}

		if (_monitorDetail == false || _detailServices.size())
		{
			for (auto& revpair: rvMap)
			{
				queriedServices.erase(revpair.first);
				auto it = _servicesMap.find(revpair.first);
				if (it != _servicesMap.end())
				{
					if (it->second->revision != revpair.second)
						needUpdateServices.insert(revpair.first);
				}
				else
					needUpdateServices.insert(revpair.first);
			}

			if (_monitorDetail)
				needUpdateServices.insert(_detailServices.begin(), _detailServices.end());
		}
		else
		{
			for (auto& revpair: rvMap)
				queriedServices.erase(revpair.first);

			needUpdateServices = _interestServices;
		}

		for (auto& service: queriedServices)
		{
			_servicesMap.erase(service);
			LOG_INFO("Services %s cluster is empty in FPZK clinet cache!",  service.c_str());
		}
	}

	if(!needUpdateServices.empty())
		fetchInterestServices(needUpdateServices);

	return true;
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
		std::string raiser = ar.getString("raiser");
		if (code == FPZKError::ProjectNotFound || code == FPZKError::ProjectTokenNotMatched) {
			LOG_FATAL("getServiceInfo failed. code: %d, ex: %s, raiser: %s", code, ex.c_str(), raiser.c_str());
		} else {
			LOG_ERROR("getServiceInfo failed. code: %d, ex: %s, raiser: %s", code, ex.c_str(), raiser.c_str());
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

			for (int k = 0; k < (int)nodeInfos.size(); k++)
			{
				if(nodeInfoFields[k] =="endpoint")
					endpoint = nodeInfos[k];
				else if(nodeInfoFields[k] =="srvVersion")
					sn.version = nodeInfos[k];
				else if(nodeInfoFields[k] =="connNum")
					sn.connCount = stoi(nodeInfos[k]);
				else if(nodeInfoFields[k] =="loadAvg")
					sn.loadAvg = stof(nodeInfos[k]);
				else if(nodeInfoFields[k] =="cpuUsage")
					sn.CPULoad = stof(nodeInfos[k]);
				else if(nodeInfoFields[k] =="registerTime")
					sn.registerTime = stoll(nodeInfos[k]);
				else if(nodeInfoFields[k] =="lastMTime")
					sn.activedTime = stoll(nodeInfos[k]);
				else if(nodeInfoFields[k] =="online")
					sn.online = (nodeInfos[k] == "true"? true : false);

				else if(nodeInfoFields[k] =="port")
					sn.port = stoi(nodeInfos[k]);
				else if(nodeInfoFields[k] =="port6")
					sn.port6 = stoi(nodeInfos[k]);
				else if(nodeInfoFields[k] =="domain")
					sn.domain = nodeInfos[k];
				else if(nodeInfoFields[k] =="ipv4")
					sn.ipv4 = nodeInfos[k];
				else if(nodeInfoFields[k] =="ipv6")
					sn.ipv6 = nodeInfos[k];
			}

			if (endpoint.length())
			{
				sip->nodeMap[endpoint] = sn;
				if (sn.online) sip->onlineCount += 1;
			}
		}

		updatedServices[service] = sip;
	}

	{
		std::lock_guard<std::mutex> lck(_mutex);
		{
			for (auto& service: invalidServices)
			{
				_servicesMap.erase(service);
				LOG_INFO("Service %s cluster is empty now!",  service.c_str());
			}

			for (auto& updatedPair: updatedServices)
				_servicesMap[updatedPair.first] = updatedPair.second;
		}
	}

	if (_outputClusterChangeInfo)
		logClusterChange(updatedServices, invalidServices);
}
void FPZKClient::doUnregisterService()
{
	if (_supportUnregister)
		unregisterSelf();

	if (!_supportUnregister)
		syncSelfStatus(0);
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
		if (code == FPNN_EC_CORE_UNKNOWN_METHOD)
		{
			_supportUnregister = false;
			return;
		}

		std::string ex = ar.getString("ex");
		std::string raiser = ar.getString("raiser");
		if (code == FPZKError::ProjectNotFound || code == FPZKError::ProjectTokenNotMatched) {
			LOG_FATAL("unregisterService failed. code: %d, ex: %s, raiser: %s", code, ex.c_str(), raiser.c_str());
		} else {
			LOG_ERROR("unregisterService failed. code: %d, ex: %s, raiser: %s", code, ex.c_str(), raiser.c_str());
		}
		return;
	}

	_requireUnregistered = false;
	_registeredName.clear();
}
void FPZKClient::syncFunc()
{
	CPUUsage cpuUsage;

	while (_running)
	{
		int cyc = 5 * 2;		//- 2 seconds
		while (!_requireSync && _running && cyc--)
			usleep(200*1000);

		_requireSync = false;
		if (_requireUnregistered)
			doUnregisterService();

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

bool FPZKClient::registerService(const std::string& serviceName, const std::string& version, const std::string& endpoint, bool online)
{
	std::string registeredName = serviceName;
	if(registeredName.empty())
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

		if (!endpoint.empty())
			_registeredEndpoint = endpoint;
	}
	
	_requireUnregistered = false;
	_requireSync = true;
	return true;
}

bool FPZKClient::registerServiceSync(const std::string& serviceName, const std::string& version, const std::string& endpoint, bool online)
{
	if (!registerService(serviceName, version, endpoint, online))
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
			continue;

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

std::vector<std::string> FPZKClient::getServiceEndpointsWithoutMyself(const std::string& version, bool onlineOnly){
	std::vector<std::string> revc = getServiceEndpoints(_registeredName, _cluster, version, onlineOnly);
	for(size_t i = 0; i < revc.size(); ++i){
		if(revc[i] == _registeredEndpoint){
			revc.erase(revc.begin() + i);
			break;
		}
	}
	return revc;
}

std::string FPZKClient::getOldestServiceEndpoint(const std::string& version, bool onlineOnly){
	std::string endpoint;
	int32_t cur = slack_real_sec() + 1000;
	const ServiceInfosPtr sip = checkCacheStatus(clusteredServiceName(_registeredName, _cluster));
	if (sip)
	{
		for (auto& nodePair: sip->nodeMap)
		{
			if (onlineOnly && nodePair.second.online == false)
				continue;

			if (version.length() && version != nodePair.second.version)
				continue;

			if(nodePair.second.registerTime < cur){
				cur = nodePair.second.registerTime;
				endpoint = nodePair.first;
			}
		}
	}
	return endpoint;
}
