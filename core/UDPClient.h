#ifndef FPNN_UDP_Client_H
#define FPNN_UDP_Client_H

#include "HostLookup.h"
#include "StringUtil.h"
#include "NetworkUtility.h"
#include "UDPClientIOWorker.h"
#include "Config.h"
#include "ClientInterface.h"

namespace fpnn
{
	class UDPClient;
	typedef std::shared_ptr<UDPClient> UDPClientPtr;

	//=================================================================//
	//- UDP Client:
	//=================================================================//
	class UDPClient: public Client, public std::enable_shared_from_this<UDPClient>
	{
	private:
		int connectIPv4Address(ConnectionInfoPtr currConnInfo);
		int connectIPv6Address(ConnectionInfoPtr currConnInfo);
		bool perpareConnection(ConnectionInfoPtr currConnInfo);

		UDPClient(const std::string& host, int port, bool autoReconnect = true);

	public:
		virtual ~UDPClient() {}

		/*===============================================================================
		  Call by framwwork.
		=============================================================================== */
		void dealQuest(FPQuestPtr quest, ConnectionInfoPtr connectionInfo);		//-- must done in thread pool or other thread.
		/*===============================================================================
		  Call by Developer.
		=============================================================================== */
		virtual bool connect();

		inline static UDPClientPtr createClient(const std::string& host, int port, bool autoReconnect = true)
		{
			return UDPClientPtr(new UDPClient(host, port, autoReconnect));
		}
		inline static UDPClientPtr createClient(const std::string& endpoint, bool autoReconnect = true)
		{
			std::string host;
			int port;

			if (!parseAddress(endpoint, host, port))
				return nullptr;

			return UDPClientPtr(new UDPClient(host, port, autoReconnect));
		}
	};
}

#endif