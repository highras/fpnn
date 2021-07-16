#ifndef FPNN_UDP_Client_H
#define FPNN_UDP_Client_H

#include "HostLookup.h"
#include "StringUtil.h"
#include "NetworkUtility.h"
#include "UDPClientConnection.h"
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
		int _MTU;
		bool _keepAlive;
		int _untransmittedSeconds;

	private:
		int connectIPv4Address(ConnectionInfoPtr currConnInfo);
		int connectIPv6Address(ConnectionInfoPtr currConnInfo);
		bool perpareConnection(ConnectionInfoPtr currConnInfo);

		UDPClient(const std::string& host, int port, bool autoReconnect = true);

	public:
		virtual ~UDPClient() { close(); }

		/*===============================================================================
		  Call by framwwork.
		=============================================================================== */
		void dealQuest(FPQuestPtr quest, ConnectionInfoPtr connectionInfo);		//-- must done in thread pool or other thread.
		/*===============================================================================
		  Call by Developer.
		=============================================================================== */
		virtual bool connect();
		virtual void close();

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

		//-- Send Quest
		//-- Timeout in Seconds
		//-- Oneway will deal as discardable, towway will deal as reliable.
		virtual FPAnswerPtr sendQuest(FPQuestPtr quest, int timeout = 0)
		{
			return sendQuestEx(quest, quest->isOneWay(), timeout * 1000);
		}
		virtual bool sendQuest(FPQuestPtr quest, AnswerCallback* callback, int timeout = 0)
		{
			return sendQuestEx(quest, callback, quest->isOneWay(), timeout * 1000);
		}
		virtual bool sendQuest(FPQuestPtr quest, std::function<void (FPAnswerPtr answer, int errorCode)> task, int timeout = 0)
		{
			return sendQuestEx(quest, std::move(task), quest->isOneWay(), timeout * 1000);
		}

		//-- Timeout in milliseconds
		virtual FPAnswerPtr sendQuestEx(FPQuestPtr quest, bool discardable, int timeoutMsec = 0);
		virtual bool sendQuestEx(FPQuestPtr quest, AnswerCallback* callback, bool discardable, int timeoutMsec = 0);
		virtual bool sendQuestEx(FPQuestPtr quest, std::function<void (FPAnswerPtr answer, int errorCode)> task, bool discardable, int timeoutMsec = 0);

		void keepAlive();
		void setUntransmittedSeconds(int untransmittedSeconds);		//-- 0: using default; -1: disable.
		void setMTU(int MTU) { _MTU = MTU; }		//-- Please called before connect() or sendQuest().
	};
}

#endif