#ifndef FPNN_TCP_FPZK_Consistency_Proxy_H
#define FPNN_TCP_FPZK_Consistency_Proxy_H

#include "TCPConsistencyProxy.hpp"
#include "TCPFPZKProxyCore.hpp"

namespace fpnn
{
	/*
		If using Quest Processor, please set Quest Processor before before call updateEndpoints().
	*/
	class TCPFPZKConsistencyProxy: virtual public TCPConsistencyProxy, virtual public TCPFPZKProxyCore
	{
		virtual void beforeBorcastQuest()
		{
			checkRevision("FPZK Consistency Proxy");
		}
		
		virtual void beforeGetClient()
		{
			checkRevision("FPZK Consistency Proxy");
		}

	public:
		/*
		 *	If questTimeoutSeconds less then zero, mean using global settings.
		 *  If SuccessCondition isn't CountedSuccess, parameter requiredCount is ignored.
		 */
		TCPFPZKConsistencyProxy(FPZKClientPtr fpzkClient, const std::string& serviceName,
				ConsistencySuccessCondition condition, int requiredCount = 0, int64_t questTimeoutSeconds = -1):
			TCPProxyCore(questTimeoutSeconds), TCPConsistencyProxy(condition, requiredCount, questTimeoutSeconds),
			TCPFPZKProxyCore(fpzkClient, serviceName, "", questTimeoutSeconds)
		{
		}
		TCPFPZKConsistencyProxy(FPZKClientPtr fpzkClient, const std::string& serviceName, const std::string& cluster,
				ConsistencySuccessCondition condition, int requiredCount = 0, int64_t questTimeoutSeconds = -1):
			TCPProxyCore(questTimeoutSeconds), TCPConsistencyProxy(condition, requiredCount, questTimeoutSeconds),
			TCPFPZKProxyCore(fpzkClient, serviceName, cluster, questTimeoutSeconds)
		{
		}
		virtual ~TCPFPZKConsistencyProxy()
		{
			/* don't need to do anythings.
			 * The _callbackMapSuite will be holding in each callback of each client in each connection callback map of them.
			 * If all clients are destroyed after this function, each client will clear their connection callback map.
			 * When the callback map of client clearing, the callback in callback map will be given an error, and the action will
			 * trigger the ConsistencyAnswerCallback to increase the failed count. When the failed count is triggered the failed
			 * condition, orginial callback in map of _callbackMapSuite will be triggered, and the original callback will be cleared
			 * from _callbackMapSuite. When the last ConsistencyAnswerCallback destroyed, _callbackMapSuite's reference count will
			 * come to zero, and _callbackMapSuite will be destroyed. 
			 */
		}
	};
	typedef std::shared_ptr<TCPFPZKConsistencyProxy> TCPFPZKConsistencyProxyPtr;
}

#endif
