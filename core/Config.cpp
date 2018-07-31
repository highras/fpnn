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

static std::mutex configLock;
static bool systemVariablesConfigured(false);
static bool serverVariablesConfigured(false);

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
    if(Setting::setted(key)){
        n = Setting::getInt(key);
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

    _sName = Setting::getString("FPNN.server.name", "FPNN.TEST");

    std::string logEndpoint = Setting::getString("FPNN.server.log.endpoint", "std::cout");
    std::string logRoute = Setting::getString("FPNN.server.log.route", "FPNN.TEST");
    std::string logLevel = Setting::getString("FPNN.server.log.level", "DEBUG");
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

    serverVariablesConfigured = true;
}

void Config::initClientVaribles(){
	_log_client_quest = Setting::getBool("FPNN.client.quest.log", false);
	_log_client_answer = Setting::getBool("FPNN.client.answer.log", false);
	_log_client_slow = Setting::getInt("FPNN.client.slow.log", 0);
    _max_recv_package_length = Setting::getInt("FPNN.global.max.package.len", FPNN_DEFAULT_MAX_PACKAGE_LEN);
}

