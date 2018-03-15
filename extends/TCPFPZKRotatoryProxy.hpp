#ifndef FPNN_TCP_FPZK_Rotatory_Proxy_H
#define FPNN_TCP_FPZK_Rotatory_Proxy_H

#include "ExtendsException.hpp"
#include "TCPFPZKProxyCore.hpp"

namespace fpnn
{
	class TCPFPZKRotatoryProxy: public TCPFPZKProxyCore
	{
		size_t _index;

	public:
		//-- If questTimeoutSeconds less then zero, mean using global settings.
		TCPFPZKRotatoryProxy(FPZKClientPtr fpzkClient, const std::string& serviceName, int64_t questTimeoutSeconds = -1):
			TCPFPZKProxyCore(fpzkClient, serviceName, questTimeoutSeconds), _index(0)
		{
		}

		TCPClientPtr getClient(bool connect)
		{
			std::unique_lock<std::mutex> lck(_mutex);
			checkRevision("FPZK Rotatory Proxy");

			_index++;
			while (_endpoints.size())
			{
				if (_index >= _endpoints.size())
					_index = 0;

				auto iter = _clients.find(_endpoints[_index]);
				if (iter != _clients.end())
				{
					TCPClientPtr client = iter->second;
					if (!connect || client->connected() || client->reconnect())
						return client;
				}
				else
				{
					TCPClientPtr client = createTCPClient(_endpoints[_index], connect);
					if (client)
						return client;
				}
				_endpoints.erase(_endpoints.begin() + _index);
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
	typedef std::shared_ptr<TCPFPZKRotatoryProxy> TCPFPZKRotatoryProxyPtr;
}

#endif
