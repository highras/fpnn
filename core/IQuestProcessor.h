#ifndef FPNN_Quest_Processor_Interface_H
#define FPNN_Quest_Processor_Interface_H

#include <memory>
#include <sstream>
#include <unordered_map>
#include <netinet/in.h>
#include "FPReader.h"
#include "FPWriter.h"
#include "AnswerCallbacks.h"
#include "ConcurrentSenderInterface.h"
#include "NetworkUtility.h"
#include "Statistics.h"
#include "net.h"

namespace fpnn
{
	class IQuestProcessor;
	class ConnectionInfo;
	class QuestSender;
	class IAsyncAnswer;
	typedef std::shared_ptr<IQuestProcessor> IQuestProcessorPtr;
	typedef std::shared_ptr<ConnectionInfo> ConnectionInfoPtr;
	typedef std::shared_ptr<QuestSender> QuestSenderPtr;
	typedef std::shared_ptr<IAsyncAnswer> IAsyncAnswerPtr;

	//=================================================================//
	//- Connection Info:
	//-     Just basic info to identify a connection.
	//=================================================================//
	class ConnectionInfo
	{
	private:
		static std::atomic<uint64_t> uniqueIdBase;
	private:
		friend class TCPClient;
		friend class TCPEpollServer;
		friend class TCPBasicConnection;
		friend class IQuestProcessor;
		friend class UDPRecvBuffer;
		friend class UDPSendBuffer;
		friend class UDPQuestSender;
		friend class UDPClientConnection;
		friend class UDPServerConnection;
		friend class UDPEpollServer;
		friend class UDPClient;
		friend class Client;
		friend class RawClient;
		friend class RawTCPClient;
		friend class RawUDPClient;

		std::mutex* _mutex;		//-- only for sync quest to set answer map.
		uint64_t _uniqueId;
		bool _isSSL;
		bool _isTCP;
		bool _isIPv4;
		bool _encrypted;
		bool _isWebSocket;
		bool _internalAddress;
		bool _serverConnection;

		//-- Only use for UDP.
		uint8_t* _socketAddress;

		ConnectionInfo(int socket_, int port_, const std::string& ip_, bool isIPv4, bool serverConnection):
			_mutex(0), _isSSL(false), _isTCP(true), _isIPv4(isIPv4), _encrypted(false), _isWebSocket(false),
			_internalAddress(false),_serverConnection(serverConnection), _socketAddress(NULL),
			token(0), socket(socket_), port(port_), ip(ip_)
		{
			bool maybeIPv4 = isIPv4;
			const char* ipPtr = ip.c_str();

			if (!isIPv4 && (strncasecmp("::ffff:", ipPtr, 7) == 0))
			{
				maybeIPv4 = true;
				ipPtr += 7;
			}

			if (maybeIPv4)
			{
				_internalAddress = !ipv4_is_external_s(ipPtr);
				ipv4 = ipv4_aton(ip.c_str());
			}
			else
				_internalAddress = NetworkUtil::isPrivateIPv6(ip);

			_uniqueId = ++uniqueIdBase;
		}

		void changeToUDP(uint8_t* socketAddress)	//-- UDP Server Used.
		{
			_isTCP = false;

			if (_socketAddress)
				free(_socketAddress);

			int len = _isIPv4 ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6);
			_socketAddress = (uint8_t*)malloc(len);
			memcpy(_socketAddress, socketAddress, len);
		}

		void changeToUDP(int newSocket, uint8_t* socketAddress)	//-- UDP Client Used.
		{
			_isTCP = false;
			socket = newSocket;
			
			if (_socketAddress)
				free(_socketAddress);

			_socketAddress = socketAddress;
		}

		void changeToUDP()
		{
			_isTCP = false;
		}

	public:
		uint64_t token;
		int socket;
		int port;
		std::string ip;
		uint32_t ipv4;

		std::string str() const {
			std::stringstream ss;
			ss << "Socket: " << socket << ", address: " << ip << ":" <<port;
			return ss.str();
		}
		
		std::string endpoint() const {
			return ip + ":" + std::to_string(port);
		}
		
		uint64_t getPortIPv4() const {
			uint64_t p = port;
			return (p << 32) | ipv4;
		}

		inline bool isSSL() const { return _isSSL; }
		inline bool isTCP() const { return _isTCP; }
		inline bool isUDP() const { return !_isTCP; }
		inline bool isIPv4() const { return _isIPv4; }
		inline bool isIPv6() const { return !_isIPv4; }
		inline bool isEncrypted() const { return _encrypted || _isSSL; }
		inline bool isPrivateIP() const { return _internalAddress; }
		inline bool isWebSocket() const { return _isWebSocket; }
		inline bool isServerConnection() const { return _serverConnection; }
		inline uint64_t getToken() const { return token; }
		inline uint64_t uniqueId() const { return _uniqueId; }
		std::string getIP() const { return ip; }

		ConnectionInfo(const ConnectionInfo& ci): _mutex(0), _uniqueId(ci._uniqueId),
			_isSSL(ci._isSSL), _isTCP(ci._isTCP), _isIPv4(ci._isIPv4),
			_encrypted(ci._encrypted), _isWebSocket(ci._isWebSocket), _internalAddress(ci._internalAddress),
			_serverConnection(ci._serverConnection), _socketAddress(NULL),
			token(ci.token), socket(ci.socket), port(ci.port), ip(ci.ip), ipv4(ci.ipv4)
		{
			if (ci._socketAddress)
			{
				int len = ci._isIPv4 ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6);
				_socketAddress = (uint8_t*)malloc(len);
				memcpy(_socketAddress, ci._socketAddress, len);
			}
		}

		~ConnectionInfo()
		{
			if (_socketAddress)
				free(_socketAddress);
		}
	};

	//=================================================================//
	//- Quest Sender:
	//-     Just using for send quest to peer.
	//-		For all sendQuest() interfaces:
	//-     	If throw exception or return false, caller must free quest & callback.
	//-			If return true, or FPAnswerPtr is NULL, don't free quest & callback.
	//=================================================================//
	class QuestSender
	{
	public:
		virtual ~QuestSender() {}
		/**
			All SendQuest() & SendQuestEx():
				If return false, caller must free quest & callback.
				If return true, don't free quest & callback.

				timeout for sendQuest() is in seconds, timeoutMsec for sendQuestEx() is in milliseconds.
		*/
		virtual FPAnswerPtr sendQuest(FPQuestPtr quest, int timeout = 0) = 0;
		virtual bool sendQuest(FPQuestPtr quest, AnswerCallback* callback, int timeout = 0) = 0;
		virtual bool sendQuest(FPQuestPtr quest, std::function<void (FPAnswerPtr answer, int errorCode)> task, int timeout = 0) = 0;

		virtual FPAnswerPtr sendQuestEx(FPQuestPtr quest, bool discardable, int timeoutMsec = 0) = 0;
		virtual bool sendQuestEx(FPQuestPtr quest, AnswerCallback* callback, bool discardable, int timeoutMsec = 0) = 0;
		virtual bool sendQuestEx(FPQuestPtr quest, std::function<void (FPAnswerPtr answer, int errorCode)> task, bool discardable, int timeoutMsec = 0) = 0;
	};

	//=================================================================//
	//- Async Answer Interface:
	//-     Just using for delay send answer after the noraml flow.
	//-     Before then end of normal flow, gen the instance of Async Answer Interface,
	//-     then, return NULL or nullptr as the return value for normal flow in IQuestProcessor.
	//=================================================================//
	class IAsyncAnswer
	{
	public:
		virtual ~IAsyncAnswer() {}

		virtual const FPQuestPtr getQuest() = 0;
		/** need release answer anyway. */
		virtual bool sendAnswer(FPAnswerPtr) = 0;
		virtual bool isSent() = 0;

		//-- Extends
		virtual void cacheTraceInfo(const std::string&) = 0;
		virtual void cacheTraceInfo(const char *) = 0;

		virtual bool sendErrorAnswer(int code = 0, const std::string& ex = "")
		{
			FPAnswerPtr answer = FpnnErrorAnswer(getQuest(), code, ex);
			return sendAnswer(answer);
		}
		virtual bool sendErrorAnswer(int code = 0, const char* ex = "")
		{
			FPAnswerPtr answer = FpnnErrorAnswer(getQuest(), code, ex);
			return sendAnswer(answer);
		}
		virtual bool sendEmptyAnswer()
		{
			FPAnswerPtr answer = FPAWriter::emptyAnswer(getQuest());
			return sendAnswer(answer);
		}
	};

	//=================================================================//
	//- Quest Processor Interface (Quest Handler Interface):
	//-     "Call by framwwork." section are used internal by framework.
	//-     "Call by Developer." section are using by developer.
	//=================================================================//
	class IQuestProcessor
	{
	private:
		IConcurrentSender* _concurrentSender;
		IConcurrentUDPSender* _concurrentUDPSender;

	private:
		inline bool standardInterface(const ConnectionInfo& connInfo) const
		{
			return connInfo._isTCP || !(connInfo._serverConnection);
		}

	protected:
		enum MethodAttribute { EncryptOnly = 0x1, PrivateIPOnly = 0x2 };	//-- Only for methods attributes.

	public:
		IQuestProcessor(): _concurrentSender(0), _concurrentUDPSender(0) {}
		virtual ~IQuestProcessor() {}

		/*===============================================================================
		  Call by framwwork.
		=============================================================================== */
		/** Call by framework. */
		void initAnswerStatus(ConnectionInfoPtr connInfo, FPQuestPtr quest);
		bool getQuestAnsweredStatus();
		bool finishAnswerStatus();	//-- return answer status

		virtual void setConcurrentSender(IConcurrentSender* concurrentSender) { if (!_concurrentSender) _concurrentSender = concurrentSender; }
		virtual void setConcurrentSender(IConcurrentUDPSender* concurrentUDPSender) { if (!_concurrentUDPSender) _concurrentUDPSender = concurrentUDPSender; }
		virtual FPAnswerPtr processQuest(const FPReaderPtr args, const FPQuestPtr quest, const ConnectionInfo& connectionInfo) = 0;
		virtual const char* getCompiledDate() = 0;
		virtual const char* getCompiledTime() = 0;

		/*===============================================================================
		  Call by Developer.
		=============================================================================== */
	protected:
		/*
			The following four methods ONLY can be called in server interfaces function which is called by FPNN framework.
		*/
		bool sendAnswer(FPAnswerPtr answer);
		inline bool sendAnswer(const FPQuestPtr, FPAnswerPtr answer)	//-- for compatible old version.
		{
			return sendAnswer(answer);
		}
		IAsyncAnswerPtr genAsyncAnswer();
		inline IAsyncAnswerPtr genAsyncAnswer(const FPQuestPtr)	//-- for compatible old version.
		{
			return genAsyncAnswer();
		}

	protected:
		inline FPAnswerPtr illegalAccessRequest(const std::string& method_name, const FPQuestPtr quest, const ConnectionInfo& connInfo)
		{
			if (quest->isTwoWay())
			{
				LOG_ERROR("Illegal access request(twoway): method:%s. %s Info: %s", method_name.c_str(), connInfo.str().c_str(), quest->info().c_str());
				return FpnnErrorAnswer(quest, FPNN_EC_CORE_FORBIDDEN, std::string("Action is Forbidden! ") + connInfo.str());
			}
			else{
				LOG_ERROR("Illegal access request(oneway): method:%s. %s Info: %s", method_name.c_str(), connInfo.str().c_str(), quest->info().c_str());
				return NULL;
			}
		}

	public:
		QuestSenderPtr genQuestSender(const ConnectionInfo& connectionInfo);

		/**
			All SendQuest() & sendQuestEx():
				If return false, caller must free quest & callback.
				If return true, don't free quest & callback.

				timeout for sendQuest() is in seconds, timeoutMsec for sendQuestEx() is in milliseconds.
		*/
		virtual FPAnswerPtr sendQuest(FPQuestPtr quest, int timeout = 0);
		virtual bool sendQuest(FPQuestPtr quest, AnswerCallback* callback, int timeout = 0);
		virtual bool sendQuest(FPQuestPtr quest, std::function<void (FPAnswerPtr answer, int errorCode)> task, int timeout = 0);

		virtual FPAnswerPtr sendQuestEx(FPQuestPtr quest, bool discardable, int timeoutMsec = 0);
		virtual bool sendQuestEx(FPQuestPtr quest, AnswerCallback* callback, bool discardable, int timeoutMsec = 0);
		virtual bool sendQuestEx(FPQuestPtr quest, std::function<void (FPAnswerPtr answer, int errorCode)> task, bool discardable, int timeoutMsec = 0);

		/*===============================================================================
		  Event Hook. (Common)
		=============================================================================== */
	public:
		virtual void connected(const ConnectionInfo&) {}
		virtual void connectionWillClose(const ConnectionInfo& connInfo, bool closeByError)
		{
			(void)connInfo;
			(void)closeByError;
		}
		virtual FPAnswerPtr unknownMethod(const std::string& method_name, const FPReaderPtr args, const FPQuestPtr quest, const ConnectionInfo& connInfo) 
		{
			if (quest->isTwoWay()){
				LOG_ERROR("TwoWay Quest, UNKNOWN method:%s. %s Info: %s", method_name.c_str(), connInfo.str().c_str(), quest->info().c_str());
				return FpnnErrorAnswer(quest, FPNN_EC_CORE_UNKNOWN_METHOD, std::string("Unknow method:") + method_name + ", " + connInfo.str());
			}
			else{
				LOG_ERROR("OneWay Quest, UNKNOWN method:%s. %s Info: %s", method_name.c_str(), connInfo.str().c_str(), quest->info().c_str());
				return NULL;
			}
		}

		/*===============================================================================
		  Event Hook. (Server Only)
		=============================================================================== */
		// -- for start user thread
		virtual void start(){}					//-- ONLY used by server

		//call user defined function before server exit
		virtual void serverWillStop() {}		//-- ONLY used by server

		//call user defined function after server exit
		virtual void serverStopped() {}			//-- ONLY used by server

		//status only for ops to find if server is ok
		virtual bool status() { return true; }	//-- ONLY used by server
		//-- Append or excute default interfaces. Just be called when it as server quest processor. MUST return a JSON dict (maybe array).
		virtual std::string infos() { return "{}"; }		//-- ONLY used by server
		virtual void tune(const std::string& key, std::string& value) {}	//-- ONLY used by server
	};

	#define QuestProcessorClassPrivateFields(processor_name)	\
	public:	\
		typedef FPAnswerPtr (processor_name::* MethodFunc)(const FPReaderPtr, const FPQuestPtr, const ConnectionInfo&);\
	private:\
		struct MethodInfo { MethodFunc func; uint32_t attributes; }; \
		std::unordered_map<std::string, MethodInfo> _methodMap;\

	#define QuestProcessorClassBasicPublicFuncs	\
		virtual const char* getCompiledDate() { return __DATE__; } \
		virtual const char* getCompiledTime() { return __TIME__; } \
		inline void registerMethod(const std::string& method_name, MethodFunc func, uint32_t attributes = 0)	{ \
			_methodMap[method_name] = {func, attributes}; \
			Statistics::initMethod(method_name); \
		} \
		virtual FPAnswerPtr processQuest(const FPReaderPtr args, const FPQuestPtr quest, const ConnectionInfo& connInfo)	\
		{	\
			const std::string& method = quest->method();	\
			auto iter = _methodMap.find(method);	\
			if (iter != _methodMap.end()) {	\
				if ((iter->second.attributes & EncryptOnly) && connInfo.isEncrypted() == false)	\
					return illegalAccessRequest(method, quest, connInfo); \
				if ((iter->second.attributes & PrivateIPOnly) && connInfo.isPrivateIP() == false)	\
					return illegalAccessRequest(method, quest, connInfo); \
				return (this->*(iter->second.func))(args, quest, connInfo); }	\
			else  \
				return unknownMethod(method, args, quest, connInfo);	\
		}
}

#endif
