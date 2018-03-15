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
		FPZKClientPtr _fpzkClient;
		std::string _serviceName;

		void checkRevision(const char* proxyTypeForLog)
		{
			FPZKClient::ServiceInfosPtr sip = _fpzkClient->getServiceInfos(_serviceName);
			if (sip == nullptr)
			{
				LOG_ERROR("[%s] Service %s is empty! rev:%lld", proxyTypeForLog, _serviceName.c_str(), _rev);
				sip = std::make_shared<FPZKClient::ServiceInfos>();
			}

			if (_rev != sip->revision || !_rev || _endpoints.empty())
			{
				_endpoints.clear();
				for (auto& nodePair: sip->nodeMap)
					_endpoints.push_back(nodePair.first);
				
				LOG_INFO("[%s Changed] Rev(old/new):%lld/%lld serviceName:%s, server list:%s", proxyTypeForLog,
					_rev, sip->revision, _serviceName.c_str(), StringUtil::join(_endpoints, ", ").c_str());

				checkClientMap();
				_rev = sip->revision;
				afterUpdate();
			}
		}

	public:
		//-- If questTimeoutSeconds less then zero, mean using global settings.
		TCPFPZKProxyCore(FPZKClientPtr fpzkClient, const std::string& serviceName, int64_t questTimeoutSeconds = -1):
			TCPProxyCore(questTimeoutSeconds), _rev(0), _fpzkClient(fpzkClient), _serviceName(serviceName)
		{
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
