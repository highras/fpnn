#include "FPLog.h"
#include "NetworkUtility.h"
#include "RawTransmissionCommon.h"

using namespace fpnn;

void IRawDataProcessor::process(ConnectionInfoPtr, uint8_t* buffer, uint32_t len)
{
	LOG_ERROR("!!! No process(uint8_t* buffer, uint32_t len) for class IRawDataProcessor Implemented.");
}

void IRawDataBatchProcessor::process(ConnectionInfoPtr connectionInfo, const std::list<ReceivedRawData*>& dataList)
{
	for (auto data: dataList)
		IRawDataProcessor::process(connectionInfo, data->data, data->len);
}

void IRawDataChargeProcessor::process(ConnectionInfoPtr connectionInfo, std::list<ReceivedRawData*>& dataList)
{
	for (auto data: dataList)
	{
		IRawDataProcessor::process(connectionInfo, data->data, data->len);
		delete data;
	}
}


ReceivedRawData* RawDataReceiver::fetch()
{
	if (_reusingBuffer && _reusingBuffer->len > 0)
	{
		ReceivedRawData* data = _reusingBuffer;
		_reusingBuffer = NULL;
		return data;
	}

	return NULL;
}

bool TCPRawDataReceiver::recv(int socket)
{
	if (_reusingBuffer == NULL)
		_reusingBuffer = new ReceivedRawData(_chunkSize);

	while (_reusingBuffer->len < _chunkSize)
	{
		uint32_t requiredLength = _chunkSize - _reusingBuffer->len;
		ssize_t readBytes = ::recv(socket, _reusingBuffer->data + _reusingBuffer->len, requiredLength, 0);

		if (readBytes > 0)
			_reusingBuffer->len += (uint32_t)readBytes;

		if ((uint32_t)readBytes != requiredLength)
		{
			if (errno == EAGAIN || errno == EWOULDBLOCK)
				return true;
			
			if (errno == EINTR)
				continue;

			if (readBytes == 0)
			{
				_requireClose = true;
				return (_reusingBuffer->len > 0);
			}

			if (errno == 0)
				return true;

			return false;
		}
		else
		{
			return true;
		}
	}

	return true;	//-- The logic cannot triggered here, but compiler need a return here for logic checking.
}

bool UDPRawDataReceiver::recv(int socket)
{
	if (_reusingBuffer == NULL)
		_reusingBuffer = new ReceivedRawData(_chunkSize);

	while (true)
	{
		ssize_t readBytes = ::recv(socket, _reusingBuffer->data, _chunkSize, 0);
		if (readBytes > 0)
		{
			_reusingBuffer->len = (uint32_t)readBytes;
			return true;
		}
		else
		{
			if (errno == 0 || readBytes == 0)
				return true;

			if (errno == EAGAIN || errno == EWOULDBLOCK)
				return true;

			if (errno == EINTR)
				continue;
			
			return false;
		}
	}
}


int RawDataSender::send(int fd, bool& needWaitSendEvent, bool& actualSent, std::string* data)
{
	if (data && data->empty())
	{
		delete data;
		data = NULL;
	}

	{
		std::unique_lock<std::mutex> lck(*_mutex);
		if (data)
			_outQueue.push(data);

		if (!_token)
		{
			actualSent = false;
			return 0;
		}

		_token = false;
	}

	actualSent = true;

	//-- Token will be return in realSend()/sslRealSend() function.
	//-- ignore all error status. it will be deal in IO thread.
	return realSend(fd, needWaitSendEvent);
}

int TCPRawDataSender::realSend(int fd, bool& needWaitSendEvent)
{
	uint64_t currSendBytes = 0;

	needWaitSendEvent = false;
	while (true)
	{
		if (_currBuffer == NULL)
		{
			{
				std::unique_lock<std::mutex> lck(*_mutex);
				if (_outQueue.size() == 0)
				{
					_sentBytes += currSendBytes;
					_token = true;
					return 0;
				}

				_currBuffer = _outQueue.front();
				_outQueue.pop();
				_offset = 0;
			}
		}

		size_t requireSend = _currBuffer->length() - _offset;
		ssize_t sendBytes = write(fd, _currBuffer->data() + _offset, requireSend);
		if (sendBytes == -1)
		{
			if (errno == EAGAIN || errno == EWOULDBLOCK)
			{
				needWaitSendEvent = true;
				std::unique_lock<std::mutex> lck(*_mutex);
				_sentBytes += currSendBytes;
				_token = true;
				return 0;
			}
			if (errno == EINTR)
				continue;

			std::unique_lock<std::mutex> lck(*_mutex);
			_sentBytes += currSendBytes;
			_token = true;
			return errno;
		}
		else
		{
			_offset += (size_t)sendBytes;
			currSendBytes += (uint64_t)sendBytes;
			if (_offset == _currBuffer->length())
			{
				delete _currBuffer;
				_currBuffer = NULL;
				_offset = 0;
			}
		}
	}
}

int UDPRawDataSender::realSend(int fd, bool& needWaitSendEvent)
{
	uint64_t currSendBytes = 0;

	int retryCount = 0;
	needWaitSendEvent = false;
	while (true)
	{
		if (_currBuffer == NULL)
		{
			retryCount = 0;
			{
				std::unique_lock<std::mutex> lck(*_mutex);
				if (_outQueue.size() == 0)
				{
					_sentBytes += currSendBytes;
					_token = true;
					return 0;
				}

				_currBuffer = _outQueue.front();
				_outQueue.pop();
			}
		}

		ssize_t sendBytes = ::send(fd, _currBuffer->data(), _currBuffer->length(), 0);
		if ((size_t)sendBytes == _currBuffer->length())
		{
			currSendBytes += (uint64_t)sendBytes;
			delete _currBuffer;
			_currBuffer = NULL;
		}
		else if (sendBytes == -1)
		{
			if (errno == EAGAIN || errno == EWOULDBLOCK || errno == ENOBUFS)
			{
				needWaitSendEvent = true;
				std::unique_lock<std::mutex> lck(*_mutex);
				_token = true;
				return 0;
			}
			if (errno == EINTR)
				continue;

			std::unique_lock<std::mutex> lck(*_mutex);
			_token = true;
			return errno;
		}
		else
		{
			LOG_ERROR("Send Raw UDP data on socket(%d) endpoint: %s error. Want to send %d bytes, real sent %d bytes. Retry count: %d",
				fd, NetworkUtil::getPeerName(fd).c_str(), (int)(_currBuffer->length()), (int)sendBytes, retryCount);

			const int maxRetry = 3;
			retryCount += 1;

			if (retryCount >= maxRetry)
			{
				delete _currBuffer;
				_currBuffer = NULL;
			}
		}
	}
}