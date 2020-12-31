#include "hex.h"
#include "FPLog.h"
#include "Config.h"
#include "Decoder.h"
#include "Receiver.h"
#include "AutoRelease.h"
#include "FormattedPrint.h"
#include "NetworkUtility.h"

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

bool StandardReceiver::sslRecv(int fd, int requireRead, int& totalReadBytes)
{
	totalReadBytes = 0;

	while (requireRead)
	{
		int currRecvLen = requireRead;
		if (currRecvLen > _sslBufferLen)
			currRecvLen = _sslBufferLen;

		requireRead -= currRecvLen;
		int readBytes = SSL_read(_sslContext->_ssl, _sslBuffer, currRecvLen);
		if (readBytes <= 0)
		{
			int errorCode = SSL_get_error(_sslContext->_ssl, readBytes);
			if (errorCode == SSL_ERROR_WANT_WRITE)
			{
				LOG_INFO("SSL/TSL re-negotiation occurred. SSL_read WANT_WRITE. socket: %d, address: %s", fd, NetworkUtil::getPeerName(fd).c_str());
				_sslContext->_negotiate = SSLNegotiate::Read_Want_Write;
				return true;
			}
			else if (errorCode == SSL_ERROR_WANT_READ)
			{
				//LOG_WARN("SSL/TSL re-negotiation occurred. SSL_read WANT_READ. socket: %d, address: %s", fd, NetworkUtil::getPeerName(fd).c_str());
				//_sslContext->_negotiate = SSLNegotiate::Read_Want_Read;
				return true;
			}
			else if (errorCode == SSL_ERROR_ZERO_RETURN)
			{
				//LOG_INFO("socket %d ssl is closed, address: %s", fd, NetworkUtil::getPeerName(fd).c_str());
				//-- TLS/SSL connection is colsed. But the underlying transport maybe hasn't been closed.
				//-- Please Refer the SSL_get_error() doc on https://www.openssl.org.
				return true;
			}
			else if (errorCode == SSL_ERROR_SYSCALL)
			{
				if (errno == EAGAIN || errno == EWOULDBLOCK)
					return true;

				if (errno == 0 || errno == ECONNRESET)
				{
					OpenSSLModule::logLastErrors();
					return false;
				}

				LOG_ERROR("SSL read syscall error. socket %d, address: %s, errno: %d", fd, NetworkUtil::getPeerName(fd).c_str(), errno);
				OpenSSLModule::logLastErrors();
				return false;
			}
			else if (errorCode == SSL_ERROR_SSL)
			{
				LOG_ERROR("SSL read error in openSSL library. SSL error code: SSL_ERROR_SSL. socket %d, address: %s", fd, NetworkUtil::getPeerName(fd).c_str());
				OpenSSLModule::logLastErrors();
				return false;
			}
			else
			{
				LOG_ERROR("SSL read error. socket %d, address: %s, SSL error code %d, errno: %d", fd, NetworkUtil::getPeerName(fd).c_str(), errorCode, errno);
				return false;
			}
		}

		_buffer->append(_sslBuffer, readBytes);
		totalReadBytes += readBytes;
	}

	return true;
}

bool StandardReceiver::recv(int fd, int length)
{
	if (length)
	{
		_total += length;
		if (_total > Config::_max_recv_package_length)
		{
			LOG_ERROR("Recv huge(%ld) TCP data from socket: %d, address: %s. Connection will be closed by framework.", _total, fd, NetworkUtil::getPeerName(fd).c_str());
			return false;
		}
	}

	if (_sslContext)
		return sslRecvFPNNData(fd);

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

bool StandardReceiver::sslRecvFPNNData(int fd)
{
	int totalReadBytes = 0;
	bool successful = sslRecv(fd, _total - _curr, totalReadBytes);
	_curr += totalReadBytes;
	return successful;
}

bool StandardReceiver::recvTextData(int fd)
{
	if (_sslContext)
		return sslRecvTextData(fd);

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
					LOG_ERROR("Recv huge HTTP data from socket: %d, address: %s. Connection will be closed by framework.", fd, NetworkUtil::getPeerName(fd).c_str());
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
			LOG_ERROR("Recv huge HTTP data from socket: %d, address: %s. Connection will be closed by framework.", fd, NetworkUtil::getPeerName(fd).c_str());
			return false;
		}
	}
	return true;
}

bool StandardReceiver::sslRecvTextData(int fd)
{
	int totalReadBytes = 0;
	bool successful = sslRecv(fd, Config::_max_recv_package_length + 1, totalReadBytes);
	_curr += totalReadBytes;
	if (_curr > Config::_max_recv_package_length)
	{
		LOG_ERROR("Recv huge HTTPS data from socket: %d, address: %s. Connection will be closed by framework.", fd, NetworkUtil::getPeerName(fd).c_str());
		return false;
	}
	return successful;
}

bool StandardReceiver::recvTcpPackage(int fd, int length, bool& needNextEvent)
{
	if (recv(fd, length) == false)
		return false;

	needNextEvent = (_curr < _total);
	return true;
}

static void logHttpBufferError(ChainBuffer* cb, int size, int fd, const char* peerName)
{
	const int maxLoggedSize = 256;
	int loggedSize = (size <= maxLoggedSize) ? size : maxLoggedSize;

	void* buffer = malloc(loggedSize + 1);
	AutoFreeGuard bufferGuard(buffer);

	cb->writeTo(buffer, loggedSize, 0);
	((char*)buffer)[loggedSize] = 0;

	std::string visibleBinaryString = visibleBinaryBuffer(buffer, loggedSize, " ");

	char* hexBuffer = (char*)malloc(loggedSize * 2 + 1);
	AutoFreeGuard hexBufferGuard(hexBuffer);

	Hexlify(hexBuffer, buffer, loggedSize);

	LOG_WARN("Http buffer recording (visible). org size: %d, rec size: %d. socket: %d, address: %s. Buffer: %s",
		size, loggedSize, fd, peerName, visibleBinaryString.c_str());
	LOG_WARN("Http buffer recording (hex). org size: %d, rec size: %d. socket: %d, address: %s. Buffer: %s",
		size, loggedSize, fd, peerName, hexBuffer);
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
				std::string peerName = NetworkUtil::getPeerName(fd);

				if (_httpParser._headerLength == 14 && _buffer->memcmp("GET / HTTP/1.1", 14))
				{
					LOG_WARN("Receive HTTP scan, error:(%d)%s. socket: %d, address: %s", ex.code(), ex.what(), fd, peerName.c_str());
					return false;
				}

				LOG_ERROR("HttpParser::parseHeader() function error:(%d)%s. socket: %d, address: %s", ex.code(), ex.what(), fd, peerName.c_str());
				logHttpBufferError(_buffer, _httpParser._headerLength, fd, peerName.c_str());
				return false;
			}
			catch (...)
			{
				std::string peerName = NetworkUtil::getPeerName(fd);
				LOG_ERROR("Unknown ERROR, HttpParser::parseHeader() function error. socket: %d, address: %s", fd, peerName.c_str());
				logHttpBufferError(_buffer, _httpParser._headerLength, fd, peerName.c_str());
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
			LOG_ERROR("This Server DO NOT support HTTP, socket: %d, address: %s", fd, NetworkUtil::getPeerName(fd).c_str());
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
				rev = (quest != nullptr);
			}
			else
			{
				desc = "TCP answer";
				answer = Decoder::decodeAnswer(cb);
				rev = (answer != nullptr);
			}
		}
		else
		{
			isHTTP = true;
			desc = "HTTP quest";
		
			quest = Decoder::decodeHttpQuest(cb, _httpParser);
			_httpParser.reset();
			rev = (quest != nullptr);
		}
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
