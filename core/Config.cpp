#include <sys/time.h>
#include <sys/resource.h>
#include "Setting.h"
#include "Config.h"
#include "unix_user.h"

using namespace fpnn;

//global
int Config::_max_recv_package_length(FPNN_DEFAULT_MAX_PACKAGE_LEN);

//server
bool Config::_log_server_quest(false);
bool Config::_log_server_answer(false);
int16_t Config::_log_server_slow(0);
bool Config::_server_http_supported(false);
bool Config::_server_http_close_after_answered(true);
bool Config::_server_stat(true);
bool Config::_server_preset_signals(true);
int32_t Config::_server_perfect_connections(FPNN_PERFECT_CONNECTIONS);
bool Config::_server_user_methods_force_encrypted(false);

//client config
bool Config::_log_client_quest(false);
bool Config::_log_client_answer(false);
int16_t Config::_log_client_slow(false);

void Config::initSystemVaribles(){
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
}

void Config::initServerVaribles(){
	_log_server_quest = Setting::getBool("FPNN.server.quest.log", false);
	_log_server_answer = Setting::getBool("FPNN.server.answer.log", false);
	_log_server_slow = Setting::getInt("FPNN.server.slow.log", 0);
	_server_http_supported = Setting::getBool("FPNN.server.http.supported", false);
    _server_http_close_after_answered = Setting::getBool("FPNN.server.http.closeAfterAnswered", true);
	_server_stat = Setting::getBool("FPNN.server.stat", true);
	_server_preset_signals = Setting::getBool("FPNN.server.preset.signal", true);
	_max_recv_package_length = Setting::getInt("FPNN.global.max.package.len", FPNN_DEFAULT_MAX_PACKAGE_LEN);
	_server_perfect_connections = Setting::getInt("FPNN.server.perfect.connections", FPNN_PERFECT_CONNECTIONS);
    _server_user_methods_force_encrypted = Setting::getBool("FPNN.server.security.forceEncrypt.userMethods", false);
}

void Config::initClientVaribles(){
	_log_client_quest = Setting::getBool("FPNN.client.quest.log", false);
	_log_client_answer = Setting::getBool("FPNN.client.answer.log", false);
	_log_client_slow = Setting::getInt("FPNN.client.slow.log", 0);
    _max_recv_package_length = Setting::getInt("FPNN.global.max.package.len", FPNN_DEFAULT_MAX_PACKAGE_LEN);
}

