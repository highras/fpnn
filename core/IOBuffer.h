#ifndef FPNN_IO_Buffer_H
#define FPNN_IO_Buffer_H

#include <unistd.h>
#include <atomic>
#include <string>
#include <queue>
#include <mutex>
#include <memory>
#include "OpenSSLModule.h"
#include "FPMessage.h"
#include "Receiver.h"

namespace fpnn
{
	class RecvBuffer
	{
		std::mutex* _mutex;		//-- only using for sendBuffer and sendToken
		bool _token;
		uint32_t _receivedPackage;
		Receiver* _receiver;

	public:
		RecvBuffer(int chunkSize, std::mutex* mutex): _mutex(mutex), _token(true), _receivedPackage(0)
		{
			_receiver = new StandardReceiver(chunkSize);
		}
		~RecvBuffer()
		{
			if (_receiver)
				delete _receiver;
		}

		inline void setMutex(std::mutex* mutex)
		{
			_mutex = mutex;
		}

		inline bool recvPackage(int fd, bool& needNextEvent)
		{
			return _receiver->recvPackage(fd, needNextEvent);
		}
		inline bool fetch(FPQuestPtr& quest, FPAnswerPtr& answer, bool &isHTTP)
		{
			bool rev = _receiver->fetch(quest, answer, isHTTP);
			if (rev) _receivedPackage += 1;
			return rev;
		}
		inline bool getToken()
		{
			std::unique_lock<std::mutex> lck(*_mutex);
			if (!_token)
				return false;

			_token = false;
			return true;
		}
		inline void returnToken()
		{
			std::unique_lock<std::mutex> lck(*_mutex);
			_token = true;
		}

		bool entryEncryptMode(uint8_t *key, size_t key_len, uint8_t *iv, bool streamMode);
		void entryWebSocketMode();
		inline uint8_t getWebSocketOpCode()
		{
			return ((WebSocketReceiver*)_receiver)->getControlFrameCode();
		}
		void enrtySSLMode(SSLContext *sslContext)
		{
			_receiver->enrtySSLMode(sslContext);
		}
	};

	class SendBuffer
	{
		typedef void (SendBuffer::* CurrBufferProcessFunc)();

	private:
		std::mutex* _mutex;		//-- only using for sendBuffer and sendToken
		bool _sendToken;

		size_t _offset;
		std::string* _currBuffer;
		std::queue<std::string*> _outQueue;
		uint64_t _sentBytes;		//-- Total Bytes
		uint64_t _sentPackage;
		bool _stopAppendData;
		bool _encryptAfterFirstPackage;
		Encryptor* _encryptor;
		SSLContext* _sslContext;

		CurrBufferProcessFunc _currBufferProcess;

		void encryptData();
		void addWebSocketWrap();
		int realSend(int fd, bool& needWaitSendEvent);
		int sslRealSend(int fd, bool& needWaitSendEvent);

	public:
		SendBuffer(std::mutex* mutex): _mutex(mutex), _sendToken(true), _offset(0), _currBuffer(0),
			_sentBytes(0), _sentPackage(0), _stopAppendData(false), _encryptAfterFirstPackage(false),
			_encryptor(NULL), _sslContext(NULL), _currBufferProcess(NULL) {}
		~SendBuffer()
		{
			while (_outQueue.size())
			{
				std::string* data = _outQueue.front();
				_outQueue.pop();
				delete data;
			}

			if (_currBuffer)
				delete _currBuffer;

			if (_encryptor)
				delete _encryptor;
		}
		
		inline void setMutex(std::mutex* mutex) { _mutex = mutex; }

		/** returned INT: id 0, success, else, is errno. */
		int send(int fd, bool& needWaitSendEvent, bool& actualSent, std::string* data = NULL);
		bool entryEncryptMode(uint8_t *key, size_t key_len, uint8_t *iv, bool streamMode);
		void encryptAfterFirstPackage() { _encryptAfterFirstPackage = true; }
		void appendData(std::string* data);
		void stopAppendData() { _stopAppendData = true; }
		void entryWebSocketMode(std::string* data = NULL);	//-- if data not NULL, MUST set additionalSend in TCPServerIOWorker::read().
		void enrtySSLMode(SSLContext *sslContext) { _sslContext = sslContext; }
		bool empty();
	};
}

#endif
