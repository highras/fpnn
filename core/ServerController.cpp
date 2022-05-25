#include <sys/time.h>
#include <sys/resource.h>
#include <signal.h>
#include <thread>
#include "Setting.h"
#include "StringUtil.h"
#include "TimeUtil.h"
#include "FPLog.h"
#include "Config.h"
#include "Statistics.h"
#include "ClientEngine.h"
#include "TCPEpollServer.h"
#include "UDPEpollServer.h"
#include "ServerController.h"

using namespace fpnn;

void ServerUtils::adjustThreadPoolParams(int &minThread, int &maxThread, int constMin, int constMax)
{
	if (minThread < constMin)
		minThread = constMin;
	else if (minThread > constMax)
		minThread = constMax;

	if (maxThread < constMin)
		maxThread = constMin;
	else if (maxThread > constMax)
		maxThread = constMax;

	if (minThread > maxThread)
		minThread = maxThread;
}

std::mutex ServerController::_mutex;
std::string ServerController::_infosHeader;
std::shared_ptr<std::string> ServerController::_serverBaseInfos;
std::shared_ptr<std::string> ServerController::_clientBaseInfos;

void formatServerIPPort(std::stringstream& s, ServerPtr server)
{
	int port = server->port();
	if (port)
	{
		s<<"\"IPv4\":{";
		s<<"\"listenIP\":"<<StringUtil::escapseString(server->ip())<<",";
		s<<"\"listenPort\":"<<port<<"},";
	}

	int port6 = server->port6();
	if (port6)
	{
		s<<"\"IPv6\":{";
		s<<"\"listenIP\":"<<StringUtil::escapseString(server->ipv6())<<",";
		s<<"\"listenPort\":"<<port6<<"},";
	}
}

void formatSSLIPPort(std::stringstream& s, TCPServerPtr server)
{
	int sslPort = server->sslPort();
	if (sslPort)
	{
		s<<"\"IPv4(SSL)\":{";
		s<<"\"listenIP\":"<<StringUtil::escapseString(server->sslIP())<<",";
		s<<"\"listenPort\":"<<sslPort<<"},";
	}

	int sslPort6 = server->sslPort6();
	if (sslPort6)
	{
		s<<"\"IPv6(SSL)\":{";
		s<<"\"listenIP\":"<<StringUtil::escapseString(server->sslIP6())<<",";
		s<<"\"listenPort\":"<<sslPort6<<"},";
	}
}

time_t calculateCompiledTime(ServerPtr server)
{
	if (!server)
		return 0;

	IQuestProcessorPtr processor = server->getQuestProcessor();
	if (!processor)
		return 0;

	std::string orgDatetime;
	orgDatetime.append(processor->getCompiledDate()).append(" ").append(processor->getCompiledTime());
	
	struct tm t;
	strptime(orgDatetime.c_str(), "%b %d %Y %H:%M:%S", &t);
	return mktime(&t);
}

void ServerController::serverInfos(std::stringstream& ss, bool show_interface_stat)
{
	std::shared_ptr<std::string> serverBaseInfos;
	{
		std::unique_lock<std::mutex> lck(_mutex);
		serverBaseInfos = _serverBaseInfos;	
	}

	TCPServerPtr tcpServer = TCPEpollServer::instance();
	UDPServerPtr udpServer = UDPEpollServer::instance();

	if (serverBaseInfos == nullptr)
	{
		time_t compiledTime = calculateCompiledTime(tcpServer);
		if (!compiledTime)
			compiledTime = calculateCompiledTime(udpServer);

		std::stringstream s;
		s<<"{";

		//-- sBase
		s<<"\"base\":{";
		s<<"\"name\":"<<StringUtil::escapseString(Config::_sName)<<",";
		s<<"\"startTime\":"<<Config::_started<<",";
		s<<"\"startTimeStr\":"<<StringUtil::escapseString(TimeUtil::getDateTime(Config::_started))<<",";
		s<<"\"compileTime\":"<<compiledTime<<",";
		s<<"\"compileTimeStr\":"<<StringUtil::escapseString(TimeUtil::getDateTime(compiledTime))<<",";
		s<<"\"frameworkCompileTime\":"<<Config::_compiled<<",";
		s<<"\"frameworkCompileTimeStr\":"<<StringUtil::escapseString(TimeUtil::getDateTime(Config::_compiled))<<",";
		s<<"\"frameworkVersion\":"<<StringUtil::escapseString(Config::_version)<<",";/*record history of core*/
		s<<std::boolalpha<<"\"presetSignal\":"<<Config::_server_preset_signals<<",";
		s<<std::boolalpha<<"\"stat\":"<<Config::_server_stat<<"},"; //may be tuned

		//-- tcp server
		if (tcpServer)
		{
			s<<"\"tcp\":{";
			formatServerIPPort(s, tcpServer);
			formatSSLIPPort(s, tcpServer);
			s<<"\"listenBacklog\":"<<tcpServer->backlog()<<",",
			s<<"\"iobufferChunkSize\":"<<tcpServer->ioBufferChunkSize()<<",";
			s<<"\"maxEvent\":"<<tcpServer->maxEvent()<<",";
			s<<"\"maxConnection\":"<<tcpServer->maxConnectionLimitation()<<",";
			s<<"\"questTimeout\":"<<tcpServer->getQuestTimeout()<<",";
			s<<"\"idleTimeout\":"<<tcpServer->getIdleTimeout()<<",";
			s<<std::boolalpha<<"\"httpSupported\":"<<Config::_server_http_supported<<"},"; //may be tuned
		}

		//-- udp server
		if (udpServer)
		{
			s<<"\"udp\":{";
			formatServerIPPort(s, udpServer);
			s<<"\"MTU\":{";
			s<<"\"internet\":"<<Config::UDP::_internet_MTU<<",";
			s<<"\"LAN\":"<<Config::UDP::_LAN_MTU;
			s<<"},";
			s<<"\"ARQ\":{";

			s<<"\"heartbeatIntervalSeconds\":"<<Config::UDP::_heartbeat_interval_seconds<<",";
			s<<"\"disorderedSequenceTolerance\":{";
			s<<"\"afterFirstPackageReceived\":"<<Config::UDP::_disordered_seq_tolerance<<",";
			s<<"\"beforeFirstPackageReceived\":"<<Config::UDP::_disordered_seq_tolerance_before_first_package_received;
			s<<"},";
			s<<"\"reAckInterval(ms)\":"<<Config::UDP::_arq_reAck_interval_milliseconds<<",";
			s<<"\"syncInterval(ms)\":"<<Config::UDP::_arq_seqs_sync_interval_milliseconds<<",";
			s<<"\"maxUncompletedSegmentedPackageCount\":"<<Config::UDP::_max_cached_uncompleted_segment_package_count<<",";
			s<<"\"maxCachedSecondsForUncompletedSegments\":"<<Config::UDP::_max_cached_uncompleted_segment_seconds<<",";
			s<<"\"urgentSyncTriggeredThreshold\":"<<Config::UDP::_arq_urgent_seqs_sync_triggered_threshold<<",";
			s<<"\"urgentSyncInterval(ms)\":"<<Config::UDP::_arq_urgnet_seqs_sync_interval_milliseconds<<",";
//			s<<"\"UNAInterpolationRate\":"<<Config::UDP::_arq_una_include_rate<<",";
			s<<"\"maxUncomfiredPackageCount\":"<<Config::UDP::_unconfiremed_package_limitation<<",";
			s<<"\"maxResendCountPerCalling\":"<<Config::UDP::_max_resent_count_per_call<<",";
			s<<"\"maxWaitingTimeForFirstReliablePackage(ms)\":"<<Config::UDP::_max_tolerated_milliseconds_before_first_package_received<<",";
			s<<"\"maxWaitingTimeForValidReliablePackage(ms)\":"<<Config::UDP::_max_tolerated_milliseconds_before_valid_package_received<<",";
			s<<"\"maxToleratedBeforeValidReliablePackageReceived(ms)\":"<<Config::UDP::_max_tolerated_count_before_valid_package_received<<",";
			s<<"\"idleTimeout\":"<<Config::UDP::_max_untransmitted_seconds;

			s<<"},";
			s<<"\"untransmittedIdleSeconds\":"<<Config::UDP::_max_untransmitted_seconds<<",";
			s<<"\"perSecondPackageSendingLimitation\":"<<Config::UDP::_max_package_sent_limitation_per_connection_second;
			s<<"},";
		}

		//-- sProto
		s<<"\"proto\":{";
		s<<"\"msgpackVersion\":"<<StringUtil::escapseString(msgpack_version())<<",";
		s<<"\"rapidjsonVersion\":"<<StringUtil::escapseString(RAPIDJSON_VERSION_STRING)<<",";
		s<<"\"currentVersion\":"<<(int32_t)FPMessage::currentVersion()<<",";
		s<<"\"supportedVersion\":"<<(int32_t)FPMessage::supportedVersion()<<"},";

		//-- sSystem
		s<<"\"system\":{";
		struct rlimit r;
		getrlimit(RLIMIT_NOFILE,&r);
		s<<"\"noFile\":"<<r.rlim_cur<<",";
		getrlimit(RLIMIT_CORE,&r);
		s<<"\"coreSize\":"<<r.rlim_cur<<",";
		getrlimit(RLIMIT_STACK,&r);
		s<<"\"stackSize\":"<<r.rlim_cur<<"},";

		//-- sLog
		s<<"\"log\":{";
		s<<"\"level\":"<<StringUtil::escapseString(FPLog::getLevel())<<",";
		s<<std::boolalpha<<"\"quest\":"<<Config::_log_server_quest<<",";
		s<<std::boolalpha<<"\"answer\":"<<Config::_log_server_answer<<",";
		s<<"\"slow\":"<<Config::_log_server_slow<<",";
		s<<std::boolalpha<<"\"statusInfos\":"<<Config::_logServerStatusInfos<<",";
		s<<"\"statusInfosInterval\":"<<Config::_logStatusIntervalSeconds<<"},";
		

		std::string cache = s.str();

		std::unique_lock<std::mutex> lck(_mutex);
		_serverBaseInfos.reset(new std::string());
		_serverBaseInfos->swap(cache);
		serverBaseInfos = _serverBaseInfos;
	}

	ss<<(*serverBaseInfos);

	if (show_interface_stat)
		ss<<"\"stat\":"<<Statistics::str()<<",";

	ss<<"\"status\":{";
	
		if (tcpServer)
		{
			ss<<"\"tcp\":{";
			ss<<"\"currentConnections\":"<<tcpServer->currentConnections()<<"}";
		}

		if (udpServer)
		{
			if (tcpServer)
				ss<<",";

			ss<<"\"udp\":{";
			ss<<"\"currentSessions\":"<<udpServer->currentConnections()<<",";
			ss<<"\"currnetARQConnections\":"<<udpServer->validARQConnections()<<"}";
		}

	ss<<"},";

	if (tcpServer || udpServer)
	{
		ss<<"\"thread\":{";
		if (tcpServer)
		{
			ss<<"\"tcp\":{";
			ss<<"\"workThreadStatus\":"<<tcpServer->workerPoolStatus()<<",";
			ss<<"\"ioThreadStatus\":"<<GlobalIOPool::nakedInstance()->ioPoolStatus()<<",";
			ss<<"\"duplexThreadStatus\":"<<tcpServer->answerCallbackPoolStatus()<<"}";
		}

		if (tcpServer && udpServer)
			ss<<",";

		if (udpServer)
		{
			ss<<"\"udp\":{";
			ss<<"\"workThreadStatus\":"<<udpServer->workerPoolStatus()<<",";
			ss<<"\"ioThreadStatus\":"<<GlobalIOPool::nakedInstance()->ioPoolStatus()<<",";
			ss<<"\"duplexThreadStatus\":"<<udpServer->answerCallbackPoolStatus()<<"}";
		}

		ss<<"}";
	}

	ss<<"}";
}
void ServerController::clientInfos(std::stringstream& ss)
{
	std::shared_ptr<std::string> clientBaseInfos;
	{
		std::unique_lock<std::mutex> lck(_mutex);
		clientBaseInfos = _clientBaseInfos;	
	}
	
	if (clientBaseInfos == nullptr)
	{
		std::stringstream s;
		s<<"{";

		//-- cBase
		s<<"\"base\":{";
		s<<"\"questTimeout\":"<<(ClientEngine::created() ? ClientEngine::getQuestTimeout() : FPNN_DEFAULT_QUEST_TIMEOUT);
		s<<"},";

		//-- cLog
		s<<"\"log\":{";
		s<<std::boolalpha<<"\"quest\":"<<Config::_log_client_quest<<",";
		s<<std::boolalpha<<"\"answer\":"<<Config::_log_client_answer<<",";
		s<<"\"slow\":"<<Config::_log_client_slow<<"},";

		std::string cache = s.str();
		std::unique_lock<std::mutex> lck(_mutex);
		_clientBaseInfos.reset(new std::string());
		_clientBaseInfos->swap(cache);
		clientBaseInfos = _clientBaseInfos;
	}

	ss<<(*clientBaseInfos);

	ss<<"\"thread\":{";

	if (ClientEngine::created())
		ss<<"\"ioThreadStatus\":"<<GlobalIOPool::nakedInstance()->ioPoolStatus()<<",";
	else
		ss<<"\"ioThreadStatus\":{},";

	ss<<"\"workThreadStatus\":"<<ClientEngine::answerCallbackPoolStatus()<<",";
	ss<<"\"duplexThreadStatus\":"<<ClientEngine::questProcessPoolStatus()<<"}";

	ss<<"}";
}
void formatAppInfos(std::stringstream& ss, IQuestProcessorPtr processor, const char* type)
{
	try
	{
		ss<<processor->infos();
	}
	catch (const FpnnError& ex){
		LOG_ERROR("Error in %sinfos:(%d)%s", type, ex.code(), ex.what());
		ss<<"{}";
	}
	catch (...)
	{
		LOG_ERROR("Unknown Error when quest %sinfos", type);
		ss<<"{}";
	}
}
void ServerController::appInfos(std::stringstream& ss)
{
	TCPServerPtr tcpServer = TCPEpollServer::instance();
	UDPServerPtr udpServer = UDPEpollServer::instance();

	IQuestProcessorPtr tcpQuestProcessor = tcpServer ? tcpServer->getQuestProcessor() : nullptr;
	IQuestProcessorPtr udpQuestProcessor = udpServer ? udpServer->getQuestProcessor() : nullptr;

	if (tcpQuestProcessor && udpQuestProcessor)
	{
		if (tcpQuestProcessor.get() == udpQuestProcessor.get())
			formatAppInfos(ss, tcpQuestProcessor, "");
		else
		{
			ss<<"{\"tcp\":";
			formatAppInfos(ss, tcpQuestProcessor, "tcp ");
			ss<<",\"udp\":";
			formatAppInfos(ss, udpQuestProcessor, "udp ");
			ss<<"}";
		}
	}
	else if (tcpQuestProcessor)
		formatAppInfos(ss, tcpQuestProcessor, "");
	else if (udpQuestProcessor)
		formatAppInfos(ss, udpQuestProcessor, "");
	else
		ss<<"{}";
}
void ServerController::logStatus()
{
	{
		std::stringstream ss;
		serverInfos(ss, false);
		UXLOG("SVR.INFOS.BAISC", "%s", ss.str().c_str());
	}

	UXLOG("SVR.INFOS.STAT", "%s", Statistics::str().c_str());

	{
		std::stringstream ss;
		clientInfos(ss);
		UXLOG("SVR.INFOS.CLIENT", "%s", ss.str().c_str());
	}

	{
		std::stringstream ss;
		appInfos(ss);
		UXLOG("SVR.INFOS.APP", "%s", ss.str().c_str());
	}
}

std::string ServerController::infos()
{
	std::stringstream ss;
	{
		std::unique_lock<std::mutex> lck(_mutex);
		if (_infosHeader.empty())
		{
			std::stringstream s;
			s<<"{";
			s<<"\"FPNN.status\":{";
			s<<"\"server\":";
			_infosHeader = s.str();
		}
		ss<<_infosHeader;
	}

	serverInfos(ss, true);
	ss<<",";// end of server

	ss<<"\"client\":";
	clientInfos(ss);

	ss <<"},"; // end of fpnn.status

	ss<<"\"APP.status\":";
	appInfos(ss);

	ss<<"}"; //end of ALL

	return ss.str();
}

bool ServerController::tune(const std::string& key, std::string& value)
{
	bool tuned = true;

	struct rlimit rlim;
	intmax_t n = atoi((value).c_str());
	bool boolValue = (strcasecmp("true", value.c_str()) == 0);
	if (!boolValue && n > 0)
		boolValue = true;

	if(key == "server.core.size"){
		rlim.rlim_cur = n;  
		rlim.rlim_max = n;  
		setrlimit(RLIMIT_CORE, &rlim);
	}    
	else if(key == "server.no.file"){
		rlim.rlim_cur = n;  
		rlim.rlim_max = n;  
		setrlimit(RLIMIT_NOFILE, &rlim);
	}   
	else if(key == "server.log.level"){
		std::string level = value;
		FPLog::setLevel(level);
	}  
	else if(key == "server.quest.log"){
		Config::_log_server_quest = boolValue;
	}
	else if(key == "server.answer.log"){
		Config::_log_server_answer = boolValue;
	}
	else if(key == "server.slow.log"){
		Config::_log_server_slow = n;
	}
	else if(key == "client.quest.log"){
		Config::_log_client_quest = boolValue;
	}
	else if(key == "client.answer.log"){
		Config::_log_client_answer = boolValue;
	}
	else if(key == "client.slow.log"){
		Config::_log_client_slow = n;
	}
	else if(key == "server.http.supported"){
		Config::_server_http_supported = boolValue;
	}
	else if(key == "server.stat"){
		Config::_server_stat = boolValue;
	}
	else if(key == "server.status.logStatusInfos")
		Config::_logServerStatusInfos = boolValue;
	else if(key == "server.status.logStatusInterval")
		Config::_logStatusIntervalSeconds = n;
	else
		tuned = false;

	{
		std::unique_lock<std::mutex> lck(_mutex);
		_serverBaseInfos.reset();
		_clientBaseInfos.reset();
	}

	return tuned;
}

//------------------[ Server Stop Controller ]---------------------------//

static std::atomic<bool> local_installSignalHandler(false);
static std::atomic<bool> local_stopSignalTriggered(false);

void serverStopTCPServer()
{
	TCPServerPtr tcpServer = TCPEpollServer::instance();

	if (tcpServer)
		tcpServer->stop();
}

void serverStopUDPServer()
{
	UDPServerPtr udpServer = UDPEpollServer::instance();

	if (udpServer)
		udpServer->stop();
}

void stopSignalHandler(int sig)
{
	if (local_stopSignalTriggered.exchange(true))
		return;

	std::thread(&serverStopTCPServer).detach();
	std::thread(&serverStopUDPServer).detach();
}

void ServerController::installSignal()
{
	bool status = local_installSignalHandler.exchange(true);

	if(status || !Setting::getBool("FPNN.server.preset.signal", true))
		return;

	signal(SIGXFSZ, SIG_IGN);  
	signal(SIGPIPE, SIG_IGN);
	signal(SIGALRM, SIG_IGN);
	signal(SIGINT, &stopSignalHandler);
	signal(SIGTERM, &stopSignalHandler);
	signal(SIGQUIT, &stopSignalHandler);
	signal(SIGUSR1, &stopSignalHandler);
	signal(SIGUSR2, &stopSignalHandler);
}

//------------------[ Server Timeout Check & status log Controller ]---------------------------//

static std::atomic<int> local_serverInstanceCount(0);
static std::atomic<bool> local_timeoutCheckerRunning(false);
static std::thread local_timeoutCheckThread;

void timeoutCheckThread()
{
	int statusLogIntervalCount = 0;

	while (local_timeoutCheckerRunning)
	{
		int cyc = 100, udpPeriodCheckCyc = 5;
		while (local_timeoutCheckerRunning && cyc--)
		{
			if (--udpPeriodCheckCyc == 0)
			{
				UDPServerPtr udpServer = UDPEpollServer::instance();
				if (udpServer)
					udpServer->periodSendingCheck();

				udpPeriodCheckCyc = 5;
			}
			usleep(10000);
		}

		//-- TCP Server
		{
			TCPServerPtr tcpServer = TCPEpollServer::instance();
			if (tcpServer)
				tcpServer->checkTimeout();
		}

		//-- UDP Server
		{
			UDPServerPtr udpServer = UDPEpollServer::instance();
			if (udpServer)
				udpServer->checkTimeout();
		}

		//-- TCP Client Engine
		//{
		//	if (ClientEngine::created())
		//		ClientEngine::instance()->checkTimeout();
		//}

		//-- Status Log
		if (Config::_logServerStatusInfos && (fpLogLevel >= FP_LEVEL_WARN))
		{
			statusLogIntervalCount += 1;
			if (statusLogIntervalCount >= Config::_logStatusIntervalSeconds)
			{
				ServerController::logStatus();
				statusLogIntervalCount = 0;
			}
		}
	}
}

void ServerController::startTimeoutCheckThread()
{
	int count = local_serverInstanceCount++;
	if (count == 0)
	{
		local_timeoutCheckerRunning = true;
		local_timeoutCheckThread = std::thread(&timeoutCheckThread);
	}
}

void ServerController::joinTimeoutCheckThread()
{
	int count = --local_serverInstanceCount;
	if (count == 0)
	{
		local_timeoutCheckerRunning = false;
		local_timeoutCheckThread.join();
	}
}
