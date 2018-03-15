#ifndef FPNN_IP_WhiteList_H
#define FPNN_IP_WhiteList_H

#include <netinet/in.h>
#include <arpa/inet.h>
#include <string>
#include <set>
#include <mutex>
#include <map>

namespace fpnn
{
	class RecordIPList
	{
		class IPv6Comp
		{
		public:
			bool operator() (const std::string& a, const std::string& b) const
			{
				uint64_t *a1 = (uint64_t *)a.data();
				uint64_t *a2 = (uint64_t *)(a.data() + 8);

				uint64_t *b1 = (uint64_t *)b.data();
				uint64_t *b2 = (uint64_t *)(b.data() + 8);

				if (a1 == b1)
					return a2 < b2;
				
				return a1 < b1;
			}
		};

		std::mutex _mutex;
		std::set<uint32_t> _ipv4s;
		std::set<std::string, IPv6Comp> _ipv6s;
		std::map<uint32_t, std::set<uint32_t>> _ipv4Subnets;		//-- map<subnet mask, set<net ip>>

		bool isInSubnet(in_addr_t s_addr)
		{
			for (auto& spair: _ipv4Subnets)
			{
				uint32_t netIP = (uint32_t)s_addr & spair.first;
				if (spair.second.find(netIP) != spair.second.end())
					return true;
			}
			return false;
		}

	public:
		bool addIP(const std::string& ip)
		{
			if (ip.find(':') == std::string::npos)
			{
				uint32_t addr = (uint32_t)inet_addr(ip.c_str());
				std::unique_lock<std::mutex> lck(_mutex);
				_ipv4s.insert(addr);
				return true;
			}
			else
			{
				struct sockaddr_in6 serverAddr;
				memset(&serverAddr, 0, sizeof(serverAddr));
				if (inet_pton(AF_INET6, ip.c_str(), &serverAddr.sin6_addr) != 1)
					return false;

				std::string ip6((char *)serverAddr.sin6_addr.s6_addr, 16);
				std::unique_lock<std::mutex> lck(_mutex);
				_ipv6s.insert(ip6);
				return true;
			}
		}

		void removeIP(const std::string& ip)
		{
			if (ip.find(':') == std::string::npos)
			{
				uint32_t addr = (uint32_t)inet_addr(ip.c_str());
				std::unique_lock<std::mutex> lck(_mutex);
				_ipv4s.erase(addr);
			}
			else
			{
				struct sockaddr_in6 serverAddr;
				memset(&serverAddr, 0, sizeof(serverAddr));
				if (inet_pton(AF_INET6, ip.c_str(), &serverAddr.sin6_addr) != 1)
					return;

				std::string ip6((char *)serverAddr.sin6_addr.s6_addr, 16);
				std::unique_lock<std::mutex> lck(_mutex);
				_ipv6s.erase(ip6);
			}
		}

		inline bool addIPv4SubNet(const std::string& ip, int subnetMaskBits)
		{
			if (subnetMaskBits < 1 || subnetMaskBits > 31)
				return false;

			uint32_t mask = 0;
			{
				int idx = 31;
				for (int i = 0; i < subnetMaskBits; i++, idx--)
					mask |= (0x1 << idx);
			}
			mask = htonl(mask);

			uint32_t netIP = (uint32_t)inet_addr(ip.c_str());
			netIP = netIP & mask;

			std::unique_lock<std::mutex> lck(_mutex);
			_ipv4Subnets[mask].insert(netIP);
			return true;
		}

		inline bool exist(in_addr_t s_addr)
		{
			std::unique_lock<std::mutex> lck(_mutex);
			bool status = (_ipv4s.find((uint32_t)s_addr) != _ipv4s.end());
			return status ? true : isInSubnet(s_addr);
		}

		inline bool exist(const struct in6_addr& sin6_addr)
		{
			std::string ip6((char*)sin6_addr.s6_addr, 16);
			std::unique_lock<std::mutex> lck(_mutex);
			return (_ipv6s.find(ip6) != _ipv6s.end());
		}
	};

	class IPWhiteList: public RecordIPList
	{
	public:
		IPWhiteList()
		{
			addIP("127.0.0.1");
			addIP("::1");
		}
	};
}

#endif
