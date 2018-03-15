#include "FPLog.h"
#include "Config.h"
#include "Decoder.h"
#include "Receiver.h"

using namespace fpnn;

/**
*	If return -1, mean is HTTP package, or error package. Pls check _isHTTP flag.
*/
int StandardReceiver::remainDataLen()
{
	/**
		IChainBuffer->header(int require_length) maybe return NULL.
		But TWO things ensure in this case, the function alway returns available pointer, NEVER returns NULL.
		1. StandardReceiver chunk size is larger than FPMessage::HeaderLength.
			(Set By TCPServerConnection in IOWorker.h or TCPClientConnection in ClientIOWorker.h.)
		2. By framework logic, all caller will call this function after received at least FPMessage::HeaderLength bytes data.
	*/
	const char* header = (const char *)_buffer->header(FPMessage::_HeaderLength);

	//-- check Magic Header
	if (FPMessage::isTCP(header))
	{
		_isTCP = true;
		_isHTTP = false;

		return (int)(sizeof(FPMessage::Header) + FPMessage::BodyLen(header)) - _curr;
	}
	else if (FPMessage::isHTTP(header))
	{
		_isTCP = false;
		_isHTTP = true;
		_httpParser.reset();
		return -1;
	}
	else
	{
		_isTCP = false;
		_isHTTP = false;
		return -1;
	}
}

bool StandardReceiver::recv(int fd, int length)
{
	if (length)
	{
		_total += length;
		if (_total > Config::_max_recv_package_length)
		{
			LOG_ERROR("Recv huge TCP data from socket: %d. Connection will be closed by framework.", fd);
			return false;
		}	
	}

	while (_curr < _total)
	{
		int requireRead = _total - _curr;
		int readBytes = _buffer->readfd(fd, requireRead);
		if (readBytes != requireRead)
		{
			if (errno == EAGAIN || errno == EWOULDBLOCK)
			{
				if (readBytes > 0)
					_curr += readBytes;

				return true;
			}

			if (errno == 0 || errno == EINTR)
			{
				if (readBytes > 0)
				{
					_curr += readBytes;
					continue;
				}	
				else
					return false;
			}
			else
				return false;
		}
		
		_curr += readBytes;
		return true;
	}
	return true;
}

bool StandardReceiver::recvTextData(int fd)
{
	const int defaultRequireRead = 1024;
	while (true)
	{
		int readBytes = _buffer->readfd(fd, defaultRequireRead);
		if (readBytes != defaultRequireRead)
		{
			if (readBytes > 0)
			{
				_curr += readBytes;

				if (_curr > Config::_max_recv_package_length)
				{
					LOG_ERROR("Recv huge HTTP data from socket: %d. Connection will be closed by framework.", fd);
					return false;
				}
				return true;
			}

			if (errno == EAGAIN || errno == EWOULDBLOCK)
				return true;

			if (errno == 0 || errno == EINTR)
				return false;
			else
				return false;
		}

		_curr += readBytes;
		if (_curr > Config::_max_recv_package_length)
		{
			LOG_ERROR("Recv huge HTTP data from socket: %d. Connection will be closed by framework.", fd);
			return false;
		}
	}
	return true;
}

bool StandardReceiver::recvTcpPackage(int fd, int length, bool& needNextEvent)
{
	if (recv(fd, length) == false)
		return false;

	needNextEvent = (_curr < _total);
	return true;
}

bool StandardReceiver::recvHttpPackage(int fd, bool& needNextEvent)
{
	_total = FPMessage::_HeaderLength + 1;	//-- disable _total;

	if (recvTextData(fd) == false)
		return false;

	if (!_httpParser._headReceived)
	{
		_httpParser.checkHttpHeader(_buffer);

		if (!_httpParser._headReceived)
		{
			needNextEvent = true;
			return true;
		}
		else
		{
			try
			{
				_httpParser.parseHeader(_buffer, _httpParser._headerLength);
			}
			catch (const FpnnError& ex)
			{
				LOG_ERROR("HttpParser::parseHeader() function error:(%d)%s. socket: %d", ex.code(), ex.what(), fd);
				return false;
			}
			catch (...)
			{
				LOG_ERROR("Unknown ERROR, HttpParser::parseHeader() function error. socket: %d", fd);
				return false;
			}
		}
	}

	if (_httpParser._chunked)
	{
		if (!_httpParser.isChunckedContentCompleted(_buffer))
		{
			needNextEvent = true;
			return true;
		}
	}
	else
	{
		if (_curr < _httpParser._contentOffset + _httpParser._contentLen)
		{
			needNextEvent = true;
			return true;
		}
	}
	
	needNextEvent = false;
	return true;
}

bool StandardReceiver::recvPackage(int fd, bool& needNextEvent)
{
	if (_curr < FPMessage::_HeaderLength)
	{
		if (recv(fd) == false)
			return false;

		if (_curr < FPMessage::_HeaderLength)
		{
			needNextEvent = true;
			return true;
		}
	}

	int length = 0;
	if (_curr == _total && _total == FPMessage::_HeaderLength)
	{
		length = remainDataLen();
	}

	if (_isTCP)
		return recvTcpPackage(fd, length, needNextEvent);

	if (_isHTTP){
		if(Config::_server_http_supported){
			return recvHttpPackage(fd, needNextEvent);
		}
		else{
			LOG_ERROR("This Server DO NOT support HTTP, fd:%d", fd);
		}
	}

	return false;
}

ChainBuffer* StandardReceiver::fetchBuffer()
{
	if (_isTCP && (_curr != _total))
		return NULL;

	ChainBuffer* cb = _buffer;
	_buffer = new ChainBuffer(_chunkSize);
	_curr = 0;
	_total = FPMessage::_HeaderLength;
	return cb;
}

bool StandardReceiver::fetch(FPQuestPtr& quest, FPAnswerPtr& answer, bool &isHTTP)
{
	ChainBuffer * cb = fetchBuffer();
	if (!cb)
		return false;

	bool rev = false;
	const char *desc = "unknown";
	try
	{
		if (_isTCP)
		{
			isHTTP = false;
			if (Decoder::isQuest(cb))
			{
				desc = "TCP quest";
				quest = Decoder::decodeQuest(cb);
			}
			else
			{
				desc = "TCP answer";
				answer = Decoder::decodeAnswer(cb);
			}
		}
		else
		{
			isHTTP = true;
			desc = "HTTP quest";
		
			quest = Decoder::decodeHttpQuest(cb, _httpParser);
			_httpParser.reset();
		}
		rev = true;
	}
	catch (const FpnnError& ex)
	{
		LOG_ERROR("Decode %s error. Connection will be closed by server. Code: %d, error: %s.", desc, ex.code(), ex.what());
	}
	catch (...)
	{
		LOG_ERROR("Decode %s error. Connection will be closed by server.", desc);
	}

	delete cb;
	return rev;
}
