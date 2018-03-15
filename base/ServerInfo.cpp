#include <exception>
#include <stdexcept>
#include <sstream>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "HttpClient.h"
#include "ServerInfo.h"
#include "FPLog.h"
#include "Setting.h"
#include "net.h"
#include "StringUtil.h"
using namespace fpnn;
//curl http://169.254.169.254/latest/meta-data/placement/availability-zone/
//curl http://169.254.169.254/latest/meta-data/public-hostname/
//

std::string ServerInfo::_serverDomain;
std::string ServerInfo::_serverRegionName;
std::string ServerInfo::_serverZoneName;
std::string ServerInfo::_serverLocalIP4;
std::string ServerInfo::_serverPubliceIP4;
std::string ServerInfo::_serverLocalIP6;
std::string ServerInfo::_serverPubliceIP6;

static std::string AWS_BASE_HOST = "169.254.169.254";
static std::string HOST_PLATFORM = "FP.server.host.platform";

const std::string& ServerInfo::getServerDomain(){
	if(_serverDomain.empty()){
		std::string key = "FP.server.domain";
		if(Setting::setted(key)){
			_serverDomain = Setting::getString(key);
			return _serverDomain;
		}
#ifdef HOST_PLATFORM_AWS
		std::string url = "http://"+AWS_BASE_HOST+"/latest/meta-data/public-hostname/";
		_serverDomain = getInfo(url);
#elif HOST_PLATFORM_QINGCLOUD
		//qingcloud
		_serverDomain = "http://qingcloud";
#elif HOST_PLATFORM_UCLOUD
		//ucloud
		_serverDomain = "http://ucloud";
#else
		_serverDomain = "unknown";
#endif
	}
	return _serverDomain;
}

const std::string& ServerInfo::getServerZoneName(){
	if(_serverZoneName.empty()){
		std::string key = "FP.server.zone.name";
		if(Setting::setted(key)){
			_serverZoneName = Setting::getString(key);
			return _serverZoneName;
		}
#ifdef HOST_PLATFORM_AWS
		std::string url = "http://"+AWS_BASE_HOST+"/latest/meta-data/placement/availability-zone/";
		_serverZoneName = getInfo(url);
#elif HOST_PLATFORM_QINGCLOUD
		//qingcloud
		_serverZoneName = "qcloud-zone";
#elif HOST_PLATFORM_UCLOUD
		//ucloud
		_serverZoneName = "ucloud-zone";
#else
		_serverZoneName = "unknown";
#endif
	}
	return _serverZoneName;
}

const std::string& ServerInfo::getServerRegionName(){
	if(_serverRegionName.empty()){
		std::string key = "FP.server.region.name";
		if(Setting::setted(key)){
			_serverRegionName = Setting::getString(key);
			return _serverRegionName;
		}
#ifdef HOST_PLATFORM_AWS
		std::string zoneName = getServerZoneName();
		_serverRegionName = zoneName.substr(0, zoneName.length()-1);    // remove zone suffix
#elif HOST_PLATFORM_QINGCLOUD
		//qingcloud
		_serverRegionName= "qcloud-region";
#elif HOST_PLATFORM_UCLOUD
		//ucloud
		_serverRegionName= "ucloud-region";
#else
		_serverRegionName = "unknown";
#endif
	}
	return _serverRegionName;
}

const std::string& ServerInfo::getServerLocalIP4(){
	if(_serverLocalIP4.empty()){
		std::string key = "FP.server.local.ip4";
		if(Setting::setted(key)){
			_serverLocalIP4 = Setting::getString(key);
			return _serverLocalIP4;
		}
#ifdef HOST_PLATFORM_AWS
		std::string url = "http://"+AWS_BASE_HOST+"/latest/meta-data/local-ipv4/";
		_serverLocalIP4 = getInfo(url);
#elif HOST_PLATFORM_QINGCLOUD
		//qingcloud
		char localIP[32] = {0};
		net_get_internal_ip(localIP);
		_serverLocalIP4 = localIP;
#elif HOST_PLATFORM_UCLOUD
		//ucloud
		char localIP[32] = {0};
		net_get_internal_ip(localIP);
		_serverLocalIP4 = localIP;
#else
		_serverLocalIP4 = "unknown";
#endif
	}
	return _serverLocalIP4;
}

const std::string& ServerInfo::getServerPublicIP4(){
	if(_serverPubliceIP4.empty()){
		std::string key = "FP.server.public.ip4";
		if(Setting::setted(key)){
			_serverPubliceIP4 = Setting::getString(key);
			return _serverPubliceIP4;
		} 
#ifdef HOST_PLATFORM_AWS
		std::string url = "http://"+AWS_BASE_HOST+"/latest/meta-data/public-ipv4/";
		_serverPubliceIP4 = getInfo(url);
#elif HOST_PLATFORM_QINGCLOUD
		//qingcloud
		_serverPubliceIP4 = "empty";
#elif HOST_PLATFORM_UCLOUD
		//ucloud
		_serverPubliceIP4 = "empty";
#else
		_serverPubliceIP4 = "unknown";
#endif
	}
	return _serverPubliceIP4;
}

const std::string& ServerInfo::getServerLocalIP6(){
    if(_serverLocalIP6.empty()) {
        std::string ipv4 = getServerLocalIP4();
        _serverLocalIP6 = ipv4Toipv6(ipv4);
        if (_serverLocalIP6.empty())
            _serverLocalIP6 = "unknown";
    }
	return _serverLocalIP6;
}

const std::string& ServerInfo::getServerPublicIP6(){
    if(_serverPubliceIP6.empty()) {
        std::string ipv4 = getServerPublicIP4();
        _serverPubliceIP6 = ipv4Toipv6(ipv4);
        if (_serverPubliceIP6.empty())
            _serverPubliceIP6 = "unknown";
    }
	return _serverPubliceIP6;
}

std::string ServerInfo::getInfo(std::string url){
	std::string resp;
	HttpClient::Get(url, resp, 1);
	if(resp.empty()) resp = "unknown";
	return resp;
}

std::string ServerInfo::ipv4Toipv6(const std::string& ipv4) {
    // https://tools.ietf.org/html/rfc6052
    std::vector<std::string> parts;
    StringUtil::split(ipv4, ".", parts);
    if (parts.size() != 4)
        return "";
    for (auto& part : parts) {
        int32_t p = atoi(part.c_str());
        if (p < 0 || p > 255)
            return "";
    }
    int32_t part7 = atoi(parts[0].c_str()) * 256 + atoi(parts[1].c_str());
    int32_t part8 = atoi(parts[2].c_str()) * 256 + atoi(parts[3].c_str());
    std::stringstream ss;
    ss << "64:ff9b::" << std::hex << part7 << ":" << std::hex << part8; 
    return ss.str();
}

void ServerInfo::getAllInfos(){
	try{
		getServerDomain();
		getServerZoneName();
		getServerLocalIP4();
		getServerPublicIP4();
		getServerLocalIP6();
		getServerPublicIP6();
	}
	catch(const std::exception& ex){
		//do again, if error, throw it
		getServerDomain();
		getServerZoneName();
		getServerLocalIP4();
		getServerPublicIP4();
		getServerLocalIP6();
		getServerPublicIP6();
	}
}

#ifdef TEST_SERVER_INFO
//g++ -std=c++11 -g -DTEST_SERVER_INFO ServerInfo.cpp  -I. -L. -lcurl -lfpbase
using namespace std;

int main(int argc, char **argv)
{
	//Setting::insert("FP.server.host.platform", "aws");
	//Setting::insert("FP.server.host.platform", "qingcloud");

	string publicHostName = ServerInfo::getServerDomain();
	string zoneName = ServerInfo::getServerZoneName();
	string localIP = ServerInfo::getServerLocalIP4();
	string publicIP = ServerInfo::getServerPublicIP4();
	string regionName = ServerInfo::getServerRegionName();

	printf("publicHostName:%s\n",publicHostName.c_str());
	printf("zoneName:%s\n",zoneName.c_str());
	printf("localIP:%s\n",localIP.c_str());
	printf("publicIP:%s\n",publicIP.c_str());
	printf("regionName:%s\n",regionName.c_str());
	return 0;
}
#endif
