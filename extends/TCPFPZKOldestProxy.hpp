#ifndef FPNN_TCP_FPZK_Oldest_Proxy_H
#define FPNN_TCP_FPZK_Oldest_Proxy_H

#include "msec.h"
#include "ExtendsException.hpp"
#include "TCPFPZKProxyCore.hpp"

namespace fpnn
{
	class TCPFPZKOldestProxy: public TCPFPZKProxyCore
	{
	private:
		//-- When TCPFPZKProxyCore change the action of _serviceName & cluster, opend the comments.
		//std::string _originalServiceName;
		//std::string _originalCluster;
		std::string _clientEndpoint;
		TCPClientPtr _client;

	protected:
		virtual void afterUpdate()
		{
			//-- When TCPFPZKProxyCore change the action of _serviceName & cluster, opend the comments.
			//std::string endpoint = _fpzkClient->getOldestServiceEndpoint(_originalServiceName, _originalCluster);
			std::string endpoint = _fpzkClient->getOldestServiceEndpoint(_serviceName);
			if (endpoint != _clientEndpoint)
			{
				_clientEndpoint = endpoint;
				_client = nullptr;
			}
		}

	public:
		//-- If questTimeoutSeconds less then zero, mean using global settings.
		TCPFPZKOldestProxy(FPZKClientPtr fpzkClient, const std::string& serviceName, const std::string& cluster = "", int64_t questTimeoutSeconds = -1):
			TCPFPZKProxyCore(fpzkClient, serviceName, cluster, questTimeoutSeconds)//, _originalServiceName(serviceName), _originalCluster(cluster)
		{
		}
		virtual ~TCPFPZKOldestProxy() {}

		std::string getOldestEndpoint()
		{
			std::unique_lock<std::mutex> lck(_mutex);
			checkRevision("FPZK Oldest Proxy");
			return _clientEndpoint;
		}

		bool isMyself(){
			std::unique_lock<std::mutex> lck(_mutex);
			checkRevision("FPZK Oldest Proxy");
			return _clientEndpoint == _fpzkClient->registeredEndpoint();
		}

		TCPClientPtr getClient(bool connect)
		{
			std::unique_lock<std::mutex> lck(_mutex);
			checkRevision("FPZK Oldest Proxy");

			if (_client)
			{
				if (!connect || _client->connected() || _client->reconnect())
						return _client;
			}
			else if (_clientEndpoint.length())
				_client = createTCPClient(_clientEndpoint, connect);

			return _client;
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
	typedef std::shared_ptr<TCPFPZKOldestProxy> TCPFPZKOldestProxyPtr;
}

#endif
