#ifndef FPNN_TCP_FPZK_PROXY_CORE_H
#define FPNN_TCP_FPZK_PROXY_CORE_H

#include "FPLog.h"
#include "StringUtil.h"
#include "FPZKClient.h"
#include "TCPProxyCore.hpp"

namespace fpnn
{
	class TCPFPZKProxyCore: virtual public TCPProxyCore
	{
	protected:
		int64_t _rev;
		int64_t _clusterAlteredMsec;
		FPZKClientPtr _fpzkClient;
		std::string _serviceName;

		void checkRevision(const char* proxyTypeForLog)
		{
			FPZKClient::ServiceInfosPtr sip = _fpzkClient->getServiceInfos(_serviceName);
			if (sip == nullptr)
			{
				LOG_ERROR("[%s] Service %s is empty! last rev:%lld, last altered msec:%lld",
					proxyTypeForLog, _serviceName.c_str(), _rev, _clusterAlteredMsec);

				sip = std::make_shared<FPZKClient::ServiceInfos>();
			}

			if (_clusterAlteredMsec != sip->clusterAlteredMsec || _rev != sip->revision || _endpoints.empty())
			{
				_endpoints.clear();
				for (auto& nodePair: sip->nodeMap)
					_endpoints.push_back(nodePair.first);
				
				LOG_INFO("[%s Changed] ServiceName: %s, rev(old/new):%lld/%lld, alter msec(old/new):%lld/%lld, server list:%s",
					proxyTypeForLog, _serviceName.c_str(), _rev, sip->revision,
					_clusterAlteredMsec, sip->clusterAlteredMsec, StringUtil::join(_endpoints, ", ").c_str());

				checkClientMap();
				_rev = sip->revision;
				_clusterAlteredMsec = sip->clusterAlteredMsec;
				afterUpdate();
			}
		}

	public:
		//-- If questTimeoutSeconds less then zero, mean using global settings.
		TCPFPZKProxyCore(FPZKClientPtr fpzkClient, const std::string& serviceName, const std::string& cluster = "", int64_t questTimeoutSeconds = -1):
			TCPProxyCore(questTimeoutSeconds), _rev(0), _clusterAlteredMsec(0), _fpzkClient(fpzkClient), _serviceName(serviceName)
		{
			if (!cluster.empty())
				_serviceName.append("@").append(cluster);
		}

		virtual ~TCPFPZKProxyCore()
		{
		}

		const std::string& serviceName() const
		{
			return _serviceName;
		}
	};
}

#endif
