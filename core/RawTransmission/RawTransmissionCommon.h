#ifndef FPNN_Raw_Transmission_Common_H
#define FPNN_Raw_Transmission_Common_H

#include <stdint.h>
#include <stdlib.h>
#include <memory>
#include <mutex>
#include <queue>
#include <list>
#include "../IQuestProcessor.h"

namespace fpnn
{
	struct ReceivedRawData
	{
		uint8_t* data;
		uint32_t len;

		ReceivedRawData(uint32_t maxBufferLength): len(0)
		{
			const uint32_t maxLen = 65535;

			if (maxBufferLength < maxLen)
				data = (uint8_t*)malloc(maxBufferLength);
			else
				data = (uint8_t*)malloc(maxLen);
		}

		~ReceivedRawData()
		{
			free(data);
		}
	};

	class IRawDataProcessor
	{
		friend class RawClient;

	protected:
		enum RawDataProcessorType
		{
			StandardType,
			BatchType,
			ChargeType
		};

		virtual enum RawDataProcessorType processType() { return StandardType; }

	public:
		virtual void process(ConnectionInfoPtr connectionInfo, uint8_t* buffer, uint32_t len);
		virtual ~IRawDataProcessor() {}
	};
	typedef std::shared_ptr<IRawDataProcessor> IRawDataProcessorPtr;

	class IRawDataBatchProcessor: public IRawDataProcessor
	{
	protected:
		virtual enum RawDataProcessorType processType() { return BatchType; }
	public:
		using IRawDataProcessor::process;
		virtual void process(ConnectionInfoPtr connectionInfo, const std::list<ReceivedRawData*>& dataList);	//-- DO NOT delete all ReceivedRawData* pointers.
		virtual ~IRawDataBatchProcessor() {}
	};

	class IRawDataChargeProcessor: public IRawDataProcessor
	{
	protected:
		virtual enum RawDataProcessorType processType() { return ChargeType; }
	public:
		using IRawDataProcessor::process;
		virtual void process(ConnectionInfoPtr connectionInfo, std::list<ReceivedRawData*>& dataList);	//-- MUST delete all ReceivedRawData* pointers.
		virtual ~IRawDataChargeProcessor() {}
	};

	class RawDataReceiver
	{
		std::mutex* _mutex;
		bool _token;

	protected:
		uint32_t _chunkSize;
		ReceivedRawData* _reusingBuffer;

	public:
		RawDataReceiver(uint32_t chunkSize): _mutex(NULL),
			_token(true), _chunkSize(chunkSize), _reusingBuffer(NULL) {}
			
		virtual ~RawDataReceiver()
		{
			if (_reusingBuffer)
				delete _reusingBuffer;
		}

		inline void setMutex(std::mutex* mutex)
		{
			_mutex = mutex;
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

		ReceivedRawData* fetch();

		virtual bool recv(int socket) = 0;
		virtual bool requireClose() { return false; }	//-- for RawClientConnection::recv().
	};

	class TCPRawDataReceiver: public RawDataReceiver
	{
		bool _requireClose;

	public:
		TCPRawDataReceiver(uint32_t chunkSize): RawDataReceiver(chunkSize), _requireClose(false) {}
		virtual ~TCPRawDataReceiver() {}

		virtual bool recv(int socket);
		virtual bool requireClose() { return _requireClose; }
	};

	class UDPRawDataReceiver: public RawDataReceiver
	{
	public:
		UDPRawDataReceiver(uint32_t chunkSize): RawDataReceiver(chunkSize) {}
		virtual ~UDPRawDataReceiver() {}

		virtual bool recv(int socket);
	};



	class RawDataSender
	{
	protected:
		std::mutex* _mutex;
		bool _token;

		size_t _offset;
		std::string* _currBuffer;
		std::queue<std::string*> _outQueue;
		uint64_t _sentBytes;		//-- Total Bytes

	protected:
		virtual int realSend(int fd, bool& needWaitSendEvent) = 0;

	public:
		RawDataSender(): _mutex(NULL), _token(true), _offset(0), _currBuffer(0),
			_sentBytes(0) {}

		virtual ~RawDataSender()
		{
			while (_outQueue.size())
			{
				std::string* data = _outQueue.front();
				_outQueue.pop();
				delete data;
			}

			if (_currBuffer)
				delete _currBuffer;
		}

		inline void setMutex(std::mutex* mutex)
		{
			_mutex = mutex;
		}

		/** returned INT: id 0, success, else, is errno. */
		int send(int fd, bool& needWaitSendEvent, bool& actualSent, std::string* data = NULL);
	};

	class TCPRawDataSender: public RawDataSender
	{
	protected:
		virtual int realSend(int fd, bool& needWaitSendEvent);
	public:
		virtual ~TCPRawDataSender() {}
	};

	class UDPRawDataSender: public RawDataSender
	{
	protected:
		virtual int realSend(int fd, bool& needWaitSendEvent);
	public:
		virtual ~UDPRawDataSender() {}
	};
}

#endif