#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>
#include "StringUtil.h"
#include "NetworkUtility.h"

using namespace fpnn;

bool fpnn::nonblockedFd(int fd)
{
	int flags = fcntl(fd, F_GETFL);
	if (flags < 0)
		return false;

	if (-1 == fcntl(fd, F_SETFL, flags | O_NONBLOCK))
		return false;

	return true;
}

const char* IPV4DigitTable[] = {	//-- gen by shell script.
	"0","1","2","3","4","5","6","7","8","9","10","11","12","13","14","15","16","17","18","19","20",
	"21","22","23","24","25","26","27","28","29","30","31","32","33","34","35","36","37","38","39","40",
	"41","42","43","44","45","46","47","48","49","50","51","52","53","54","55","56","57","58","59","60",
	"61","62","63","64","65","66","67","68","69","70","71","72","73","74","75","76","77","78","79","80",
	"81","82","83","84","85","86","87","88","89","90","91","92","93","94","95","96","97","98","99","100",
	"101","102","103","104","105","106","107","108","109","110","111","112","113","114","115","116","117","118","119","120",
	"121","122","123","124","125","126","127","128","129","130","131","132","133","134","135","136","137","138","139","140",
	"141","142","143","144","145","146","147","148","149","150","151","152","153","154","155","156","157","158","159","160",
	"161","162","163","164","165","166","167","168","169","170","171","172","173","174","175","176","177","178","179","180",
	"181","182","183","184","185","186","187","188","189","190","191","192","193","194","195","196","197","198","199","200",
	"201","202","203","204","205","206","207","208","209","210","211","212","213","214","215","216","217","218","219","220",
	"221","222","223","224","225","226","227","228","229","230","231","232","233","234","235","236","237","238","239","240",
	"241","242","243","244","245","246","247","248","249","250","251","252","253","254","255"};

std::string fpnn::IPV4ToString(uint32_t internalAddr)
{
	uint8_t *ip = (uint8_t *)&internalAddr;
	std::string desc;
	desc.append(IPV4DigitTable[ip[0]]).append(".");
	desc.append(IPV4DigitTable[ip[1]]).append(".");
	desc.append(IPV4DigitTable[ip[2]]).append(".");
	desc.append(IPV4DigitTable[ip[3]]);
	
	return desc;
}

bool fpnn::checkIP4(const std::string& ip){
	std::vector<std::string> result;
	StringUtil::split(ip, ".", result);
	if(result.size() == 4){
		for(size_t i = 0; i < result.size(); ++i){
			if(result[i].size() > 3) return false;
			for(size_t j = 0; j < result[i].size(); ++j){
				if(!isdigit(result[i][j])) return false;
			}
			if(atoi(result[i].c_str()) > 255) return false;
		}
		return true;
	}
	return false;
}

bool fpnn::parseAddress(const std::string& address, std::string& host, int& port){
	EndPointType eType;
	return parseAddress(address, host, port, eType);
}

bool fpnn::parseAddress(const std::string& address, std::string& host, int& port, EndPointType& eType){
	std::string endpoint = address;
	StringUtil::trim(endpoint);

	std::vector<std::string> result;
	StringUtil::split(endpoint, "#", result);
	if(result.size() == 2){
		host = result[0]; 
		port = atoi(result[1].c_str());
		if(host.find(":") != std::string::npos)//ip6
			eType = ENDPOINT_TYPE_IP6;
		else{ 
			if(checkIP4(host)) eType = ENDPOINT_TYPE_IP4;
			else eType = ENDPOINT_TYPE_DOMAIN;
		}
		return true;
	}

	result.clear();
	StringUtil::split(endpoint, ":", result);
	if(result.size() == 2){
		host = result[0]; 
		port = atoi(result[1].c_str());
		if(checkIP4(host)) eType = ENDPOINT_TYPE_IP4;
		else eType = ENDPOINT_TYPE_DOMAIN;
		return true;
	}
	else if(result.size() > 2){
		port = atoi(result[result.size()-1].c_str());
		host = endpoint.substr(0, endpoint.size() - result[result.size()-1].size() - 1);	
		if(host[0] == '[' && host[host.size() - 1] == ']'){
			host = host.substr(1, host.size() - 2);
		}
		eType = ENDPOINT_TYPE_IP6;
		return true;
	}

    return false;
}

bool fpnn::getIPAddress(const std::string& hostname, std::string& IPAddress, EndPointType& eType)
{
	struct addrinfo hints, *res = NULL;
	memset(&hints, 0, sizeof(struct addrinfo));

	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	int errcode = getaddrinfo(hostname.c_str(), NULL, &hints, &res);
	if (errcode == 0)
	{
		struct sockaddr_in *addr = (struct sockaddr_in *)res->ai_addr;
		IPAddress = inet_ntoa(addr->sin_addr);
		freeaddrinfo(res);
		eType = ENDPOINT_TYPE_IP4;
		return true;
	}

	if (errcode == EAI_ADDRFAMILY || errcode == EAI_FAMILY || errcode == EAI_NODATA)
	{
		//-- Do nothing.
	}
	else
	{
		if (res) freeaddrinfo(res);
		return false;
	}

	//-- IPv6 process

	hints.ai_family = AF_INET6;
	hints.ai_flags = AI_V4MAPPED;

	errcode = getaddrinfo(hostname.c_str(), NULL, &hints, &res);
	if (errcode == 0)
	{
		struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)res->ai_addr;

		char buf[INET6_ADDRSTRLEN + 4];
		const char *rev = inet_ntop(AF_INET6, &(addr6->sin6_addr), buf, sizeof(buf));
		if (rev)
		{
			IPAddress = buf;
			freeaddrinfo(res);
			eType = ENDPOINT_TYPE_IP6;
			return true;
		}
	}

	if (res)
		freeaddrinfo(res);

	return false;
}

bool fpnn::getIPs(std::map<enum IPTypes, std::set<std::string>>& ipDict)
{
	struct ifaddrs *ifaddr, *ifa;
	char host[NI_MAXHOST];

	if (getifaddrs(&ifaddr) == -1)
		return false;

	for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
	{
		if (ifa->ifa_addr == NULL)
			continue;

		if (ifa->ifa_addr->sa_family == AF_INET)
		{
			struct sockaddr_in* addr = (struct sockaddr_in*)(ifa->ifa_addr);
			std::string ipStr = IPV4ToString(addr->sin_addr.s_addr);
			uint8_t *ip = (uint8_t *)&(addr->sin_addr.s_addr);

			enum IPTypes type = IPv4_Public;
			if (ip[0] == 10)
				type = IPv4_Local;
			else if (ip[0] == 172 && (ip[1] > 15 && ip[1] < 32))
				type = IPv4_Local;
			else if (ip[0] == 192 && ip[1] == 168)
				type = IPv4_Local;
			else if (ip[0] == 127 && ip[1] == 0 && ip[2] == 0 && ip[3] == 1)
				type = IPv4_Loopback;

			ipDict[type].insert(ipStr);
		}
		else if (ifa->ifa_addr->sa_family == AF_INET6)
		{
			struct sockaddr_in6* addr = (struct sockaddr_in6*)(ifa->ifa_addr);
			const char *rev = inet_ntop(AF_INET6, &(addr->sin6_addr), host, NI_MAXHOST);
			if (rev == NULL)
			{
				freeifaddrs(ifaddr);
				return false;
			}

			enum IPTypes type = IPv6_Global;
			if (addr->sin6_addr.s6_addr[0] == 0xfe)
			{
				if (addr->sin6_addr.s6_addr[1] == 0x80)
					type = IPv6_LinkLocal;
				else if (addr->sin6_addr.s6_addr[1] == 0xc0)
					type = IPv6_SiteLocal;
			}	
			else if (addr->sin6_addr.s6_addr[0] == 0xff && addr->sin6_addr.s6_addr[1] == 0x00)
				type = IPv6_Multicast;
			else if (strcmp(host, "::1") == 0)
				type = IPv6_Loopback;

			ipDict[type].insert(host);
		}
	}

	freeifaddrs(ifaddr);
	return true;
}

bool fpnn::NetworkUtil::isPrivateIP(struct sockaddr_in* addr)
{
	uint8_t *ip = (uint8_t *)&(addr->sin_addr.s_addr);
	if (ip[0] == 10)
		return true;
	
	if (ip[0] == 172 && (ip[1] > 15 && ip[1] < 32))
		return true;
	
	if (ip[0] == 192 && ip[1] == 168)
		return true;
	
	if (ip[0] == 127 && ip[1] == 0 && ip[2] == 0 && ip[3] == 1)
		return true;

	return false;
}
bool fpnn::NetworkUtil::isPrivateIP(struct sockaddr_in6* addr)
{
	if (addr->sin6_addr.s6_addr[0] == 0xfe)
	{
		if (addr->sin6_addr.s6_addr[1] == 0x80)
			return true;
		else if (addr->sin6_addr.s6_addr[1] == 0xc0)
			return true;
	}
	else
	{
		//-- Loopback
		for (int i = 0; i < 15; i++)
			if (addr->sin6_addr.s6_addr[i] != 0)
				return false;

		if (addr->sin6_addr.s6_addr[15] == 1)
			return true;
	}

	return false;
}
bool fpnn::NetworkUtil::isPrivateIPv4(const std::string& ipv4)
{
	struct sockaddr_in addr;

	if (inet_pton(AF_INET, ipv4.c_str(), &addr.sin_addr) != 1)
		return false;

	return isPrivateIP(&addr);
}
bool fpnn::NetworkUtil::isPrivateIPv6(const std::string& ipv6)
{
	struct sockaddr_in6 addr;

	if (inet_pton(AF_INET6, ipv6.c_str(), &addr.sin6_addr) != 1)
		return false;

	return isPrivateIP(&addr);
}

std::string fpnn::NetworkUtil::getFirstIPAddress(enum IPTypes type)
{
	std::map<enum IPTypes, std::set<std::string>> ipDict;
	if (getIPs(ipDict))
	{
		std::set<std::string>& IPs = ipDict[type];
		if (IPs.empty())
			return "";
		else
			return *(IPs.begin());
	}
	else return "";
}

std::string fpnn::NetworkUtil::getLocalIP4() { return getFirstIPAddress(IPv4_Local); }
std::string fpnn::NetworkUtil::getPublicIP4() { return getFirstIPAddress(IPv4_Public); }

/*
std::string fpnn::NetworkUtil::getPeerName(int fd){
	struct sockaddr_in name;
	socklen_t namelen = sizeof(name);
	char ipAddr[INET_ADDRSTRLEN] = {0};
	if(!getpeername(fd, (struct sockaddr *)&name, &namelen)) {
		const char* ip = inet_ntop(AF_INET, &name.sin_addr, ipAddr, sizeof(ipAddr));
		if(ip) return std::string(ip) + ":" + std::to_string(ntohs(name.sin_port));
	}
	return std::to_string(fd);
}*/

std::string fpnn::NetworkUtil::getPeerName(int fd){

	struct sockaddr_storage name = {0};
	socklen_t namelen = sizeof(name);

	if(!getpeername(fd, (struct sockaddr *)&name, &namelen)) {
		if (name.ss_family == AF_INET)
		{
			char ipAddr[INET_ADDRSTRLEN] = {0};
			struct sockaddr_in* name4 = (struct sockaddr_in*)&name;
			const char* ip = inet_ntop(AF_INET, &(name4->sin_addr), ipAddr, sizeof(ipAddr));
			if(ip) return std::string(ip) + ":" + std::to_string(ntohs(name4->sin_port));
		}
		else if (name.ss_family == AF_INET6)
		{
			char ipAddr[INET6_ADDRSTRLEN] = {0};
			struct sockaddr_in6* name6 = (struct sockaddr_in6*)&name;
			const char* ip = inet_ntop(AF_INET6, &(name6->sin6_addr), ipAddr, sizeof(ipAddr));
			if(ip) return std::string("[") + std::string(ip) + "]:" + std::to_string(ntohs(name6->sin6_port));
		}
	}
	return std::to_string(fd);
}
/*
std::string fpnn::NetworkUtil::getSockName(int fd){
	struct sockaddr_in name;
	socklen_t namelen = sizeof(name);
	if(!getsockname(fd, (struct sockaddr *)&name, &namelen)) {
		char* ip = inet_ntoa(name.sin_addr);
		if(ip) return std::string(ip) + ":" + std::to_string(ntohs(name.sin_port));
	}
	return std::to_string(fd);
}
*/
std::string fpnn::NetworkUtil::getSockName(int fd){
	struct sockaddr_storage name = {0};
	socklen_t namelen = sizeof(name);

	if(!getsockname(fd, (struct sockaddr *)&name, &namelen)) {
		if (name.ss_family == AF_INET)
		{
			char ipAddr[INET_ADDRSTRLEN] = {0};
			struct sockaddr_in* name4 = (struct sockaddr_in*)&name;
			const char* ip = inet_ntop(AF_INET, &(name4->sin_addr), ipAddr, sizeof(ipAddr));
			if(ip) return std::string(ip) + ":" + std::to_string(ntohs(name4->sin_port));
		}
		else if (name.ss_family == AF_INET6)
		{
			char ipAddr[INET6_ADDRSTRLEN] = {0};
			struct sockaddr_in6* name6 = (struct sockaddr_in6*)&name;
			const char* ip = inet_ntop(AF_INET6, &(name6->sin6_addr), ipAddr, sizeof(ipAddr));
			if(ip) return std::string("[") + std::string(ip) + "]:" + std::to_string(ntohs(name6->sin6_port));
		}
	}
	return std::to_string(fd);
}


#ifdef TEST_NETWORK_UTIL
//g++ -g -DTEST_NETWORK_UTIL NetworkUtility.cpp -std=c++11 -lfpbase -L.
#include <sstream>
#include <iostream>
using namespace std;
using namespace StringUtil;

void testGetIPs()
{
	std::map<enum IPTypes, std::set<std::string>> ipDict;
	if (getIPs(ipDict) == false)
	{
		cout<<"getIPs() return false"<<endl;
		return;
	}

	for (auto& ip: ipDict)
	{
		if (ip.first == IPv4_Public)
			cout<<"IPv4_Public:"<<endl;
		else if (ip.first == IPv4_Local)
			cout<<"IPv4_Local:"<<endl;
		else if (ip.first == IPv4_Loopback)
			cout<<"IPv4_Loopback:"<<endl;
		else if (ip.first == IPv6_Global)
			cout<<"IPv6_Global:"<<endl;
		else if (ip.first == IPv6_LinkLocal)
			cout<<"IPv6_LinkLocal:"<<endl;
		else if (ip.first == IPv6_SiteLocal)
			cout<<"IPv6_SiteLocal:"<<endl;
		else if (ip.first == IPv6_Multicast)
			cout<<"IPv6_Multicast:"<<endl;
		else if (ip.first == IPv6_Loopback)
			cout<<"IPv6_Loopback:"<<endl;

		for (auto& ip2: ip.second)
			cout<<"\t"<<ip2<<endl;
	}
}

int main(int argc, char **argv)
{
	string ip4_1 = "192.168.0.1:80";
	string ip4_2 = "192.168.0.1#80";
	string ip6_1 = "[2001:db8::1]:80";
	string ip6_2 = "2001:db8::1:80";
	string ip6_3 = "2001:db8::1#80";
	string domain_1 = "localhost: 80";
	string domain_2 = "www.baidu.com#80";

	string host;
	int port;
	EndPointType eType;

	parseAddress(ip4_1, host, port, eType);
	cout<<"endpoint: "<<ip4_1<<" parsed host: "<<host<<" port: "<<port<<" eType: "<<eType<<endl;

	parseAddress(ip4_2, host, port, eType);
	cout<<"endpoint: "<<ip4_2<<" parsed host: "<<host<<" port: "<<port<<" eType: "<<eType<<endl;

	parseAddress(ip6_1, host, port, eType);
	cout<<"endpoint: "<<ip6_1<<" parsed host: "<<host<<" port: "<<port<<" eType: "<<eType<<endl;

	parseAddress(ip6_2, host, port, eType);
	cout<<"endpoint: "<<ip6_2<<" parsed host: "<<host<<" port: "<<port<<" eType: "<<eType<<endl;

	parseAddress(ip6_3, host, port, eType);
	cout<<"endpoint: "<<ip6_3<<" parsed host: "<<host<<" port: "<<port<<" eType: "<<eType<<endl;

	parseAddress(domain_1, host, port, eType);
	cout<<"endpoint: "<<domain_1<<" parsed host: "<<host<<" port: "<<port<<" eType: "<<eType<<endl;

	parseAddress(domain_2, host, port, eType);
	cout<<"endpoint: "<<domain_2<<" parsed host: "<<host<<" port: "<<port<<" eType: "<<eType<<endl;


	std::string domain = "www.baidu.com";
	bool ok = getIPAddress(domain, host, eType);
	if (ok)
		cout<<"GetAddress: "<<domain<<": "<<host<<" type: "<<eType<<endl;
	else
		cout<<"GetAddress: "<<domain<<" failed."<<endl;

	domain = "www.google.com";
	ok = getIPAddress(domain, host, eType);
	if (ok)
		cout<<"GetAddress: "<<domain<<": "<<host<<" type: "<<eType<<endl;
	else
		cout<<"GetAddress: "<<domain<<" failed."<<endl;


	testGetIPs();
	return 0;
}

#endif
