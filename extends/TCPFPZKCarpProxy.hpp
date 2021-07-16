#ifndef FPNN_TCP_FPZK_Carp_Proxy_H
#define FPNN_TCP_FPZK_Carp_Proxy_H

#include "FunCarpSequence.h"
#include "ExtendsException.hpp"
#include "TCPFPZKProxyCore.hpp"

namespace fpnn
{
	class TCPFPZKCarpProxy: public TCPFPZKProxyCore
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
			checkRevision("FPZK Carp Proxy");

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

		template<typename K>
		bool internalGetClients(const std::vector<K>& keys, bool connect, std::map<TCPClientPtr, std::set<K>>& result)
		{
			std::unique_lock<std::mutex> lck(_mutex);
			checkRevision("FPZK Carp Proxy");

			if (!_funcarp)
				return false;
			else if (!_funcarp->size())
				return false;

			for (const auto& key: keys)
			{
				int index = _funcarp->which(key);
				if (index < 0 || index >= (int)_endpoints.size())
				{
					continue;
				}

				auto iter = _clients.find(_endpoints[index]);
				if (iter != _clients.end())
				{
					TCPClientPtr client = iter->second;
					if (connect && !client->connected())
						client->reconnect();

					result[client].insert(key);
					continue;
				}

				TCPClientPtr client = createTCPClient(_endpoints[index], connect);
				if (client)
					result[client].insert(key);
			}

			if (result.empty())
				return false;

			return true;
		}

		template<typename K>
		bool internalGetClients(const std::vector<K>& keys, bool connect, std::map<TCPClientPtr, std::vector<K>>& result)
		{
			std::unique_lock<std::mutex> lck(_mutex);
			checkRevision("FPZK Carp Proxy");

			if (!_funcarp)
				return false;
			else if (!_funcarp->size())
				return false;

			for (const auto& key: keys)
			{
				int index = _funcarp->which(key);
				if (index < 0 || index >= (int)_endpoints.size())
				{
					continue;
				}

				auto iter = _clients.find(_endpoints[index]);
				if (iter != _clients.end())
				{
					TCPClientPtr client = iter->second;
					if (connect && !client->connected())
						client->reconnect();

					result[client].push_back(key);
					continue;
				}

				TCPClientPtr client = createTCPClient(_endpoints[index], connect);
				if (client)
					result[client].push_back(key);
			}

			if (result.empty())
				return false;

			return true;
		}

		template<typename K>
		bool internalGetClients(const std::set<K>& keys, bool connect, std::map<TCPClientPtr, std::set<K>>& result)
		{
			std::unique_lock<std::mutex> lck(_mutex);
			checkRevision("FPZK Carp Proxy");

			if (!_funcarp)
				return false;
			else if (!_funcarp->size())
				return false;

			for (const auto& key: keys)
			{
				int index = _funcarp->which(key);
				if (index < 0 || index >= (int)_endpoints.size())
				{
					continue;
				}

				auto iter = _clients.find(_endpoints[index]);
				if (iter != _clients.end())
				{
					TCPClientPtr client = iter->second;
					if (connect && !client->connected())
						client->reconnect();

					result[client].insert(key);
					continue;
				}

				TCPClientPtr client = createTCPClient(_endpoints[index], connect);
				if (client)
					result[client].insert(key);
			}

			if (result.empty())
				return false;

			return true;
		}

	public:
		//-- If questTimeoutSeconds less then zero, mean using global settings.
		TCPFPZKCarpProxy(FPZKClientPtr fpzkClient, const std::string& serviceName, const std::string& cluster = "", int64_t questTimeoutSeconds = -1, uint32_t keymask = 0):
			TCPFPZKProxyCore(fpzkClient, serviceName, cluster, questTimeoutSeconds), _keymask(keymask)
		{
		}
		virtual ~TCPFPZKCarpProxy() {}

		TCPClientPtr getClient(int64_t key, bool connect)
		{
			return internalGetClient(key, connect);
		}

		TCPClientPtr getClient(const std::string& key, bool connect)
		{
			return internalGetClient(key, connect);
		}

		bool getClients(const std::set<int64_t>& keys, bool connect, std::map<TCPClientPtr, std::set<int64_t>>& result){
			return internalGetClients(keys, connect, result);
		}

		bool getClients(const std::set<std::string>& keys, bool connect, std::map<TCPClientPtr, std::set<std::string>>& result){
			return internalGetClients(keys, connect, result);
		}

		bool getClients(const std::vector<int64_t>& keys, bool connect, std::map<TCPClientPtr, std::vector<int64_t>>& result){
			return internalGetClients(keys, connect, result);
		}

		bool getClients(const std::vector<std::string>& keys, bool connect, std::map<TCPClientPtr, std::vector<std::string>>& result){
			return internalGetClients(keys, connect, result);
		}

		bool getClients(const std::vector<int64_t>& keys, bool connect, std::map<TCPClientPtr, std::set<int64_t>>& result)
		{
			return internalGetClients(keys, connect, result);
		}

		bool getClients(const std::vector<std::string>& keys, bool connect, std::map<TCPClientPtr, std::set<std::string>>& result)
		{
			return internalGetClients(keys, connect, result);
		}

		bool connectAll(){
			std::unique_lock<std::mutex> lck(_mutex);
			checkRevision("FPZK Carp Proxy");

			if (!_funcarp)
				return false;
			else if (!_funcarp->size())
				return false;

			for (size_t i = 0; i < _endpoints.size(); ++i)
			{
				createTCPClient(_endpoints[i], true);
			}

			return true;
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
		std::map<std::string, TCPClientPtr> getAllClients(){
			std::unique_lock<std::mutex> lck(_mutex);
			return 	_clients;
		}
		size_t count(){
			std::unique_lock<std::mutex> lck(_mutex);
			return 	_clients.size();
		}
	};
	typedef std::shared_ptr<TCPFPZKCarpProxy> TCPFPZKCarpProxyPtr;
}

#endif
