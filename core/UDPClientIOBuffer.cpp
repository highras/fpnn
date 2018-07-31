#include <errno.h>
#include <arpa/inet.h>
#include "FPLog.h"
#include "Decoder.h"
#include "UDPClientIOBuffer.h"

using namespace fpnn;

void UDPClientRecvBuffer::decodeBuffer(int dataLen, std::list<FPQuestPtr> &questList, std::list<FPAnswerPtr> &answerList)
{
	int offset = 0;

	while (true)
	{
		int remain = dataLen - offset;
		if (remain == 0)
			return;
		else if (remain < FPMessage::_HeaderLength)
		{
			LOG_ERROR("Invalid data, which maybe truncated. Remained data length is %d.", remain);
			return;
		}

		char* buffHeader = (char*)(_buffer + offset);

		//-- FPMessage::isTCP() will changed as FPMessage::isFPNN().
		if (!FPMessage::isTCP((char *)buffHeader))
		{
			LOG_ERROR("Invalid data, which is not encoded as FPNN protocol package. Invalid length is %d.", remain);
			return;
		}

		int currPackageLen = (int)(sizeof(FPMessage::Header) + FPMessage::BodyLen((char *)buffHeader));
		if (remain < currPackageLen)
		{
			LOG_ERROR("Invalid data. Required package length is %d, but remain data length is %d.", currPackageLen, remain);
			return;
		}

		const char *desc = "unknown";
		try
		{
			if (FPMessage::isQuest(buffHeader))
			{
				desc = "UDP quest";
				FPQuestPtr quest = Decoder::decodeQuest(buffHeader, currPackageLen);
				questList.push_back(quest);
			}
			else// if (FPMessage::isAnswer(buffHeader))
			{
				desc = "UDP answer";
				FPAnswerPtr answer = Decoder::decodeAnswer(buffHeader, currPackageLen);
				answerList.push_back(answer);
			}
			/*else
			{
				LOG_ERROR("Invalid data. FPNN MType is error. Drop remain data length %d.", remain);
				return;
			}*/
		}
		catch (const FpnnError& ex)
		{
			LOG_ERROR("Decode %s error. Drop remain data length %d. Code: %d, error: %s.", desc, remain, ex.code(), ex.what());
			return;
		}
		catch (...)
		{
			LOG_ERROR("Decode %s error. Drop remain data length %d.", desc, remain);
			return;
		}

		offset += currPackageLen;
	}
}

//-- Return: true: continue; false: wait for next wakeup.
bool UDPClientRecvBuffer::recv(ConnectionInfoPtr ci, std::list<FPQuestPtr> &questList, std::list<FPAnswerPtr> &answerList)
{
	ssize_t readBytes = ::recv(ci->socket, _buffer, _UDPMaxDataLen, 0);
	//ssize_t readBytes = recvfrom(ci->socket, _buffer, _UDPMaxDataLen, 0, NULL, NULL);
	if (readBytes > 0)
	{
		decodeBuffer((int)readBytes, questList, answerList);
		return true;
	}
	else
	{
		if (readBytes == 0)
			return false;

		if (errno == 0 || errno == EINTR)
			return true;

		if (errno == EAGAIN || errno == EWOULDBLOCK)
			return false;

		return false;
	}
}

void UDPClientSendBuffer::realSend(bool& needWaitSendEvent)
{
	needWaitSendEvent = false;
	std::string* raw = NULL;
	bool retry = false;

	while (true)
	{
		if (!retry)
		{
			std::unique_lock<std::mutex> lck(*_mutex);
			if (raw)
			{
				_outQueue.pop();
				delete raw;
			}

			if (_outQueue.size() == 0)
			{
				_sendToken = true;
				return;
			}

			raw = _outQueue.front();
		}
		else
			retry = false;

		ssize_t sendBytes = ::send(_connectionInfo->socket, raw->data(), raw->length(), 0);
		//ssize_t sendBytes = sendto(_connectionInfo->socket, raw->data(), raw->length(), 0, NULL, 0);
		if (sendBytes == -1)
		{
			/*
			ENOBUFS: 
				The  output queue for a network interface was full.  This generally indicates that the interface has stopped sending,
              but may be caused by transient congestion.  (Normally, this does not occur  in  Linux.   Packets  are  just  silently
              dropped when a device queue overflows.)
			*/

			if (errno == EAGAIN || errno == EWOULDBLOCK || errno == ENOBUFS)
			{
				needWaitSendEvent = true;
				std::unique_lock<std::mutex> lck(*_mutex);
				_sendToken = true;
				return;
			}
			if (errno == EINTR)
			{
				retry = true;
				continue;
			}

			LOG_ERROR("Send UDP data on %s socket(%d) error: %d", _connectionInfo->isIPv4() ? "IPv4" : "IPv6",
				_connectionInfo->socket, errno);

			//std::unique_lock<std::mutex> lck(*_mutex);
			//_sendToken = true;
			//return errno;
		}
		else if ((size_t)sendBytes != raw->length())
		{
			LOG_ERROR("Send UDP data on %s socket(%d) error. Want to send %d bytes, real sent %d bytes.",
				_connectionInfo->isIPv4() ? "IPv4" : "IPv6", _connectionInfo->socket, (int)(raw->length()), (int)sendBytes);
		}
	}
}

void UDPClientSendBuffer::send(bool& needWaitSendEvent)
{
	{
		std::unique_lock<std::mutex> lck(*_mutex);
		if (!_sendToken)
			return;

		_sendToken = false;
	}

	//-- Token will be return in realSend() function.
	realSend(needWaitSendEvent);
}

void UDPClientSendBuffer::send(bool& needWaitSendEvent, bool& actualSent, std::string* data)
{
	{
		std::unique_lock<std::mutex> lck(*_mutex);
		if (data)
			_outQueue.push(data);

		if (!_sendToken)
		{
			actualSent = false;
			return;
		}

		_sendToken = false;
	}

	actualSent = true;

	//-- Token will be return in realSend() function.
	realSend(needWaitSendEvent);
}