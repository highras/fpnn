#ifndef FPNN_IO_Worker_H
#define FPNN_IO_Worker_H

#include "ConnectionReclaimer.h"
#include "IQuestProcessor.h"
#include "IOBuffer.h"
#include "msec.h"

namespace fpnn
{
	class TCPBasicConnection: public IReleaseable
	{
	public:
		enum ConnectionType
		{
			ClientConnectionType,
			ServerConnectionType
		};

	public:
		ConnectionInfoPtr _connectionInfo;
		
		int64_t _activeTime;
		std::atomic<int> _refCount;
		std::atomic<bool> _needRecv;
		std::atomic<bool> _needSend;
		RecvBuffer _recvBuffer;
		SendBuffer _sendBuffer;

		std::unordered_map<uint32_t, BasicAnswerCallback*> _callbackMap;

	public:
		virtual bool waitForAllEvents() = 0;
		virtual enum ConnectionType connectionType() = 0;
		virtual bool releaseable() { return (_refCount == 0); }

		inline bool recvPackage(bool& needNextEvent) { return _recvBuffer.recvPackage(_connectionInfo->socket, needNextEvent); }
		inline bool entryEncryptMode(uint8_t *key, size_t key_len, uint8_t *iv, bool streamMode)
		{
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

		int socket() const { return _connectionInfo->socket; }

		inline void setNeedSendFlag() { _needSend = true; }
		inline void setNeedRecvFlag() { _needRecv = true; }
		inline bool isEncrypted() { return _connectionInfo->_encrypted; }
		inline bool isWebSocket() { return _connectionInfo->_isWebSocket; }

		inline int send(bool& needWaitSendEvent, std::string* data = NULL)
		{
			//-- _activeTime vaule maybe in confusion after concurrent Sending on one connection.
			//-- But the probability is very low even server with high load. So, it hasn't be adjusted at current.
			_activeTime = slack_real_sec();
			return _sendBuffer.send(_connectionInfo->socket, needWaitSendEvent, data);
		}
		
		TCPBasicConnection(std::mutex* mutex, int ioChunkSize, ConnectionInfoPtr connectionInfo):
			_connectionInfo(connectionInfo), _refCount(0), _needRecv(false), _needSend(false), _recvBuffer(ioChunkSize, mutex), _sendBuffer(mutex)
		{
			_connectionInfo->token = (uint64_t)this;	//-- if use Virtual Derive, must redo this in subclass constructor.
			_connectionInfo->_mutex = mutex;
			_activeTime = slack_real_sec();
		}
		
		TCPBasicConnection(std::mutex* mutex, int ioChunkSize, size_t callbackMapSize, ConnectionInfoPtr connectionInfo):
			_connectionInfo(connectionInfo), _refCount(0), _needRecv(false), _needSend(false), _recvBuffer(ioChunkSize, mutex), _sendBuffer(mutex),
			_callbackMap(callbackMapSize)
		{
			_connectionInfo->token = (uint64_t)this;	//-- if use Virtual Deriver, must redo this in subclass constructor.
			_connectionInfo->_mutex = mutex;
			_activeTime = slack_real_sec();
		}

		virtual ~TCPBasicConnection()
		{
			close(_connectionInfo->socket);
		}
	};
}

#endif
