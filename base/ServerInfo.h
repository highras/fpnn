#ifndef SERVERINFO_H_
#define SERVERINFO_H_
#include <string>
namespace fpnn {
class ServerInfo
{
public:
	static void getAllInfos();

	static const std::string& getServerHostName();
	static const std::string& getServerRegionName();
	static const std::string& getServerZoneName();
	static const std::string& getServerLocalIP4();
	static const std::string& getServerPublicIP4();
	static const std::string& getServerLocalIP6();
	static const std::string& getServerPublicIP6();

private:
	static std::string getAWSInfo(const std::string& url);
	static std::string getGCPInfo(const std::string& url);
	static bool getAZUREInfo(const std::string& url);
    static std::string ipv4Toipv6(const std::string& ipv4);

	static std::string _serverHostName;
	static std::string _serverRegionName;
	static std::string _serverZoneName;
	static std::string _serverLocalIP4;
	static std::string _serverPubliceIP4;
	static std::string _serverLocalIP6;
	static std::string _serverPubliceIP6;
};
}
#endif
