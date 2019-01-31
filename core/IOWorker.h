#ifndef FPNN_IO_Worker_H
#define FPNN_IO_Worker_H

#include "ConnectionReclaimer.h"
#include "IQuestProcessor.h"
#include "OpenSSLModule.h"
#include "IOBuffer.h"
#include "msec.h"

namespace fpnn
{
	class BasicConnection: public IReleaseable
	{
	public:
		enum ConnectionType
		{
			TCPServerConnectionType,
			TCPClientConnectionType,
			UDPClientConnectionType
		};

	public:
		ConnectionInfoPtr _connectionInfo;
		
		int64_t _activeTime;
		std::atomic<int> _refCount;
		std::atomic<bool> _needRecv;
		std::atomic<bool> _needSend;

		std::unordered_map<uint32_t, BasicAnswerCallback*> _callbackMap;

	public:
		BasicConnection(ConnectionInfoPtr connectionInfo): _connectionInfo(connectionInfo), _refCount(0), _needRecv(false), _needSend(false)
		{
			_connectionInfo->token = (uint64_t)this;	//-- if use Virtual Derive, must redo this in subclass constructor.
			_activeTime = slack_real_sec();
		}

		virtual ~BasicConnection()
		{
			close(_connectionInfo->socket);
		}

		virtual bool waitForAllEvents() = 0;
		virtual enum ConnectionType connectionType() = 0;
		virtual bool releaseable() { return (_refCount == 0); }

		inline void setNeedSendFlag() { _needSend = true; }
		inline void setNeedRecvFlag() { _needRecv = true; }
		inline int socket() const { return _connectionInfo->socket; }
		virtual int send(bool& needWaitSendEvent, std::string* data = NULL) = 0;
	};

	class TCPBasicConnection: public BasicConnection
	{
	public:
		RecvBuffer _recvBuffer;
		SendBuffer _sendBuffer;
		SSLContext _sslContext;

	public:
		inline bool recvPackage(bool& needNextEvent) { return _recvBuffer.recvPackage(_connectionInfo->socket, needNextEvent); }
		inline bool entryEncryptMode(uint8_t *key, size_t key_len, uint8_t *iv, bool streamMode)
		{
			if (_sslContext._ssl)
			{
				LOG_ERROR("Entry encrypt mode failed. Current connection is SSL/TSL connection. Double-encrypting is unnecessary. Connection will be closed by server. %s", _connectionInfo->str().c_str());
				return false;
			}
			if (_recvBuffer.entryEncryptMode(key, key_len, iv, streamMode) == false)
			{
				LOG_ERROR("Entry encrypt mode failed. Entry cmd is not the first cmd. Connection will be closed by server. %s", _connectionInfo->str().c_str());
				return false;
			}
			if (_sendBuffer.entryEncryptMode(key, key_len, iv, streamMode) == false)
			{
				LOG_ERROR("Entry encrypt mode failed. Connection has bytes sending. Connection will be closed by server. %s", _connectionInfo->str().c_str());
				return false;
			}
			_connectionInfo->_encrypted = true;
			return true;
		}
		inline void encryptAfterFirstPackageSent() { _sendBuffer.encryptAfterFirstPackage(); }
		void entryWebSocketMode(std::string* data = NULL) //-- if data not NULL, MUST set additionalSend in TCPServerIOWorker::read().
		{
			_recvBuffer.entryWebSocketMode();
			_sendBuffer.entryWebSocketMode(data);
			_connectionInfo->_isWebSocket = true;
		}
		bool prepareSSL(bool server)
		{
			if (_sslContext.init(_connectionInfo->socket, server))
			{
				_recvBuffer.enrtySSLMode(&_sslContext);
				_sendBuffer.enrtySSLMode(&_sslContext);
				return true;
			}
			else
				return false;
		}

		inline bool isEncrypted() { return _connectionInfo->isEncrypted(); }
		inline bool isWebSocket() { return _connectionInfo->_isWebSocket; }

		virtual int send(bool& needWaitSendEvent, std::string* data = NULL)
		{
			bool actualSent = false;
			//-- _activeTime vaule maybe in confusion after concurrent Sending on one connection.
			//-- But the probability is very low even server with high load. So, it hasn't be adjusted at current.
			_activeTime = slack_real_sec();
			return _sendBuffer.send(_connectionInfo->socket, needWaitSendEvent, actualSent, data);
		}
		
		TCPBasicConnection(std::mutex* mutex, int ioChunkSize, ConnectionInfoPtr connectionInfo):
			BasicConnection(connectionInfo), _recvBuffer(ioChunkSize, mutex), _sendBuffer(mutex)
		{
			_connectionInfo->token = (uint64_t)this;	//-- if use Virtual Derive, must redo this in subclass constructor.
			_connectionInfo->_mutex = mutex;
		}

		virtual ~TCPBasicConnection() { _sslContext.close(); }
	};
}

#endif
