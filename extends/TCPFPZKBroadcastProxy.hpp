#ifndef FPNN_TCP_FPZK_Broadcast_Proxy_H
#define FPNN_TCP_FPZK_Broadcast_Proxy_H

#include "TCPBroadcastProxy.hpp"
#include "TCPFPZKProxyCore.hpp"

namespace fpnn
{
	/*
		If using Quest Processor, please set Quest Processor before before call updateEndpoints().
	*/
	class TCPFPZKBroadcastProxy: virtual public TCPBroadcastProxy, virtual public TCPFPZKProxyCore
	{
		virtual void beforeBorcast()
		{
			checkRevision("FPZK Broadcast Proxy");
		}

		virtual void beforeGetClient()
		{
			checkRevision("FPZK Broadcast Proxy");
		}

	public:
		//-- If questTimeoutSeconds less then zero, mean using global settings.
		TCPFPZKBroadcastProxy(FPZKClientPtr fpzkClient, const std::string& serviceName, const std::string& cluster = "", int64_t questTimeoutSeconds = -1):
			TCPProxyCore(questTimeoutSeconds), TCPBroadcastProxy(questTimeoutSeconds),
			TCPFPZKProxyCore(fpzkClient, serviceName, cluster, questTimeoutSeconds) {}

		virtual ~TCPFPZKBroadcastProxy() {}

		void exceptSelf()
		{
			_selfEndpoint = _fpzkClient->registeredEndpoint();
		}
	};
	typedef std::shared_ptr<TCPFPZKBroadcastProxy> TCPFPZKBroadcastProxyPtr;
}

#endif
