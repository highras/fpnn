#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netdb.h>
#include <string.h>
#include "HostLookup.h"
#include "FPLog.h"

using namespace fpnn;

std::string HostLookup::get(const std::string& host){
	return add(host);
}

std::string HostLookup::add(const std::string& host){
	std::string ip = getAddrInfo(host);
	return ip.empty() ? host : ip;
}

std::string HostLookup::getAddrInfo(const std::string& host){
	int errcode = 0; 
	struct sockaddr_in addr;

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (!host.empty()){
        struct in_addr ipaddr;

        if (inet_aton(host.c_str(), &ipaddr)){
            addr.sin_addr = ipaddr;
        }    
        else if (host == "localhost"){
            addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        }    
        else 
        {    
            struct addrinfo hints = {0}, *res = NULL;

			/* Don't use gethostbyname(), it's not thread-safe. 
               Use getaddrinfo() instead.
             */
			/*
			 * getaddrinfo not thread-safe tooooooooooo, especially in multi core, so lock it.
			 * */
            hints.ai_family = AF_INET;
            hints.ai_socktype = SOCK_STREAM;
            hints.ai_protocol = IPPROTO_TCP;
            errcode = getaddrinfo(host.c_str(), NULL, &hints, &res);
            if (errcode != 0){
                LOG_ERROR("host:%s getaddrinfo() failed: %s\n", host.c_str(), gai_strerror(errcode));
            }
            else{
                memcpy(&addr, (struct sockaddr_in *)res->ai_addr, sizeof(addr));
            }

            if (res)
                freeaddrinfo(res);
        }
    }

	if(errcode) return "";
	LOG_DEBUG("resolve host:%s to:%s", host.c_str(), inet_ntoa(addr.sin_addr));
	return inet_ntoa(addr.sin_addr);
}

#ifdef TEST_HOST_LOOKUP
//g++ -g -DTEST_HOST_LOOKUP HostLookup.cpp -I. -I../base -L../base -L. -lfunplus -std=c++11
#include <sstream>
#include <iostream>
#include <vector>
using namespace std;

int main(int argc, char **argv)
{
	vector<string> hosts;
	hosts.push_back("localhost");
	hosts.push_back("ec2-54-169-77-186.ap-southeast-1.compute.amazonaws.com");
	hosts.push_back("ip-172-31-17-26.ap-southeast-1.compute.internal");
	hosts.push_back("ip-172-31-20-17.eu-west-1.compute.internal");
	hosts.push_back("ec2-54-76-216-141.eu-west-1.compute.amazonaws.com");
	hosts.push_back("ip-10-18-0-42.cn-north-1.compute.internal");
	hosts.push_back("ec2-54-223-128-134.cn-north-1.compute.amazonaws.com.cn");
	hosts.push_back("none_north-1.compute.amazonaws.com.cn");
	hosts.push_back("none.cn-north-1.compute.amazonaws.com.cn");
	for(size_t i = 0; i < hosts.size(); ++i){
		HostLookup::add(hosts[i]);
	}
	cout<<endl<<endl;
	for(size_t i = 0; i < hosts.size(); ++i){
		string ip = HostLookup::get(hosts[i]);
		cout<<"host:"<<hosts[i]<<"  ip:"<<ip<<endl;
	}
	return 0;
}
#endif
