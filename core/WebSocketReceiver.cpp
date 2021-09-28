#ifdef __APPLE__
	#include "Endian.h"
#else
	#include <endian.h>
#endif
#include "FPLog.h"
#include "Config.h"
#include "Decoder.h"
#include "Receiver.h"
#include "NetworkUtility.h"

using namespace fpnn;

void WebSocketReceiver::freeFragmentedDataList()
{
	for (auto& data: _fragmentedDataList)
		free(data._buf);

	_fragmentedDataList.clear();
	_currFragmentsTotalLength = 0;
}

bool WebSocketReceiver::recv(int fd)
{
	if (_sslContext)
		return sslRecv(fd);

	while (_curr < _total)
	{
		int requireRead = _total - _curr;
		int readBytes = (int)::read(fd, _currBuf + _curr, requireRead);
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

bool WebSocketReceiver::sslRecv(int fd)
{
	while (_curr < _total)
	{
		int requireRead = _total - _curr;
		int readBytes = SSL_read(_sslContext->_ssl, _currBuf + _curr, requireRead);
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
		
		_curr += readBytes;
	}
	return true;
}

bool WebSocketReceiver::recvPackage(int fd, bool& needNextEvent)
{
	while (true)
	{
		if (_curr < _total)
		{
			if (recv(fd) == false)
				return false;

			if (_curr < _total)
			{
				needNextEvent = true;
				return true;
			}
		}

		if (_recvStep == 0)
		{
			if (!processHeader())
				return false;
		}
		else if (_recvStep == 1)
		{
			if (!processPayloadSize(fd))
				return false;
		}
		else if (_recvStep == 2)
		{
			if (!processMaskingKey(fd))
				return false;

			//-- for control frame with zero payload.
			if (_recvStep == 0 && _dataCompleted)
			{
				needNextEvent = false;
				return true;
			}
		}
		else // if (_recvStep == 3)
		{
			processPayloadData();
			if (_dataCompleted)
			{
				needNextEvent = false;
				return true;
			}
		}
	}
}

bool WebSocketReceiver::processHeader()
{
	if ((_currBuf[1] & 0x80) == 0x0)
		return false;		//-- Data from client, MUST using masking-key. Ref: https://tools.ietf.org/html/rfc6455

	/*
		Temporarily cover the _dataCompleted field if the control frame appeared between a continued fragemented data.
		The fetch() func will not clear the _fragmentedDataList if the current frame is control frame.
	*/
	_dataCompleted = (_header & 0x8000);
	_opCode = _currBuf[0] & 0xf;

	uint8_t payloadSize = _currBuf[1] & 0x7f;
	if (payloadSize < 126)
	{
		_payloadSize = payloadSize;
		_recvStep = 2;

		_currBuf = (uint8_t*)&_maskingKey;
		_total = 4;
		_curr = 0;
	}
	else
	{
		_curr = 0;
		_currBuf = (uint8_t*)&_payloadSize;

		if (payloadSize == 126)
			_total = 2;
		else
			_total = 8;

		_recvStep = 1;
	}

	return true;
}

bool WebSocketReceiver::processPayloadSize(int fd)
{
	if (_total == 2)
	{
		uint16_t beSize;
		memcpy(&beSize, &_payloadSize, 2);
		// uint16_t size = ntohs(beSize);
		uint16_t size = be16toh(beSize);
		_payloadSize = size;
	}
	else if (_total == 8)
	{
		uint64_t size = be64toh(_payloadSize);
		_payloadSize = size;
	}
	else
		return false;

	if (_payloadSize + _currFragmentsTotalLength > (uint64_t)Config::_max_recv_package_length)
	{
		LOG_ERROR("WebSocket client want send huge TCP data (size: %llu) to server, from socket: %d, address: %s. Connection will be closed by framework.", _payloadSize, fd, NetworkUtil::getPeerName(fd).c_str());
		return false;
	}

	_recvStep = 2;
	_currBuf = (uint8_t*)&_maskingKey;
	_total = 4;
	_curr = 0;

	return true;
}

bool WebSocketReceiver::processMaskingKey(int fd)
{
	if (_payloadSize + _currFragmentsTotalLength > (uint64_t)Config::_max_recv_package_length)
	{
		LOG_ERROR("WebSocket client want send huge TCP data (size: %llu) to server, from socket: %d, address: %s. Connection will be closed by framework.", _payloadSize, fd, NetworkUtil::getPeerName(fd).c_str());
		return false;
	}

	if (_payloadSize)
	{
		FragmentedData frag;
		frag._len = _payloadSize;
		frag._buf = (uint8_t*)malloc(_payloadSize);

		_fragmentedDataList.push_back(frag);
		_currFragmentsTotalLength += frag._len;

		_recvStep = 3;
		_currBuf = frag._buf;
		_total = (int)_payloadSize;
		_curr = 0;
	}
	else	//-- for control frame with zero payload.
	{
		_curr = 0;
		_total = sizeof(uint16_t);
		_currBuf = (uint8_t*)&_header;
		_recvStep = 0;
	}

	return true;
}

void WebSocketReceiver::processPayloadData()
{
	/*
		Make masking-key process.
	*/
	uint8_t* mask = (uint8_t*)&_maskingKey;

	for (uint64_t i = 0; i < _payloadSize; i++)
		_currBuf[i] ^= mask[i%4];


	_curr = 0;
	_total = sizeof(uint16_t);
	_currBuf = (uint8_t*)&_header;
	_recvStep = 0;
}

bool WebSocketReceiver::fetch(FPQuestPtr& quest, FPAnswerPtr& answer, bool &isHTTP)
{
	/*
		1. DO NOT clear the _fragmentedDataList if the current frame is control frame.
		2. Not reset _opCode.
	*/

	isHTTP = true;

	//-- control frames & data frames which reserved for future.
	if (_opCode > 2)
	{
		if (_payloadSize > 0)
		{
			struct FragmentedData &backNode = _fragmentedDataList.back();
			_currFragmentsTotalLength -= backNode._len;
			free(backNode._buf);
			_fragmentedDataList.pop_back();
		}
		return true;
	}

	//-------------- Data frames -------------//
	struct FragmentedData fullyData;
	size_t fragmentCount = _fragmentedDataList.size();

	if (fragmentCount == 1)
	{
		fullyData = _fragmentedDataList.back();
		_fragmentedDataList.clear();
		_currFragmentsTotalLength = 0;
	}
	else if (fragmentCount > 1)
	{
		// fullyData._len = 0;
		// for (auto& frag: _fragmentedDataList)
		// 	fullyData._len += frag._len;

		fullyData._len = _currFragmentsTotalLength;

		uint64_t pos = 0;
		fullyData._buf = (uint8_t*)malloc(fullyData._len);
		for (auto it = _fragmentedDataList.begin(); it != _fragmentedDataList.end(); it++)
		{
			memcpy(fullyData._buf + pos, it->_buf, it->_len);
			pos += it->_len;
		}

		freeFragmentedDataList();
	}
	else
		return false;


	//-------------- begin decode -------------//
	bool rev = false;
	const char *desc = "unknown";
	try
	{
		if (FPMessage::isQuest((char*)fullyData._buf))
		{
			desc = "webSocket quest";
			quest = Decoder::decodeQuest((char*)fullyData._buf, fullyData._len);
			rev = (quest != nullptr);
		}
		else
		{
			desc = "webSocket answer";
			answer = Decoder::decodeAnswer((char*)fullyData._buf, fullyData._len);
			rev = (answer != nullptr);
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

	free(fullyData._buf);
	return rev;
}
