#ifndef FPNN_TCP_PROXY_CORE_H
#define FPNN_TCP_PROXY_CORE_H

#include <map>
#include <mutex>
#include <memory>
#include <vector>
#include "NetworkUtility.h"
#include "ClientEngine.h"
#include "TCPClient.h"

namespace fpnn
{
	//================[ Quest Processor Factory Interface ]==================//
	class IProxyQuestProcessorFactory
	{
	public:
		virtual IQuestProcessorPtr generate(const std::string& host, int port) = 0;
		virtual ~IProxyQuestProcessorFactory() {}
	};
	typedef std::shared_ptr<IProxyQuestProcessorFactory> IProxyQuestProcessorFactoryPtr;

	//================[ TCP Proxy Core ]==================//
	class TCPProxyCore
	{
	protected:
		std::mutex _mutex;

		//============[ Cluster fields ]================//
		std::vector<std::string> _endpoints;
		std::map<std::string, TCPClientPtr> _clients;

		//============[ Per Client fields ]==============//
		int64_t _questTimeout;

		TaskThreadPoolPtr _sharedQuestProcessPool;
		IQuestProcessorPtr _sharedQuestProcessor;

		IProxyQuestProcessorFactoryPtr _questProcessorFactory;

		//============[ Keep alive infos ]==============//
		TCPClientKeepAliveParams* _keepAliveParams;

	protected:
		void checkClientMap()
		{
			std::map<std::string, TCPClientPtr> newMap;
			for (auto& endpoint: _endpoints)
			{
				auto iter = _clients.find(endpoint);
				if (iter != _clients.end())
					newMap[endpoint] = iter->second;
			}

			_clients.swap(newMap);
		}

		TCPClientPtr createTCPClient(const std::string& endpoint, bool connect)
		{
			std::string host;
			int port;

			if (parseAddress(endpoint, host, port))
			{
				TCPClientPtr client = TCPClient::createClient(host, port);

				if (_questTimeout >= 0)
					client->setQuestTimeout(_questTimeout);

				if (_keepAliveParams)
					client->configKeepAliveParams(_keepAliveParams);

				if (_questProcessorFactory)
				{
					IQuestProcessorPtr processor = _questProcessorFactory->generate(host, port);
					processor->setConcurrentSender(ClientEngine::nakedInstance());
					client->setQuestProcessor(processor);
				}
				else if (_sharedQuestProcessor)
				{
					client->setQuestProcessor(_sharedQuestProcessor);
				}

				if (_sharedQuestProcessPool)
					client->setQuestProcessThreadPool(_sharedQuestProcessPool);

				if (!connect || client->connect())
				{
					_clients[endpoint] = client;
					return client;
				}
			}
			return nullptr;
		}

		virtual void afterUpdate() {}

	public:
		//-- If questTimeoutSeconds less then zero, mean using global settings.
		TCPProxyCore(int64_t questTimeoutSeconds = -1): _questTimeout(questTimeoutSeconds), _keepAliveParams(NULL)
		{
		}
		virtual ~TCPProxyCore()
		{
			if (_keepAliveParams)
				delete _keepAliveParams;
		}

		void enablePrivateQuestProcessor(IProxyQuestProcessorFactoryPtr factory)
		{
			_questProcessorFactory = factory;

			std::unique_lock<std::mutex> lck(_mutex);
			for (auto& clientPair: _clients)
			{
				std::string host;
				int port;
				if (parseAddress(clientPair.first, host, port))
				{
					IQuestProcessorPtr processor = _questProcessorFactory->generate(host, port);
					if (processor)
					{
						processor->setConcurrentSender(ClientEngine::nakedInstance());
						clientPair.second->setQuestProcessor(processor);
					}
				}
			}
		}

		void setSharedQuestProcessor(IQuestProcessorPtr sharedQuestProcessor)
		{			
			_sharedQuestProcessor = sharedQuestProcessor;
			_sharedQuestProcessor->setConcurrentSender(ClientEngine::nakedInstance());

			std::unique_lock<std::mutex> lck(_mutex);
			for (auto& clientPair: _clients)
				clientPair.second->setQuestProcessor(_sharedQuestProcessor);
		}

		void setSharedQuestProcessThreadPool(TaskThreadPoolPtr questProcessPool)
		{
			_sharedQuestProcessPool = questProcessPool;

			std::unique_lock<std::mutex> lck(_mutex);
			for (auto& clientPair: _clients)
				clientPair.second->setQuestProcessThreadPool(_sharedQuestProcessPool);
		}

		void keepAlive()
		{
			std::unique_lock<std::mutex> lck(_mutex);

			if (!_keepAliveParams)
			{
				_keepAliveParams = new TCPClientKeepAliveParams;

				_keepAliveParams->pingTimeout = 0;
				_keepAliveParams->pingInterval = Config::Client::KeepAlive::pingInterval;
				_keepAliveParams->maxPingRetryCount = Config::Client::KeepAlive::maxPingRetryCount;
			}
		}
		void setKeepAlivePingTimeout(int seconds)
		{
			keepAlive();
			_keepAliveParams->pingTimeout = seconds * 1000;
		}
		void setKeepAliveInterval(int seconds)
		{
			keepAlive();
			_keepAliveParams->pingInterval = seconds * 1000;
		}
		void setKeepAliveMaxPingRetryCount(int count)
		{
			keepAlive();
			_keepAliveParams->maxPingRetryCount = count;
		}

		void updateEndpoints(const std::vector<std::string>& newEndpoints)	//-- FPZK Proxy needn't using this function.
		{
			std::unique_lock<std::mutex> lck(_mutex);
			_endpoints = newEndpoints;
			checkClientMap();
			afterUpdate();
		}

		virtual bool empty()
		{
			std::unique_lock<std::mutex> lck(_mutex);
			return _endpoints.empty();
		}
		std::vector<std::string> endpoints()
		{
			std::unique_lock<std::mutex> lck(_mutex);
			return _endpoints;
		}
	};
}

#endif
