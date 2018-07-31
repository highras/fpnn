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
		friend class TCPClient;
		friend class TCPEpollServer;
		friend class TCPBasicConnection;
		friend class IQuestProcessor;
		friend class UDPRecvBuffer;
		friend class UDPSendBuffer;
		friend class UDPClient;
		friend class Client;

		std::mutex* _mutex;		//-- only for sync quest to set answer map.
		bool _isTCP;
		bool _isIPv4;
		bool _encrypted;
		bool _isWebSocket;
		bool _internalAddress;
		bool _serverConnection;

		//-- Only use for UDP.
		uint8_t* _socketAddress;

		ConnectionInfo(int socket_, int port_, const std::string& ip_, bool isIPv4, bool serverConnection): _mutex(0), _isTCP(true), _isIPv4(isIPv4),
			_encrypted(false), _isWebSocket(false), _internalAddress(false), _serverConnection(serverConnection), _socketAddress(NULL),
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
		}

		void changToUDP(uint8_t* socketAddress, uint64_t token_)	//-- UDP Server Used.
		{
			_isTCP = false;
			token = token_;
			_socketAddress = socketAddress;
		}

		void changToUDP(int newSocket, uint8_t* socketAddress)	//-- UDP Client Used.
		{
			_isTCP = false;
			socket = newSocket;
			
			if (_socketAddress)
				free(_socketAddress);

			_socketAddress = socketAddress;
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

		inline bool isTCP() const { return _isTCP; }
		inline bool isUDP() const { return !_isTCP; }
		inline bool isIPv4() const { return _isIPv4; }
		inline bool isIPv6() const { return !_isIPv4; }
		inline bool isEncrypted() const { return _encrypted; }
		inline bool isPrivateIP() const { return _internalAddress; }
		inline bool isWebSocket() const { return _isWebSocket; }

		ConnectionInfo(const ConnectionInfo& ci): _mutex(0), _isTCP(ci._isTCP), _isIPv4(ci._isIPv4),
			_encrypted(ci._encrypted), _internalAddress(ci._internalAddress), _serverConnection(ci._serverConnection),
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
			All SendQuest():
				If return false, caller must free quest & callback.
				If return true, don't free quest & callback.
		*/
		virtual FPAnswerPtr sendQuest(FPQuestPtr quest, int timeout = 0) = 0;
		virtual bool sendQuest(FPQuestPtr quest, AnswerCallback* callback, int timeout = 0) = 0;
		virtual bool sendQuest(FPQuestPtr quest, std::function<void (FPAnswerPtr answer, int errorCode)> task, int timeout = 0) = 0;
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
		virtual void cacheTraceInfo(const std::string&, const std::string& raiser = "") = 0;
		virtual void cacheTraceInfo(const char *, const char* raiser = "") = 0;

		virtual bool sendErrorAnswer(int code = 0, const std::string& ex = "", const std::string& raiser = "")
		{
			FPAnswerPtr answer = FpnnErrorAnswer(getQuest(), code, ex + ", " + raiser);
			return sendAnswer(answer);
		}
		virtual bool sendErrorAnswer(int code = 0, const char* ex = "", const char* raiser = "")
		{
			FPAnswerPtr answer = FpnnErrorAnswer(getQuest(), code, std::string(ex) + ", " + std::string(raiser));
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

	public:
		QuestSenderPtr genQuestSender(const ConnectionInfo& connectionInfo);

		/**
			All SendQuest():
				If return false, caller must free quest & callback.
				If return true, don't free quest & callback.
		*/
		virtual FPAnswerPtr sendQuest(const ConnectionInfo& connectionInfo, FPQuestPtr quest, int timeout = 0);
		virtual bool sendQuest(const ConnectionInfo& connectionInfo, FPQuestPtr quest, AnswerCallback* callback, int timeout = 0);
		virtual bool sendQuest(const ConnectionInfo& connectionInfo, FPQuestPtr quest, std::function<void (FPAnswerPtr answer, int errorCode)> task, int timeout = 0);

		/*===============================================================================
		  Event Hook. (Common)
		=============================================================================== */
		virtual void connected(const ConnectionInfo&) {}
		virtual void connectionClose(const ConnectionInfo&) {}  //-- Deprecated. Will be dropped in FPNN.v3.
		virtual void connectionErrorAndWillBeClosed(const ConnectionInfo&) {}  //-- Deprecated. Will be dropped in FPNN.v3.
		virtual void connectionWillClose(const ConnectionInfo& connInfo, bool closeByError)
		{
			if (!closeByError) connectionClose(connInfo);
			else connectionErrorAndWillBeClosed(connInfo);
		}
		virtual FPAnswerPtr unknownMethod(const std::string& method_name, const FPReaderPtr args, const FPQuestPtr quest, const ConnectionInfo& connInfo) 
		{
			if (quest->isTwoWay()){
				return FpnnErrorAnswer(quest, FPNN_EC_CORE_UNKNOWN_METHOD, std::string("Unknow method:") + method_name + ", " + connInfo.str());
			}
			else{
				LOG_ERROR("OneWay Quest, UNKNOWN method:%s. %s", method_name.c_str(), connInfo.str().c_str());
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
					return FpnnErrorAnswer(quest, FPNN_EC_CORE_FORBIDDEN, std::string("Action is Forbidden! ") + connInfo.str()); \
				if ((iter->second.attributes & PrivateIPOnly) && connInfo.isPrivateIP() == false)	\
					return FpnnErrorAnswer(quest, FPNN_EC_CORE_FORBIDDEN, std::string("Action is Forbidden! ") + connInfo.str()); \
				return (this->*(iter->second.func))(args, quest, connInfo); }	\
			else  \
				return unknownMethod(method, args, quest, connInfo);	\
		}
}

#endif
