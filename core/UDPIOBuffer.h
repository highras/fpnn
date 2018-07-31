#ifndef FPNN_UDP_IO_Buffer_H
#define FPNN_UDP_IO_Buffer_H

#include <queue>
#include <mutex>
#include <list>
#include "Config.h"
#include "IQuestProcessor.h"

namespace fpnn
{
	struct UDPReceivedResults
	{
		ConnectionInfoPtr connInfo;
		std::list<FPQuestPtr> questList;
		std::list<FPAnswerPtr> answerList;
	};

	class UDPRecvBuffer
	{
		const int _UDPMaxDataLen;			//-- Without IPv6 jumbogram.
		uint8_t* _buffer;

		void decodeBuffer(int dataLen, struct UDPReceivedResults &result);
		
	public:
		UDPRecvBuffer(): _UDPMaxDataLen(FPNN_UDP_MAX_DATA_LENGTH)
		{
			_buffer = (uint8_t*)malloc(_UDPMaxDataLen);
		}
		~UDPRecvBuffer()
		{
			free(_buffer);
		}

		//-- Return: true: continue; false: wait for next wakeup.
		bool recvIPv4(int socket, struct UDPReceivedResults &result);
		bool recvIPv6(int socket, struct UDPReceivedResults &result);
	};

	struct UDPSendUnit
	{
		ConnectionInfoPtr connInfo;
		std::string* raw;

		UDPSendUnit(): raw(NULL) {}
		UDPSendUnit(ConnectionInfoPtr ci, std::string* raw_): connInfo(ci), raw(raw_) {}
		UDPSendUnit(const UDPSendUnit& r): connInfo(r.connInfo), raw(r.raw) {}
	};

	class UDPSendBuffer
	{
	private:
		std::mutex _mutex;
		bool _sendToken;

		std::queue<struct UDPSendUnit> _outQueue;

		void realSend(bool& needWaitSendEvent);

	public:
		UDPSendBuffer(): _sendToken(true) {}
		~UDPSendBuffer()
		{
			while (_outQueue.size())
			{
				UDPSendUnit unit = _outQueue.front();
				_outQueue.pop();
				delete unit.raw;
			}
		}

		void appendData(ConnectionInfoPtr ci, std::string* raw);
		void send(ConnectionInfoPtr ci, bool& needWaitSendEvent, bool& actualSent, std::string* data = NULL);
		void send(bool& needWaitSendEvent);
	};
}

#endif