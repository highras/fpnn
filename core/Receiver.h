#ifndef FPNN_Receiver_h
#define FPNN_Receiver_h

/*
	!!! IMPORTANT !!!

	*. all fetch(bool &isQuest, bool &isHTTP) function:
		MUST call it only when a package received completed.
*/

#include "ChainBuffer.h"
#include "Encryptor.h"
#include "FPMessage.h"
#include "HttpParser.h"
#include "OpenSSLModule.h"

namespace fpnn
{
	//================================//
	//--     Receiver Interface     --//
	//================================//
	class Receiver
	{
	protected:
		int _curr;
		int _total;
		SSLContext* _sslContext;

	public:
		Receiver(): _curr(0), _total(FPMessage::_HeaderLength), _sslContext(0) {}
		virtual ~Receiver() {}

		virtual bool recvPackage(int fd, bool& needNextEvent) = 0;
		virtual bool fetch(FPQuestPtr& quest, FPAnswerPtr& answer, bool &isHTTP) = 0;
		virtual void enrtySSLMode(SSLContext *sslContext)
		{
			_sslContext = sslContext;
		}
		virtual SSLContext* getSSLContext() { return _sslContext; }
	};

	//================================//
	//--      StandardReceiver      --//
	//================================//
	class StandardReceiver: public Receiver
	{
		bool _isTCP;
		bool _isHTTP;
		int _chunkSize;
		ChainBuffer* _buffer;
		HttpParser _httpParser;
		const int _sslBufferLen;
		void* _sslBuffer;

		/**
			Only called when protocol is TCP, and header has be read.
			if return -1, mean unsupported protocol.
		*/
		int remainDataLen();

		bool sslRecv(int fd, int requireRead, int& readBytes);

		/** If length > 0; will do _total += length; */
		bool recv(int fd, int length = 0);
		bool recvTextData(int fd);

		bool sslRecvFPNNData(int fd);
		bool sslRecvTextData(int fd);

		bool recvTcpPackage(int fd, int length, bool& needNextEvent);
		bool recvHttpPackage(int fd, bool& needNextEvent);

		ChainBuffer* fetchBuffer();

	public:
		StandardReceiver(int chunkSize): Receiver(), _isTCP(false), _isHTTP(false), _chunkSize(chunkSize),
			_sslBufferLen(4 * 1024), _sslBuffer(0)
		{
			_buffer = new ChainBuffer(chunkSize);
		}
		virtual ~StandardReceiver()
		{
			if (_buffer)
				delete _buffer;

			if (_sslBuffer)
				free(_sslBuffer);
		}

		virtual bool recvPackage(int fd, bool& needNextEvent);
		virtual bool fetch(FPQuestPtr& quest, FPAnswerPtr& answer, bool &isHTTP);
		virtual void enrtySSLMode(SSLContext *sslContext)
		{
			Receiver::enrtySSLMode(sslContext);
			_sslBuffer = malloc(_sslBufferLen);
		}
	};

	//================================//
	//--  EncryptedStreamReceiver   --//
	//================================//
	class EncryptedStreamReceiver: public Receiver
	{
		StreamEncryptor _encryptor;
		uint8_t* _header;
		uint8_t* _decHeader;
		uint8_t* _currBuf;
		uint8_t* _bodyBuffer;

		/**
			Only called when protocol is TCP, and header has be read.
			if return -1, mean unsupported protocol.
		*/
		int remainDataLen();

		/** If length > 0; will do _total += length; */
		bool recv(int fd, int length = 0);
		bool recvTcpPackage(int fd, int length, bool& needNextEvent);
		
	public:
		EncryptedStreamReceiver(uint8_t *key, size_t key_len, uint8_t *iv): Receiver(), _encryptor(key, key_len, iv), _bodyBuffer(NULL)
		{
			_header = (uint8_t*)malloc(FPMessage::_HeaderLength);
			_decHeader = (uint8_t*)malloc(FPMessage::_HeaderLength);
			_currBuf = _header;
		}
		virtual ~EncryptedStreamReceiver()
		{
			if (_bodyBuffer)
				free(_bodyBuffer);

			free(_header);
			free(_decHeader);
		}

		virtual bool recvPackage(int fd, bool& needNextEvent);
		virtual bool fetch(FPQuestPtr& quest, FPAnswerPtr& answer, bool &isHTTP);
	};

	//================================//
	//--  EncryptedPackageReceiver  --//
	//================================//
	class EncryptedPackageReceiver: public Receiver
	{
		PackageEncryptor _encryptor;
		uint32_t _packageLen;
		uint8_t* _dataBuffer;
		uint8_t* _currBuf;
		bool _getLength;

		/** If length > 0; will do _total += length; */
		bool recv(int fd, int length = 0);

	public:
		EncryptedPackageReceiver(uint8_t *key, size_t key_len, uint8_t *iv):
			Receiver(), _encryptor(key, key_len, iv), _packageLen(0), _dataBuffer(NULL), _getLength(false)
		{
			_total = sizeof(uint32_t);
			_currBuf = (uint8_t*)&_packageLen;
		}
		virtual ~EncryptedPackageReceiver()
		{
			if (_dataBuffer)
				free(_dataBuffer);
		}

		virtual bool recvPackage(int fd, bool& needNextEvent);
		virtual bool fetch(FPQuestPtr& quest, FPAnswerPtr& answer, bool &isHTTP);
	};

	//================================//
	//--  EncryptedPackageReceiver  --//
	//================================//
	class WebSocketReceiver: public Receiver
	{
		struct FragmentedData
		{
			uint64_t _len;
			uint8_t* _buf;		/* Don't free buf in destructor!!! */
		};

		uint16_t _header;
		uint32_t _maskingKey;
		uint64_t _payloadSize;
		//uint8_t* _payladBuffer;
		uint8_t* _currBuf;
		std::list<struct FragmentedData> _fragmentedDataList;
		int64_t _currFragmentsTotalLength;
		uint8_t _opCode;	//-- unnecessary inited when instance constructor.
		int _recvStep; 		//- 0: header, 1: payloadSize, 2: maskingKey, 3: _payloadData.
		bool _dataCompleted;

		bool recv(int fd);
		bool sslRecv(int fd);
		void freeFragmentedDataList();
		bool processHeader();
		bool processPayloadSize(int fd);
		bool processMaskingKey(int fd);
		void processPayloadData();

	public:
		WebSocketReceiver(): Receiver(), _header(0), _maskingKey(0), _payloadSize(0), //_payladBuffer(NULL),
			_currFragmentsTotalLength(0), _recvStep(0), _dataCompleted(false)
		{
			_total = sizeof(uint16_t);
			_currBuf = (uint8_t*)&_header;
		}
		virtual ~WebSocketReceiver()
		{
			freeFragmentedDataList();
		}

		virtual bool recvPackage(int fd, bool& needNextEvent);
		virtual bool fetch(FPQuestPtr& quest, FPAnswerPtr& answer, bool &isHTTP);
		inline uint8_t getControlFrameCode() { return _opCode; }
	};
}

#endif
