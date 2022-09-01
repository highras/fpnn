#ifndef FPNN_Config_h
#define FPNN_Config_h
#include <string>
#include "Statistics.h"
#include "FPMessage.h"

namespace fpnn
{
#define FPNN_SERVER_VERSION "1.3.0"

//in second
#define FPNN_DEFAULT_QUEST_TIMEOUT (5)
#define FPNN_DEFAULT_IDLE_TIMEOUT (60)
#define FPNN_DEFAULT_MAX_PACKAGE_LEN (8*1024*1024)
#define FPNN_PERFECT_CONNECTIONS 100000

	//-- UDP max data length without IPv6 jumbogram.
#define FPNN_UDP_MAX_DATA_LENGTH (65507)
	//-- 576: Internet/X25, 1500: Ethernet
#define FPNN_UDP_Internet_MTU (576)
#define FPNN_UDP_LAN_MTU (1500)
	//-- In milliseconds
// #define FPNN_UDP_ARQ_UNA_INCLUDE_RATE (10)
#define FPNN_UDP_ARQ_RE_ACK_INTERVAL_MSEC (20)
#define FPNN_UDP_ARQ_SYNC_INTERVAL_MSEC (50)
#define FPNN_UDP_DISORDERED_SEQ_TOLERANCE (10000)
#define FPNN_UDP_DISORDERED_SEQ_TOLERANCE_BEFORE_FIRST_PACKAGE (500)
#define FPNN_UDP_HEARTBEAT_INTERVAL (20)
#define FPNN_UDP_MAX_CACHED_UNCOMPLETED_SEGMENT_PACKAGES (100)
#define FPNN_UDP_MAX_CACHED_UNCOMPLETED_SEGMENT_SECONDS (300)
#define FPNN_UDP_ARQ_URGENT_SYNC_THRESHOLD (280)
#define FPNN_UDP_ARQ_URGENT_SYNC_INTERVAL (20)
#define FPNN_UDP_ARQ_MAX_UNCONFIRMED_PACKAGES (320)
#define FPNN_UDP_ARQ_MAX_RESENT_COUNT_PER_SENDING_CALL (640)
#define FPNN_UDP_ARQ_MAX_PACKAGE_SENT_PER_CONNECTION_SECOND (5000)
#define FPNN_UDP_ARQ_MAX_TOLERATED_MSEC_BEFORE_FIRST_PACKAGE (3000)
#define FPNN_UDP_ARQ_MAX_TOLERATED_MSEC_BEFORE_VALID_PACKAGE (20000)
#define FPNN_UDP_ARQ_MAX_TOLERATED_COUNT_BEFORE_VALID_PACKAGE (1000)
#define FPNN_UDP_ARQ_ECDH_COPY_RETAINED_MSEC (10*1000)
	
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
		public:
			class TCP
			{
			public:
				static bool _server_user_methods_force_encrypted;
			};

		public:
			//client config
			static bool _log_client_quest;
			static bool _log_client_answer;
			static int16_t _log_client_slow;//no used, 

			class Client
			{
			public:
				class KeepAlive
				{
				public:
					static bool defaultEnable;
					static int pingInterval;			//-- In milliseconds
					static int maxPingRetryCount;
				};
			};

			class RawClient
			{
			public:
				static bool log_received_raw_data;
			};

		public:
			class UDP
			{
			public:
				static int _LAN_MTU;
				static int _internet_MTU;
				static int _heartbeat_interval_seconds;
				static uint32_t _disordered_seq_tolerance;
				static uint32_t _disordered_seq_tolerance_before_first_package_received;
				static uint64_t _arq_reAck_interval_milliseconds;
				static uint64_t _arq_seqs_sync_interval_milliseconds;
				static int _max_cached_uncompleted_segment_package_count;
				static int _max_cached_uncompleted_segment_seconds;
				static int _max_untransmitted_seconds;
				static size_t _arq_urgent_seqs_sync_triggered_threshold;
				static int64_t _arq_urgnet_seqs_sync_interval_milliseconds;
//				static int _arq_una_include_rate;

				static size_t _unconfiremed_package_limitation;
				static size_t _max_package_sent_limitation_per_connection_second;
				static int _max_resent_count_per_call;

				static int _max_tolerated_milliseconds_before_first_package_received;
				static int _max_tolerated_milliseconds_before_valid_package_received;
				static int _max_tolerated_count_before_valid_package_received;

				static int _ecdh_copy_retained_milliseconds;
				static bool _server_user_methods_force_encrypted;

				static bool _server_connection_reentry_replace_for_all_ip;
				static bool _server_connection_reentry_replace_for_private_ip;

				static void initUDPGlobalVaribles();
			};

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
