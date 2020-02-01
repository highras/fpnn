#include <exception>
#include <stdexcept>
#include <sstream>
#include <iostream>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "HttpClient.h"
#include "ServerInfo.h"
#include "FPLog.h"
#include "Setting.h"
#include "FPJson.h"
#include "net.h"
#include "StringUtil.h"
#include "NetworkUtility.h"
using namespace fpnn;
//aws
//curl http://169.254.169.254/latest/meta-data/placement/availability-zone/
//curl http://169.254.169.254/latest/meta-data/public-hostname/


//gcp
//curl -H "Metadata-Flavor: Google" "http://metadata/computeMetadata/v1/instance/"

//azure
//curl -H Metadata:true "http://169.254.169.254/metadata/instance?api-version=2018-02-01"

//tencent
//curl http://metadata.tencentyun.com/latest/meta-data/

//alibaba
//curl http://100.100.100.200/latest/meta-data/
//-- 等待添加

std::string ServerInfo::_serverHostName;
std::string ServerInfo::_serverRegionName;
std::string ServerInfo::_serverZoneName;
std::string ServerInfo::_serverLocalIP4;
std::string ServerInfo::_serverPubliceIP4;
std::string ServerInfo::_serverLocalIP6;
std::string ServerInfo::_serverPubliceIP6;

static std::string AWS_BASE_HOST = "http://169.254.169.254/latest/meta-data";
static std::string GCP_BASE_HOST = "http://metadata/computeMetadata/v1/instance/";
static std::string AZURE_BASE_HOST = "http://169.254.169.254/metadata/instance?api-version=2018-02-01";
static std::string TENCENT_BASE_HOST = "http://metadata.tencentyun.com/latest/meta-data";
static std::string HOST_PLATFORM = "FP.server.host.platform";

const std::string& ServerInfo::getServerHostName(){
	while(_serverHostName.empty()){
		std::string key = "FP.server.hostname";
		if(Setting::setted(key)){
			_serverHostName = Setting::getString(key);
			return _serverHostName;
		}
#ifdef HOST_PLATFORM_AWS
		std::string url = AWS_BASE_HOST+"/public-hostname/";
		_serverHostName = getAWSInfo(url);
#elif HOST_PLATFORM_GCP
		std::string url = GCP_BASE_HOST+"/hostname";
		_serverHostName = getGCPInfo(url);
#elif HOST_PLATFORM_AZURE
		if(getAZUREInfo(AZURE_BASE_HOST))
			return _serverHostName;
#elif HOST_PLATFORM_TENCENT
		std::string url = TENCENT_BASE_HOST+"/instance-name";
		_serverHostName = getTencentInfo(url);
#else
		std::cout<<"Unknow platform"<<std::endl;
		exit(-10);
#endif
		usleep(100*1000);
	}
	return _serverHostName;
}

const std::string& ServerInfo::getServerZoneName(){
	while(_serverZoneName.empty()){
		std::string key = "FP.server.zone.name";
		if(Setting::setted(key)){
			_serverZoneName = Setting::getString(key);
			return _serverZoneName;
		}
#ifdef HOST_PLATFORM_AWS
		std::string url = AWS_BASE_HOST+"/placement/availability-zone/";
		_serverZoneName = getAWSInfo(url);
#elif HOST_PLATFORM_GCP
		std::string url = GCP_BASE_HOST+"/zone";
		_serverZoneName = getGCPInfo(url);
		std::size_t pos = _serverZoneName.find_last_of("/");
		if(pos != std::string::npos)
			_serverZoneName = _serverZoneName.substr(pos+1, _serverZoneName.size());
		else{
			_serverZoneName = "";
			return _serverZoneName;
		}
#elif HOST_PLATFORM_AZURE
		if(getAZUREInfo(AZURE_BASE_HOST))
			return _serverZoneName;
#elif HOST_PLATFORM_TENCENT
		std::string url = TENCENT_BASE_HOST+"/placement/zone";
		_serverZoneName = getTencentInfo(url);
#else
		std::cout<<"Unknow platform"<<std::endl;
		exit(-10);
#endif
		usleep(100*1000);
	}
	return _serverZoneName;
}

const std::string& ServerInfo::getServerRegionName(){
	while(_serverRegionName.empty()){
		std::string key = "FP.server.region.name";
		if(Setting::setted(key)){
			_serverRegionName = Setting::getString(key);
			return _serverRegionName;
		}
#ifdef HOST_PLATFORM_AWS
		std::string zoneName = getServerZoneName();
		_serverRegionName = zoneName.substr(0, zoneName.length()-3);    // remove zone suffix
#elif HOST_PLATFORM_GCP
		std::string zoneName = getServerZoneName();
		_serverRegionName = zoneName.substr(0, zoneName.length()-3);    // remove zone suffix
#elif HOST_PLATFORM_AZURE
		if(getAZUREInfo(AZURE_BASE_HOST))
			return _serverRegionName;
#elif HOST_PLATFORM_TENCENT
		std::string url = TENCENT_BASE_HOST+"/placement/region";
		_serverRegionName = getTencentInfo(url);
#else
		std::cout<<"Unknow platform"<<std::endl;
		exit(-10);
#endif
		usleep(100*1000);
	}
	return _serverRegionName;
}

const std::string& ServerInfo::getServerLocalIP4(){
	while(_serverLocalIP4.empty()){
		std::string key = "FP.server.local.ip4";
		if(Setting::setted(key)){
			_serverLocalIP4 = Setting::getString(key);
			return _serverLocalIP4;
		}
#ifdef HOST_PLATFORM_AWS
		std::string url = AWS_BASE_HOST+"/local-ipv4/";
		_serverLocalIP4 = getAWSInfo(url);
#elif HOST_PLATFORM_GCP
		std::string url = GCP_BASE_HOST+"/network-interfaces/0/ip";
		_serverLocalIP4 = getGCPInfo(url);
#elif HOST_PLATFORM_AZURE
		if(getAZUREInfo(AZURE_BASE_HOST))
			return _serverLocalIP4;
#elif HOST_PLATFORM_TENCENT
		std::string url = TENCENT_BASE_HOST+"/local-ipv4";
		_serverLocalIP4 = getTencentInfo(url);
#else
		_serverLocalIP4 = NetworkUtil::getLocalIP4();
		if (_serverLocalIP4.length())
			return _serverLocalIP4;
		else
		{
			std::cout<<"Unknow platform"<<std::endl;
			exit(-10);
		}
#endif
		usleep(100*1000);
	}
	return _serverLocalIP4;
}

const std::string& ServerInfo::getServerPublicIP4(){
	while(_serverPubliceIP4.empty()){
		std::string key = "FP.server.public.ip4";
		if(Setting::setted(key)){
			_serverPubliceIP4 = Setting::getString(key);
			return _serverPubliceIP4;
		} 
#ifdef HOST_PLATFORM_AWS
		std::string url = AWS_BASE_HOST+"/public-ipv4/";
		_serverPubliceIP4 = getAWSInfo(url);
#elif HOST_PLATFORM_GCP
		std::string url = GCP_BASE_HOST+"/network-interfaces/0/access-configs/0/external-ip";
		_serverPubliceIP4 = getGCPInfo(url);
#elif HOST_PLATFORM_AZURE
		if(getAZUREInfo(AZURE_BASE_HOST))
			return _serverPubliceIP4;
#elif HOST_PLATFORM_TENCENT
		std::string url = TENCENT_BASE_HOST+"/public-ipv4";
		_serverPubliceIP4 = getTencentInfo(url);
#else
		_serverPubliceIP4 = NetworkUtil::getPublicIP4();
		if (_serverPubliceIP4.length())
			return _serverPubliceIP4;
		else
		{
			std::cout<<"Unknow platform"<<std::endl;
			exit(-10);
		}
#endif
		usleep(100*1000);
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

std::string ServerInfo::getAWSInfo(const std::string& url){
	std::string resp;
	std::vector<std::string> header;
	HttpClient::Get(url, header, resp);
	return resp;
}

std::string ServerInfo::getTencentInfo(const std::string& url){
	return getAWSInfo(url);
}

std::string ServerInfo::getGCPInfo(const std::string& url){
	std::string resp;
	std::vector<std::string> header;
	header.push_back("Metadata-Flavor: Google");
	HttpClient::Get(url, header, resp);
	return resp;
}

bool ServerInfo::getAZUREInfo(const std::string& url){
	std::string resp;
	std::vector<std::string> header;
	header.push_back("Metadata: true");
	HttpClient::Get(url, header, resp);
	try {
		JsonPtr json = Json::parse(resp.c_str());
		_serverHostName = json->wantString("compute/name");
		_serverRegionName = json->wantString("compute/location");
		_serverZoneName = json->wantString("compute/zone");
		json = json->getList("network/interface")->front();
		json = json->getList("ipv4/ipAddress")->front();
		_serverLocalIP4 = json->wantString("privateIpAddress");
		_serverPubliceIP4 = json->wantString("publicIpAddress");
	}   
	catch (const FpnnError &e) 
	{   
		std::cout<<"Can not get MetaData\n";
		return false;
	}   
	return true;
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
		getServerHostName();
		getServerZoneName();
		getServerLocalIP4();
		getServerPublicIP4();
		getServerLocalIP6();
		getServerPublicIP6();
	}
	catch(const std::exception& ex){
		//do again, if error, throw it
		getServerHostName();
		getServerZoneName();
		getServerLocalIP4();
		getServerPublicIP4();
		getServerLocalIP6();
		getServerPublicIP6();
	}
}

#ifdef TEST_SERVER_INFO
//g++ -std=c++11 -g -DTEST_SERVER_INFO ServerInfo.cpp  -I. -L. -lcurl -lfpbase -DHOST_PLATFORM_AWS
using namespace std;

int main(int argc, char **argv)
{
	string HostName = ServerInfo::getServerHostName();
	string zoneName = ServerInfo::getServerZoneName();
	string localIP = ServerInfo::getServerLocalIP4();
	string publicIP = ServerInfo::getServerPublicIP4();
	string regionName = ServerInfo::getServerRegionName();

	printf("HostName:%s\n",HostName.c_str());
	printf("zoneName:%s\n",zoneName.c_str());
	printf("localIP:%s\n",localIP.c_str());
	printf("publicIP:%s\n",publicIP.c_str());
	printf("regionName:%s\n",regionName.c_str());
	return 0;
}
#endif
