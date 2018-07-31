#ifndef FPNN_Server_Controller_h
#define FPNN_Server_Controller_h

#include <sstream>
#include <string>
#include <memory>
#include <mutex>

namespace fpnn
{
	class ServerController
	{
		static std::mutex _mutex;
		static std::string _infosHeader;
		static std::shared_ptr<std::string> _serverBaseInfos;
		static std::shared_ptr<std::string> _clientBaseInfos;

	private:
		static void serverInfos(std::stringstream& ss, bool show_interface_stat);
		static void clientInfos(std::stringstream& ss);
		static void appInfos(std::stringstream& ss);

	public:
		static bool tune(const std::string& key, std::string& value);
		static std::string infos();
		static void logStatus();
		static void installSignal();
		static void startTimeoutCheckThread();
		static void joinTimeoutCheckThread();
	};
}

#endif