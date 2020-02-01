#ifndef FPNN_TCP_Broadcast_Proxy_H
#define FPNN_TCP_Broadcast_Proxy_H

#include "TCPProxyCore.hpp"
#include "BroadcastAnswerCallback.hpp"

namespace fpnn
{
	/*
		If using Quest Processor, please set Quest Processor before call updateEndpoints().
	*/
	class TCPBroadcastProxy: virtual public TCPProxyCore
	{
	protected:
		std::string _selfEndpoint;

	private:
		bool broadcast(FPQuestPtr quest, BroadcastAnswerCallback* callback, int timeout)
		{
			std::map<std::string, TCPClientPtr> clients;
			{
				std::unique_lock<std::mutex> lck(_mutex);
				beforeBorcast();

				if (_clients.empty())
					return false;

				clients = _clients;
			}

			//-- For one way quest.
			if (quest->isOneWay())
			{
				for (auto& clientPair: clients)
					clientPair.second->sendQuest(quest, timeout);
				
				return true;
			}

			//-- For two way quest.
			BroadcastCallbackMonitorPtr callbackMonitor(new BroadcastCallbackMonitor(quest, callback));
			
			for (auto& clientPair: clients)
			{
				BroadcastProxyAnswerCallback* bpCallback = new BroadcastProxyAnswerCallback(clientPair.first, callbackMonitor);

				if (!clientPair.second->sendQuest(quest, bpCallback, timeout))
				{
					bpCallback->fillResult(nullptr, FPNN_EC_CORE_SEND_ERROR);
					BasicAnswerCallbackPtr task(bpCallback);

					if (!ClientEngine::wakeUpAnswerCallbackThreadPool(task))
					{
						LOG_ERROR("[Error]Process maybe exiting. Wake up answer callback thread pool of client engine to process broadcast proxy failed callback failed. Run it in current thread.");
						task->run();
					}
				}
			}
			return true;
		}

	protected:
		virtual void beforeBorcast() {}
		virtual void beforeGetClient() {}

		virtual void afterUpdate()
		{
			for (auto& endpoint: _endpoints)
			{
				if (endpoint == _selfEndpoint)
				{
					_clients.erase(endpoint);
					continue;
				}
				
				auto iter = _clients.find(endpoint);
				if (iter == _clients.end())
				{
					createTCPClient(endpoint, false);
				}
			}
		}

	public:
		//-- If questTimeoutSeconds less then zero, mean using global settings.
		TCPBroadcastProxy(int64_t questTimeoutSeconds = -1): TCPProxyCore(questTimeoutSeconds) {}
		virtual ~TCPBroadcastProxy() {}

		void exceptSelf(const std::string& endpoint)
		{
			_selfEndpoint = endpoint;
		}

		virtual bool empty()
		{
			std::unique_lock<std::mutex> lck(_mutex);
			if (_endpoints.size() == 1)
				return _endpoints[0] == _selfEndpoint;
			else 
				return _endpoints.empty();
		}

		TCPClientPtr getClient(const std::string& endpoint, bool connect)
		{
			std::unique_lock<std::mutex> lck(_mutex);
			beforeGetClient();

			auto iter = _clients.find(endpoint);
			if (iter != _clients.end())
			{
				TCPClientPtr client = iter->second;
				if (!connect || client->connected() || client->reconnect())
					return client;
			}
			else
			{
				TCPClientPtr client = createTCPClient(endpoint, connect);
				if (client)
					return client;
			}
			return nullptr;
		}

		/**
			All SendQuest():
				If return false, caller must free quest & callback.
				If return true, don't free quest & callback.

			timeout in seconds.
	
		  * For sync function sendQuest(FPQuestPtr, int):
			If one way quest, returned map always empty.
			If two way quest, returned map included all endpoints' answer. If no available client, returned map is empty.
		*/
		std::map<std::string, FPAnswerPtr> sendQuest(FPQuestPtr quest, int timeout = 0)
		{
			std::map<std::string, FPAnswerPtr> res;

			if (quest->isOneWay())
			{
				broadcast(quest, NULL, timeout);
			}
			else
			{
				std::shared_ptr<SyncedBroadcastAnswerCallback> callback(new SyncedBroadcastAnswerCallback());
				if (broadcast(quest, callback.get(), timeout))
				{
					callback->takeAnswer();
					res.swap(callback->answerMap);
				}
			}

			return res;
		}

		bool sendQuest(FPQuestPtr quest, BroadcastAnswerCallback* callback, int timeout = 0)
		{
			if (quest->isOneWay() && callback)
				return false;

			return broadcast(quest, callback, timeout);
		}

		bool sendQuest(FPQuestPtr quest, std::function<void (const std::string& endpoint, FPAnswerPtr answer, int errorCode)> task, int timeout = 0)
		{
			if (quest->isOneWay())
				return false;

			BroadcastEveryAnswerCallback* callback = new BroadcastEveryAnswerCallback();
			callback->_function = task;
			if (broadcast(quest, callback, timeout))
				return true;
			else
			{
				delete callback;
				return false;
			}
		}

		bool sendQuest(FPQuestPtr quest, std::function<void (std::map<std::string, FPAnswerPtr>& answerMap)> task, int timeout = 0)
		{
			if (quest->isOneWay())
				return false;

			BroadcastCompletedAnswerCallback* callback = new BroadcastCompletedAnswerCallback();
			callback->_function = task;
			if (broadcast(quest, callback, timeout))
				return true;
			else
			{
				delete callback;
				return false;
			}
		}
	};
	typedef std::shared_ptr<TCPBroadcastProxy> TCPBroadcastProxyPtr;
}

#endif

