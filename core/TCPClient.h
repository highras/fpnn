#ifndef FPNN_TCP_Client_H
#define FPNN_TCP_Client_H

#include "HostLookup.h"
#include "StringUtil.h"
#include "NetworkUtility.h"
#include "ClientIOWorker.h"
#include "KeyExchange.h"
#include "Config.h"
#include "ClientInterface.h"

namespace fpnn
{
	class TCPClient;
	typedef std::shared_ptr<TCPClient> TCPClientPtr;

	//=================================================================//
	//- TCP Client:
	//=================================================================//
	class TCPClient: public Client, public std::enable_shared_from_this<TCPClient>
	{
	private:
		//----------
		int _AESKeyLen;
		bool _packageEncryptionMode;
		std::string _eccCurve;
		std::string _serverPublicKey;

		bool _sslEnabled;
		//----------
		int _ioChunkSize;

	private:
		int connectIPv4Address(ConnectionInfoPtr currConnInfo);
		int connectIPv6Address(ConnectionInfoPtr currConnInfo);
		ConnectionInfoPtr perpareConnection(int socket, std::string& publicKey);
		bool configEncryptedConnection(TCPClientConnection* connection, std::string& publicKey);

		TCPClient(const std::string& host, int port, bool autoReconnect = true);

	public:
		virtual ~TCPClient() {}

		/*===============================================================================
		  Call by framwwork.
		=============================================================================== */
		void dealQuest(FPQuestPtr quest, ConnectionInfoPtr connectionInfo);		//-- must done in thread pool or other thread.
		/*===============================================================================
		  Call by Developer. Configure Function.
		=============================================================================== */
		bool enableEncryptorByDerData(const std::string &derData, bool packageMode = true, bool reinforce = false);
		bool enableEncryptorByPemData(const std::string &PemData, bool packageMode = true, bool reinforce = false);
		bool enableEncryptorByDerFile(const char *derFilePath, bool packageMode = true, bool reinforce = false);
		bool enableEncryptorByPemFile(const char *pemFilePath, bool packageMode = true, bool reinforce = false);
		inline void enableEncryptor(const std::string& curve, const std::string& peerPublicKey, bool packageMode = true, bool reinforce = false)
		{
			_eccCurve = curve;
			_serverPublicKey = peerPublicKey;
			_packageEncryptionMode = packageMode;
			_AESKeyLen = reinforce ? 32 : 16;
		}

		bool enableSSL(bool enable = true);
		inline void setIOChunckSize(int ioChunkSize) { _ioChunkSize = ioChunkSize; }
		/*===============================================================================
		  Call by Developer.
		=============================================================================== */
		virtual bool connect();

		inline static TCPClientPtr createClient(const std::string& host, int port, bool autoReconnect = true)
		{
			return TCPClientPtr(new TCPClient(host, port, autoReconnect));
		}
		inline static TCPClientPtr createClient(const std::string& endpoint, bool autoReconnect = true)
		{
			std::string host;
			int port;

			if (!parseAddress(endpoint, host, port))
				return nullptr;

			return TCPClientPtr(new TCPClient(host, port, autoReconnect));
		}

		//-- cache for change sync connect to async connect
		//=====================================//
		//--            New Codes            --//
		//=====================================//
		//bool perpareConnectIPv4Address();
		//bool connect(AnswerCallback* callback, int timeout = 0);
	};
}

#endif
