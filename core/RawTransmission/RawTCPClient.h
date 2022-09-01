#ifndef FPNN_Raw_TCP_Client_H
#define FPNN_Raw_TCP_Client_H

#include "NetworkUtility.h"
#include "RawClientInterface.h"

namespace fpnn
{
	class RawTCPClient;
	typedef std::shared_ptr<RawTCPClient> RawTCPClientPtr;

	//=================================================================//
	//- Raw TCP Client:
	//=================================================================//
	class RawTCPClient: public RawClient, public std::enable_shared_from_this<RawTCPClient>
	{
	private:
		int _ioChunkSize;

	private:
		int connectIPv4Address(ConnectionInfoPtr currConnInfo);
		int connectIPv6Address(ConnectionInfoPtr currConnInfo);
		ConnectionInfoPtr perpareConnection(int socket);

		RawTCPClient(const std::string& host, int port, bool autoReconnect = true);

	public:
		virtual ~RawTCPClient() {}

		/*===============================================================================
		  Call by Developer. Configure Function.
		=============================================================================== */
		inline void setIOChunckSize(int ioChunkSize) { _ioChunkSize = ioChunkSize; }
		/*===============================================================================
		  Call by Developer.
		=============================================================================== */
		virtual bool connect();

		virtual bool sendData(std::string* data);

		inline static RawTCPClientPtr createClient(const std::string& host, int port, bool autoReconnect = true)
		{
			return RawTCPClientPtr(new RawTCPClient(host, port, autoReconnect));
		}
		inline static RawTCPClientPtr createClient(const std::string& endpoint, bool autoReconnect = true)
		{
			std::string host;
			int port;

			if (!parseAddress(endpoint, host, port))
				return nullptr;

			return RawTCPClientPtr(new RawTCPClient(host, port, autoReconnect));
		}
	};
}

#endif