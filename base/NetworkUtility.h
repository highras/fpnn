#ifndef FPNN_Network_Utility_H
#define FPNN_Network_Utility_H

#include <string>
#include <vector>
#include <map>
#include <set>

namespace fpnn {

bool nonblockedFd(int fd);
std::string IPV4ToString(uint32_t internalAddr);
bool checkIP4(const std::string& ip);

enum EndPointType{
	ENDPOINT_TYPE_IP4 = 1,
	ENDPOINT_TYPE_IP6 = 2,
	ENDPOINT_TYPE_DOMAIN = 3,
};
/*
supported endpoint
IPV4：
192.168.0.1:80
192.168.0.1#80
IPV6：
[2001:db8::1]:80
2001:db8::1:80
2001:db8::1#80
Domain:
localhost:80
www.baidu.com#80
*/
bool parseAddress(const std::string& address, std::string& host, int& port);
bool parseAddress(const std::string& address, std::string& host, int& port, EndPointType& eType);
bool getIPAddress(const std::string& hostname, std::string& IPAddress, EndPointType& eType);	//-- IPv4 is first.

enum IPTypes {
	IPv4_Public,
	IPv4_Local,
	IPv4_Loopback,
	IPv6_Global,
	IPv6_LinkLocal,
	IPv6_SiteLocal,
	IPv6_Multicast,
	IPv6_Loopback
};
bool getIPs(std::map<enum IPTypes, std::set<std::string>>& ipDict);

namespace NetworkUtil
{
	std::string getFirstIPAddress(enum IPTypes type);
	std::string getLocalIP4();
	std::string getPublicIP4();
	std::string getPeerName(int fd);
	std::string getSockName(int fd);
}

}

#endif
