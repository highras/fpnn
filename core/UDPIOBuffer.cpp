#include <errno.h>
#include <arpa/inet.h>
#include "jenkins.h"
#include "FPLog.h"
#include "hex.h"
#include "NetworkUtility.h"
#include "Decoder.h"
#include "UDPIOBuffer.h"

using namespace fpnn;

void UDPRecvBuffer::decodeBuffer(int dataLen, struct UDPReceivedResults &result)
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
				result.questList.push_back(quest);
			}
			else// if (FPMessage::isAnswer(buffHeader))
			{
				desc = "UDP answer";
				FPAnswerPtr answer = Decoder::decodeAnswer(buffHeader, currPackageLen);
				result.answerList.push_back(answer);
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
bool UDPRecvBuffer::recvIPv4(int socket, struct UDPReceivedResults &result)
{
	socklen_t addrlen = (socklen_t)sizeof(struct sockaddr_in);
	struct sockaddr_in* addr = (struct sockaddr_in*)malloc(addrlen);

	socklen_t requiredAddrLen = addrlen;
	ssize_t readBytes = recvfrom(socket, _buffer, _UDPMaxDataLen, 0, (struct sockaddr *)addr, &requiredAddrLen);
	if (requiredAddrLen > addrlen)
	{
		LOG_ERROR("UDP recvfrom IPv4 address buffer truncated. required %d, offered is %d.", (int)requiredAddrLen, (int)addrlen);
		free(addr);
		return true;
	}
	if (readBytes > 0)
	{
		result.connInfo.reset(new ConnectionInfo(socket, ntohs(addr->sin_port), IPV4ToString(addr->sin_addr.s_addr), true, true));
		result.connInfo->changToUDP((uint8_t*)addr, jenkins_hash64(addr, (size_t)requiredAddrLen, 0));

		decodeBuffer((int)readBytes, result);
		return true;
	}
	else
	{
		free(addr);

		if (readBytes == 0)
			return false;

		if (errno == 0 || errno == EINTR)
			return true;

		if (errno == EAGAIN || errno == EWOULDBLOCK)
			return false;

		return false;
	}
}

//-- Return: true: continue; false: wait for next wakeup.
bool UDPRecvBuffer::recvIPv6(int socket, struct UDPReceivedResults &result)
{
	socklen_t addrlen = (socklen_t)sizeof(struct sockaddr_in6);
	struct sockaddr_in6* addr = (struct sockaddr_in6*)malloc(addrlen);

	socklen_t requiredAddrLen = addrlen;
	ssize_t readBytes = recvfrom(socket, _buffer, _UDPMaxDataLen, 0, (struct sockaddr *)addr, &requiredAddrLen);
	if (requiredAddrLen > addrlen)
	{
		LOG_ERROR("UDP recvfrom IPv6 address buffer truncated. required %d, offered is %d.", (int)requiredAddrLen, (int)addrlen);
		free(addr);
		return true;
	}
	if (readBytes > 0)
	{
		char buf[INET6_ADDRSTRLEN + 4];
		const char *rev = inet_ntop(AF_INET6, &(addr->sin6_addr), buf, sizeof(buf));
		if (rev == NULL)
		{
			char hex[32 + 1];
			hexlify(hex, &(addr->sin6_addr), 16);
			LOG_ERROR("Format IPv6 address for UDP socket %d failed. clientAddr.sin6_addr: %s", socket, hex);

			free(addr);
			return true;
		}

		result.connInfo.reset(new ConnectionInfo(socket, ntohs(addr->sin6_port), buf, false, true));
		result.connInfo->changToUDP((uint8_t*)addr, jenkins_hash64(addr, (size_t)requiredAddrLen, 0));

		decodeBuffer((int)readBytes, result);
		return true;
	}
	else
	{
		free(addr);

		if (readBytes == 0)
			return false;

		if (errno == 0 || errno == EINTR)
			return true;

		if (errno == EAGAIN || errno == EWOULDBLOCK)
			return false;

		return false;
	}
}

void UDPSendBuffer::realSend(bool& needWaitSendEvent)
{
	needWaitSendEvent = false;
	UDPSendUnit unit;
	bool retry = false;

	while (true)
	{
		if (!retry)
		{
			std::unique_lock<std::mutex> lck(_mutex);
			if (unit.raw)
			{
				_outQueue.pop();
				delete unit.raw;
			}

			if (_outQueue.size() == 0)
			{
				_sendToken = true;
				return;
			}

			unit = _outQueue.front();
		}
		else
			retry = false;

		ssize_t sendBytes = sendto(unit.connInfo->socket, unit.raw->data(), unit.raw->length(), 0,
			(struct sockaddr *)(unit.connInfo->_socketAddress),
			unit.connInfo->isIPv4() ? (socklen_t)sizeof(struct sockaddr_in): (socklen_t)sizeof(struct sockaddr_in6));
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
				std::unique_lock<std::mutex> lck(_mutex);
				_sendToken = true;
				return;
			}
			if (errno == EINTR)
			{
				retry = true;
				continue;
			}

			LOG_ERROR("Send UDP data on %s socket(%d) error: %d", unit.connInfo->isIPv4() ? "IPv4" : "IPv6",
				unit.connInfo->socket, errno);

			//std::unique_lock<std::mutex> lck(*_mutex);
			//_sendToken = true;
			//return errno;
		}
		else if ((size_t)sendBytes != unit.raw->length())
		{
			LOG_ERROR("Send UDP data on %s socket(%d) error. Want to send %d bytes, real sent %d bytes.",
				unit.connInfo->isIPv4() ? "IPv4" : "IPv6", unit.connInfo->socket, (int)(unit.raw->length()), (int)sendBytes);
		}
	}
}

void UDPSendBuffer::appendData(ConnectionInfoPtr ci, std::string* raw)
{
	if (!raw || raw->empty())
		return;

	std::unique_lock<std::mutex> lck(_mutex);
	_outQueue.push(UDPSendUnit(ci, raw));
}

void UDPSendBuffer::send(bool& needWaitSendEvent)
{
	{
		std::unique_lock<std::mutex> lck(_mutex);
		if (!_sendToken)
			return;

		_sendToken = false;
	}

	//-- Token will be return in realSend() function.
	realSend(needWaitSendEvent);
}

void UDPSendBuffer::send(ConnectionInfoPtr ci, bool& needWaitSendEvent, bool& actualSent, std::string* data)
{
	{
		std::unique_lock<std::mutex> lck(_mutex);
		if (data)
			_outQueue.push(UDPSendUnit(ci, data));

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