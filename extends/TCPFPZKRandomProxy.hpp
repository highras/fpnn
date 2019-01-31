#ifndef FPNN_TCP_FPZK_Random_Proxy_H
#define FPNN_TCP_FPZK_Random_Proxy_H

#include "msec.h"
#include "ExtendsException.hpp"
#include "TCPFPZKProxyCore.hpp"

namespace fpnn
{
	class TCPFPZKRandomProxy: public TCPFPZKProxyCore
	{
	public:
		//-- If questTimeoutSeconds less then zero, mean using global settings.
		TCPFPZKRandomProxy(FPZKClientPtr fpzkClient, const std::string& serviceName, const std::string& cluster = "", int64_t questTimeoutSeconds = -1):
			TCPFPZKProxyCore(fpzkClient, serviceName, cluster, questTimeoutSeconds)
		{
		}

		TCPClientPtr getClient(bool connect)
		{
			std::unique_lock<std::mutex> lck(_mutex);
			checkRevision("FPZK Random Proxy");

			while (_endpoints.size())
			{
				/* slack_real_msec() + ((int64_t)(&lck) >> 16):
				 *    Avoid tow threads call the same proxy at same time, and calculate to same client.
				 *    Just for optimizing load balancing.
				 */
				int64_t index = (slack_real_msec() + ((int64_t)(&lck) >> 16)) % (int64_t)_endpoints.size();

				auto iter = _clients.find(_endpoints[index]);
				if (iter != _clients.end())
				{
					TCPClientPtr client = iter->second;
					if (!connect || client->connected() || client->reconnect())
						return client;
				}
				else
				{
					TCPClientPtr client = createTCPClient(_endpoints[index], connect);
					if (client)
						return client;
				}
				_endpoints.erase(_endpoints.begin() + index);
			}

			return nullptr;
		}

		/**
			All SendQuest():
				If return false, caller must free quest & callback.
				If return true, don't free quest & callback.

			timeout in seconds.
		*/
		FPAnswerPtr sendQuest(FPQuestPtr quest, int timeout = 0)
		{
			TCPClientPtr client = getClient(true);
			if (client)
				return client->sendQuest(quest, timeout);
			else
				throw FPNN_ERROR_MSG(FpnnInvalidClientError, "Invalid TCP client.");
		}
		bool sendQuest(FPQuestPtr quest, AnswerCallback* callback, int timeout = 0)
		{
			TCPClientPtr client = getClient(true);
			if (client)
				return client->sendQuest(quest, callback, timeout);
			else
				return false;
		}
		bool sendQuest(FPQuestPtr quest, std::function<void (FPAnswerPtr answer, int errorCode)> task, int timeout = 0)
		{
			TCPClientPtr client = getClient(true);
			if (client)
				return client->sendQuest(quest, std::move(task), timeout);
			else
				return false;
		}
	};
	typedef std::shared_ptr<TCPFPZKRandomProxy> TCPFPZKRandomProxyPtr;
}

#endif
