#ifndef FPNN_UDP_Client_IO_Buffer_H
#define FPNN_UDP_Client_IO_Buffer_H

#include <queue>
#include <mutex>
#include <list>
#include <memory>
#include "Config.h"
#include "IQuestProcessor.h"

namespace fpnn
{
	class UDPClientRecvBuffer
	{
		const int _UDPMaxDataLen;			//-- Without IPv6 jumbogram.
		uint8_t* _buffer;

		void decodeBuffer(int dataLen, std::list<FPQuestPtr> &questList, std::list<FPAnswerPtr> &answerList);
		
	public:
		UDPClientRecvBuffer(): _UDPMaxDataLen(FPNN_UDP_MAX_DATA_LENGTH)
		{
			_buffer = (uint8_t*)malloc(_UDPMaxDataLen);
		}
		~UDPClientRecvBuffer()
		{
			free(_buffer);
		}

		//-- Return: true: continue; false: wait for next wakeup.
		bool recv(ConnectionInfoPtr ci, std::list<FPQuestPtr> &questList, std::list<FPAnswerPtr> &answerList);
	};

	class UDPClientSendBuffer
	{
	private:
		std::mutex *_mutex;
		bool _sendToken;

		ConnectionInfoPtr _connectionInfo;
		std::queue<std::string*> _outQueue;

		void realSend(bool& needWaitSendEvent);

	public:
		UDPClientSendBuffer(std::mutex *mutex, ConnectionInfoPtr ci): _mutex(mutex), _sendToken(true), _connectionInfo(ci) {}
		~UDPClientSendBuffer()
		{
			while (_outQueue.size())
			{
				std::string* raw = _outQueue.front();
				_outQueue.pop();
				delete raw;
			}
		}

		void send(bool& needWaitSendEvent, bool& actualSent, std::string* data = NULL);
		void send(bool& needWaitSendEvent);
	};
}

#endif