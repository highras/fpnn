#ifndef FPNN_TCP_Rotatory_Proxy_H
#define FPNN_TCP_Rotatory_Proxy_H

#include "ExtendsException.hpp"
#include "TCPProxyCore.hpp"

namespace fpnn
{
	class TCPRotatoryProxy: public TCPProxyCore
	{
		size_t _index;

		TCPClientPtr getClient(size_t index, bool connect)
		{
			auto iter = _clients.find(_endpoints[index]);
			if (iter != _clients.end())
			{
				TCPClientPtr client = iter->second;
				if (!connect || client->connected() || client->reconnect())
					return client;
			}
			else
			{
				return createTCPClient(_endpoints[index], connect);
			}
			return nullptr;
		}

	public:
		//-- If questTimeoutSeconds less then zero, mean using global settings.
		TCPRotatoryProxy(int64_t questTimeoutSeconds = -1): TCPProxyCore(questTimeoutSeconds), _index(0)
		{
		}

		TCPClientPtr getClient(bool connect)
		{
			std::unique_lock<std::mutex> lck(_mutex);

			_index++;
			if (_index >= _endpoints.size())
				_index = 0;

			for (size_t i = _index; i < _endpoints.size(); i++)
			{
				TCPClientPtr client = getClient(i, connect);
				if (client)
				{
					_index = i;
					return client;
				}
			}

			for (size_t i = 0; i < _index; i++)
			{
				TCPClientPtr client = getClient(i, connect);
				if (client)
				{
					_index = i;
					return client;
				}
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
	typedef std::shared_ptr<TCPRotatoryProxy> TCPRotatoryProxyPtr;
}

#endif