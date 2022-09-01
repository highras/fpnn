#ifndef FPNN_Raw_UDP_Client_H
#define FPNN_Raw_UDP_Client_H

#include "NetworkUtility.h"
#include "RawClientInterface.h"

namespace fpnn
{
	class RawUDPClient;
	typedef std::shared_ptr<RawUDPClient> RawUDPClientPtr;

	//=================================================================//
	//- UDP Client:
	//=================================================================//
	class RawUDPClient: public RawClient, public std::enable_shared_from_this<RawUDPClient>
	{
	private:
		int connectIPv4Address(ConnectionInfoPtr currConnInfo);
		int connectIPv6Address(ConnectionInfoPtr currConnInfo);
		bool perpareConnection(ConnectionInfoPtr currConnInfo);

		RawUDPClient(const std::string& host, int port, bool autoReconnect = true);

	public:
		virtual ~RawUDPClient() { close(); }

		inline void enableRawDataProcessPool(int32_t initCount, int32_t perAppendCount, int32_t perfectCount, int32_t maxCount)
		{
			if (_rawDataProcessPool)
				return;
			
			_rawDataProcessPool = new TaskThreadPool;
			_rawDataProcessPool->init(initCount, perAppendCount, perfectCount, maxCount);
		}

		/*===============================================================================
		  Call by Developer.
		=============================================================================== */
		virtual bool connect();
		virtual void close();

		virtual bool sendData(std::string* data);

		inline static RawUDPClientPtr createClient(const std::string& host, int port, bool autoReconnect = true)
		{
			return RawUDPClientPtr(new RawUDPClient(host, port, autoReconnect));
		}
		inline static RawUDPClientPtr createClient(const std::string& endpoint, bool autoReconnect = true)
		{
			std::string host;
			int port;

			if (!parseAddress(endpoint, host, port))
				return nullptr;

			return RawUDPClientPtr(new RawUDPClient(host, port, autoReconnect));
		}
	};
}

#endif