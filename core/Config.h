#ifndef FPNN_Config_h
#define FPNN_Config_h
#include <string>
#include "Statistics.h"
#include "FPMessage.h"

namespace fpnn
{
#define FPNN_SERVER_VERSION "0.8.2"

//in second
#define FPNN_DEFAULT_QUEST_TIMEOUT (5)
#define FPNN_DEFAULT_IDLE_TIMEOUT (60)
#define FPNN_DEFAULT_MAX_PACKAGE_LEN (8*1024*1024)
#define FPNN_PERFECT_CONNECTIONS 100000
#define FPNN_UDP_MAX_DATA_LENGTH (65507)
//-- UDP max data length without IPv6 jumbogram.

	class Config
	{
		public:
			//global config
			static time_t _started;
			static time_t _compiled;
			static std::string _sName;
			static std::string _version;
			static int _max_recv_package_length;

		public:
			//server config
			static bool _log_server_quest;
			static bool _log_server_answer;
			static int16_t _log_server_slow;
			static bool _logServerStatusInfos;
			static int _logStatusIntervalSeconds;
			static bool _server_http_supported;
			static bool _server_http_close_after_answered;
			static bool _server_stat;
			static bool _server_preset_signals;
			static int32_t _server_perfect_connections;
			static bool _server_user_methods_force_encrypted;

		public:
			//client config
			static bool _log_client_quest;
			static bool _log_client_answer;
			static int16_t _log_client_slow;//no used, 

		public:
			static void initSystemVaribles();
			static void initServerVaribles();
			static void initClientVaribles();

			static inline void ServerQuestLog(const FPQuestPtr quest, const std::string& ip, uint16_t port){
				if(Config::_log_server_quest){
					UXLOG("SVR.QUEST","%s:%d Q=%s", ip.c_str(), port, quest->info().c_str());
				}
			}

			static inline void ServerAnswerAndSlowLog(const FPQuestPtr quest, const FPAnswerPtr answer, const std::string& ip, uint16_t port){
				if(Config::_log_server_answer){
					UXLOG("SVR.ANSWER","%s:%d A=%s", ip.c_str(), port, answer->info().c_str());
				} 

				if(Config::_log_server_slow > 0){
					int32_t cost = answer->timeCost();
					if(cost >= Config::_log_server_slow){
						UXLOG("SVR.SLOW","%s:%d C:%d Q=%s A=%s", ip.c_str(), port, cost, quest->info().c_str(), answer->info().c_str());
					}
				}

				if(Config::_server_stat){
					Statistics::stat(quest->method(), answer->status(), answer->timeCost());
				}
			}
			static inline void ClientQuestLog(const FPQuestPtr quest, const std::string& ip, uint16_t port){
				if(Config::_log_client_quest){
					UXLOG("CLI.QUEST","%s:%d Q=%s", ip.c_str(), port, quest->info().c_str());
				}
			}

			static inline void ClientAnswerLog(const FPAnswerPtr answer, const std::string& ip, uint16_t port){
				if(Config::_log_client_answer){
					UXLOG("CLI.ANSWER","%s:%d A=%s", ip.c_str(), port, answer->info().c_str());
				}
			}
	};
}

#endif
