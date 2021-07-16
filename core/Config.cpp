#include <sys/time.h>
#include <sys/resource.h>
#include "Setting.h"
#include "Config.h"
#include "unix_user.h"

using namespace fpnn;

//global
time_t Config::_started(0);
time_t Config::_compiled(0);
std::string Config::_sName;
std::string Config::_version(FPNN_SERVER_VERSION);
int Config::_max_recv_package_length(FPNN_DEFAULT_MAX_PACKAGE_LEN);

//server
bool Config::_log_server_quest(false);
bool Config::_log_server_answer(false);
int16_t Config::_log_server_slow(0);
bool Config::_logServerStatusInfos(false);
int Config::_logStatusIntervalSeconds(60);
bool Config::_server_http_supported(false);
bool Config::_server_http_close_after_answered(false);
bool Config::_server_stat(true);
bool Config::_server_preset_signals(true);
int32_t Config::_server_perfect_connections(FPNN_PERFECT_CONNECTIONS);
bool Config::_server_user_methods_force_encrypted(false);

//client config
bool Config::_log_client_quest(false);
bool Config::_log_client_answer(false);
int16_t Config::_log_client_slow(false);

bool Config::Client::KeepAlive::defaultEnable(false);
int Config::Client::KeepAlive::pingInterval(20*1000);
int Config::Client::KeepAlive::maxPingRetryCount(3);

//UDP
int Config::UDP::_LAN_MTU(FPNN_UDP_LAN_MTU);
int Config::UDP::_internet_MTU(FPNN_UDP_Internet_MTU);
uint32_t Config::UDP::_disordered_seq_tolerance(FPNN_UDP_DISORDERED_SEQ_TOLERANCE);
uint32_t Config::UDP::_disordered_seq_tolerance_before_first_package_received(FPNN_UDP_DISORDERED_SEQ_TOLERANCE_BEFORE_FIRST_PACKAGE);
uint64_t Config::UDP::_arq_reAck_interval_milliseconds(FPNN_UDP_ARQ_RE_ACK_INTERVAL_MSEC);
uint64_t Config::UDP::_arq_seqs_sync_interval_milliseconds(FPNN_UDP_ARQ_SYNC_INTERVAL_MSEC);
int Config::UDP::_heartbeat_interval_seconds(FPNN_UDP_HEARTBEAT_INTERVAL);
int Config::UDP::_max_cached_uncompleted_segment_package_count(FPNN_UDP_MAX_CACHED_UNCOMPLETED_SEGMENT_PACKAGES);
int Config::UDP::_max_cached_uncompleted_segment_seconds(FPNN_UDP_MAX_CACHED_UNCOMPLETED_SEGMENT_SECONDS);
int Config::UDP::_max_untransmitted_seconds(FPNN_DEFAULT_IDLE_TIMEOUT);
int64_t Config::UDP::_arq_urgnet_seqs_sync_interval_milliseconds(FPNN_UDP_ARQ_URGENT_SYNC_INTERVAL);
size_t Config::UDP::_arq_urgent_seqs_sync_triggered_threshold(FPNN_UDP_ARQ_URGENT_SYNC_THRESHOLD);
size_t Config::UDP::_unconfiremed_package_limitation(FPNN_UDP_ARQ_MAX_UNCONFIRMED_PACKAGES);
size_t Config::UDP::_max_package_sent_limitation_per_connection_second(FPNN_UDP_ARQ_MAX_PACKAGE_SENT_PER_CONNECTION_SECOND);
int Config::UDP::_max_resent_count_per_call(FPNN_UDP_ARQ_MAX_RESENT_COUNT_PER_SENDING_CALL);
int Config::UDP::_max_tolerated_milliseconds_before_first_package_received(FPNN_UDP_ARQ_MAX_TOLERATED_MSEC_BEFORE_FIRST_PACKAGE);
int Config::UDP::_max_tolerated_milliseconds_before_valid_package_received(FPNN_UDP_ARQ_MAX_TOLERATED_MSEC_BEFORE_VALID_PACKAGE);
int Config::UDP::_max_tolerated_count_before_valid_package_received(FPNN_UDP_ARQ_MAX_TOLERATED_COUNT_BEFORE_VALID_PACKAGE);
// int Config::UDP::_arq_una_include_rate(FPNN_UDP_ARQ_UNA_INCLUDE_RATE);

static std::mutex configLock;
static bool systemVariablesConfigured(false);
static bool serverVariablesConfigured(false);
static bool UDPGlobalVariablesConfigured(false);

void Config::initSystemVaribles(){

    std::unique_lock<std::mutex> lck(configLock);
    if (systemVariablesConfigured)
        return;

    struct rlimit rlim;
    intmax_t n;

    std::string key = "FPNN.server.rlimit.core.size";
    if(Setting::setted(key)){
        n = Setting::getInt(key);
        rlim.rlim_cur = n;  
        rlim.rlim_max = n;  
        setrlimit(RLIMIT_CORE, &rlim);
    }   

    key = "FPNN.server.rlimit.max.nofile";
    //if(Setting::setted(key)){
	{
        n = Setting::getInt(key, 100000);
        rlim.rlim_cur = n;
        rlim.rlim_max = n;
        setrlimit(RLIMIT_NOFILE, &rlim);
    } 

    key = "FPNN.server.rlimit.stack.size";
	if(Setting::setted(key)){
		n = Setting::getInt(key);
        rlim.rlim_cur = n;
        rlim.rlim_max = n;
        setrlimit(RLIMIT_STACK, &rlim);
    }   

    std::string user = Setting::getString("FPNN.server.user");
    std::string group = Setting::getString("FPNN.server.group");
    if (!user.empty() || !group.empty()){
        unix_seteusergroup(user.c_str(), group.c_str());
    }

    _started = time(NULL);

    std::string built = std::string("") + __DATE__ + " " + __TIME__;  
    struct tm t;
    strptime(built.c_str(), "%b %d %Y %H:%M:%S", &t);
    _compiled = mktime(&t);

    systemVariablesConfigured = true;
}

void Config::initServerVaribles(){

    std::unique_lock<std::mutex> lck(configLock);
    if (serverVariablesConfigured)
        return;

    _sName = Setting::getString("FPNN.server.name", "FPNN.Server");

    std::string logEndpoint = Setting::getString("FPNN.server.log.endpoint", "unix:///tmp/fplog.sock");
    std::string logLevel = Setting::getString("FPNN.server.log.level", "ERROR");
    std::string logRoute = Setting::getString("FPNN.server.log.route");
	if(!logRoute.size()) logRoute = _sName;
    FPLog::init(logEndpoint, logRoute, logLevel, _sName);

	_log_server_quest = Setting::getBool("FPNN.server.quest.log", false);
	_log_server_answer = Setting::getBool("FPNN.server.answer.log", false);
	_log_server_slow = Setting::getInt("FPNN.server.slow.log", 0);

    _logServerStatusInfos = Setting::getBool("FPNN.server.status.logStatusInfos", false);
    _logStatusIntervalSeconds = Setting::getInt("FPNN.server.status.logStatusInterval", 60);
    if (_logStatusIntervalSeconds < 1)
        _logServerStatusInfos = false;

    _server_http_supported = Setting::getBool("FPNN.server.http.supported", false);
    _server_http_close_after_answered = Setting::getBool("FPNN.server.http.closeAfterAnswered", false);
	_server_stat = Setting::getBool("FPNN.server.stat", true);
	_server_preset_signals = Setting::getBool("FPNN.server.preset.signal", true);
	_max_recv_package_length = Setting::getInt("FPNN.global.max.package.len", FPNN_DEFAULT_MAX_PACKAGE_LEN);
	_server_perfect_connections = Setting::getInt("FPNN.server.perfect.connections", FPNN_PERFECT_CONNECTIONS);
    _server_user_methods_force_encrypted = Setting::getBool("FPNN.server.security.forceEncrypt.userMethods", false);

    UDP::initUDPGlobalVaribles();

    serverVariablesConfigured = true;
}

void Config::initClientVaribles(){
	_log_client_quest = Setting::getBool("FPNN.client.quest.log", false);
	_log_client_answer = Setting::getBool("FPNN.client.answer.log", false);
	_log_client_slow = Setting::getInt("FPNN.client.slow.log", 0);
    _max_recv_package_length = Setting::getInt("FPNN.global.max.package.len", FPNN_DEFAULT_MAX_PACKAGE_LEN);

    Config::Client::KeepAlive::defaultEnable = Setting::getBool("FPNN.client.keepAlive.defaultEnable", false);
    Config::Client::KeepAlive::pingInterval = Setting::getInt("FPNN.client.keepAlive.pingInterval", 20) * 1000;
    Config::Client::KeepAlive::maxPingRetryCount = Setting::getInt("FPNN.client.keepAlive.maxPingRetryCount", 3);

    std::unique_lock<std::mutex> lck(configLock);
    UDP::initUDPGlobalVaribles();
}

void Config::UDP::initUDPGlobalVaribles()
{
    if (UDPGlobalVariablesConfigured)
        return;

    UDP::_LAN_MTU = Setting::getInt("FPNN.global.udp.mtu.lan", FPNN_UDP_LAN_MTU);
    UDP::_internet_MTU = Setting::getInt("FPNN.global.udp.mtu.internet", FPNN_UDP_Internet_MTU);
    UDP::_disordered_seq_tolerance = Setting::getInt("FPNN.global.udp.disorderedSeq.tolerance.afterFirstPackageReceived", FPNN_UDP_DISORDERED_SEQ_TOLERANCE);
    UDP::_disordered_seq_tolerance_before_first_package_received = Setting::getInt("FPNN.global.udp.disorderedSeq.tolerance.beforeFirstPackageReceived", FPNN_UDP_DISORDERED_SEQ_TOLERANCE_BEFORE_FIRST_PACKAGE);
    UDP::_arq_reAck_interval_milliseconds = Setting::getInt("FPNN.global.udp.arq.reAckInterval", FPNN_UDP_ARQ_RE_ACK_INTERVAL_MSEC);
    UDP::_arq_seqs_sync_interval_milliseconds = Setting::getInt("FPNN.global.udp.arq.syncInterval", FPNN_UDP_ARQ_SYNC_INTERVAL_MSEC);
    UDP::_heartbeat_interval_seconds = Setting::getInt("FPNN.global.udp.arq.heartbeatInterval", FPNN_UDP_HEARTBEAT_INTERVAL);

    UDP::_max_cached_uncompleted_segment_package_count = Setting::getInt("FPNN.global.udp.arq.uncompletedSegment.maxCacheCount", FPNN_UDP_MAX_CACHED_UNCOMPLETED_SEGMENT_PACKAGES);
    UDP::_max_cached_uncompleted_segment_seconds = Setting::getInt("FPNN.global.udp.arq.uncompletedSegment.maxCacheSeconds", FPNN_UDP_MAX_CACHED_UNCOMPLETED_SEGMENT_SECONDS);
    UDP::_max_untransmitted_seconds = Setting::getInt("FPNN.global.udp.arq.maxUntransmittedSeconds", FPNN_DEFAULT_IDLE_TIMEOUT);
    UDP::_arq_urgent_seqs_sync_triggered_threshold = Setting::getInt("FPNN.global.udp.arq.urgentSync.triggeredThreshold", FPNN_UDP_ARQ_URGENT_SYNC_THRESHOLD);
    UDP::_arq_urgnet_seqs_sync_interval_milliseconds = Setting::getInt("FPNN.global.udp.arq.urgentSync.minInterval", FPNN_UDP_ARQ_URGENT_SYNC_INTERVAL);

    UDP::_unconfiremed_package_limitation = Setting::getInt("FPNN.global.udp.arq.unconfiremedPackage.maxCount", FPNN_UDP_ARQ_MAX_UNCONFIRMED_PACKAGES);
    UDP::_max_package_sent_limitation_per_connection_second = Setting::getInt("FPNN.global.udp.arq.maxSendCountPerConnectionSecond", FPNN_UDP_ARQ_MAX_PACKAGE_SENT_PER_CONNECTION_SECOND);
    UDP::_max_resent_count_per_call = Setting::getInt("FPNN.global.udp.arq.resentPerSecondLimitation", FPNN_UDP_ARQ_MAX_RESENT_COUNT_PER_SENDING_CALL);

    UDP::_max_tolerated_milliseconds_before_first_package_received = Setting::getInt("FPNN.global.udp.arq.maxMsecToleranceBeforeFirstPackageReceived", FPNN_UDP_ARQ_MAX_TOLERATED_MSEC_BEFORE_FIRST_PACKAGE);
    UDP::_max_tolerated_milliseconds_before_valid_package_received = Setting::getInt("FPNN.global.udp.arq.maxMsecToleranceBeforeValidPackageReceived", FPNN_UDP_ARQ_MAX_TOLERATED_MSEC_BEFORE_VALID_PACKAGE);
    UDP::_max_tolerated_count_before_valid_package_received = Setting::getInt("FPNN.global.udp.arq.maxToleranceCountBeforeValidPackageReceived", FPNN_UDP_ARQ_MAX_TOLERATED_COUNT_BEFORE_VALID_PACKAGE);

//    UDP::_arq_una_include_rate = Setting::getInt("FPNN.global.udp.arq.UNA.interpolationRate", FPNN_UDP_ARQ_UNA_INCLUDE_RATE);

    if (UDP::_arq_urgent_seqs_sync_triggered_threshold > UDP::_unconfiremed_package_limitation)
        UDP::_arq_urgent_seqs_sync_triggered_threshold = UDP::_unconfiremed_package_limitation;

    UDPGlobalVariablesConfigured = true;
}

