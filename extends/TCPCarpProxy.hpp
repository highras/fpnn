#ifndef FPNN_TCP_Carp_Proxy_H
#define FPNN_TCP_Carp_Proxy_H

#include "FunCarpSequence.h"
#include "ExtendsException.hpp"
#include "TCPProxyCore.hpp"

namespace fpnn
{
	class TCPCarpProxy: public TCPProxyCore
	{
		uint32_t _keymask;
		std::shared_ptr<FunCarpSequence> _funcarp;

	private:
		virtual void afterUpdate()
		{
			_funcarp.reset(new FunCarpSequence(_endpoints, _keymask));
		}

		template<typename K>
		TCPClientPtr internalGetClient(K key, bool connect)
		{
			std::unique_lock<std::mutex> lck(_mutex);

			if (!_funcarp)
				return nullptr;
			else if (!_funcarp->size())
				return nullptr;

			int index = _funcarp->which(key);
			if (index < 0 || index >= (int)_endpoints.size())
				return nullptr;

			auto iter = _clients.find(_endpoints[index]);
			if (iter != _clients.end())
			{
				TCPClientPtr client = iter->second;
				if (connect && !client->connected())
					client->reconnect();

				return client;
			}

			return createTCPClient(_endpoints[index], connect);
		}

	public:
		//-- If questTimeoutSeconds less then zero, mean using global settings.
		TCPCarpProxy(int64_t questTimeoutSeconds = -1, uint32_t keymask = 0): TCPProxyCore(questTimeoutSeconds), _keymask(keymask)
		{
		}
		virtual ~TCPCarpProxy() {}

		TCPClientPtr getClient(int64_t key, bool connect)
		{
			return internalGetClient(key, connect);
		}

		TCPClientPtr getClient(const std::string& key, bool connect)
		{
			return internalGetClient(key, connect);
		}

		/**
			All SendQuest():
				If return false, caller must free quest & callback.
				If return true, don't free quest & callback.

			timeout in seconds.
		*/
		FPAnswerPtr sendQuest(int64_t key, FPQuestPtr quest, int timeout = 0)
		{
			TCPClientPtr client = getClient(key, true);
			if (client)
				return client->sendQuest(quest, timeout);
			else
				throw FPNN_ERROR_MSG(FpnnInvalidClientError, "Invalid TCP client.");
		}
		bool sendQuest(int64_t key, FPQuestPtr quest, AnswerCallback* callback, int timeout = 0)
		{
			TCPClientPtr client = getClient(key, true);
			if (client)
				return client->sendQuest(quest, callback, timeout);
			else
				return false;
		}
		bool sendQuest(int64_t key, FPQuestPtr quest, std::function<void (FPAnswerPtr answer, int errorCode)> task, int timeout = 0)
		{
			TCPClientPtr client = getClient(key, true);
			if (client)
				return client->sendQuest(quest, std::move(task), timeout);
			else
				return false;
		}

		FPAnswerPtr sendQuest(const std::string& key, FPQuestPtr quest, int timeout = 0)
		{
			TCPClientPtr client = getClient(key, true);
			if (client)
				return client->sendQuest(quest, timeout);
			else
				throw FPNN_ERROR_MSG(FpnnInvalidClientError, "Invalid TCP client.");
		}
		bool sendQuest(const std::string& key, FPQuestPtr quest, AnswerCallback* callback, int timeout = 0)
		{
			TCPClientPtr client = getClient(key, true);
			if (client)
				return client->sendQuest(quest, callback, timeout);
			else
				return false;
		}
		bool sendQuest(const std::string& key, FPQuestPtr quest, std::function<void (FPAnswerPtr answer, int errorCode)> task, int timeout = 0)
		{
			TCPClientPtr client = getClient(key, true);
			if (client)
				return client->sendQuest(quest, std::move(task), timeout);
			else
				return false;
		}
	};
	typedef std::shared_ptr<TCPCarpProxy> TCPCarpProxyPtr;
}

#endif
