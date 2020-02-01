#include <endian.h>
#include "FPLog.h"
#include "Config.h"
#include "Decoder.h"
#include "Receiver.h"
#include "NetworkUtility.h"

using namespace fpnn;

bool EncryptedPackageReceiver::recv(int fd, int length)
{
	if (length)
	{
		_total += length;
		/*if (_total > Config::_max_recv_package_length)
		{
			LOG_ERROR("Recv huge TCP data from socket: %d. Connection will be closed by framework.", fd);
			return false;
		}*/
	}

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

bool EncryptedPackageReceiver::recvPackage(int fd, bool& needNextEvent)
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

	if (_getLength == false)
	{
		uint32_t len = le32toh(_packageLen);
		if (len > (uint32_t)Config::_max_recv_package_length)
		{
			LOG_ERROR("Recv huge TCP data from socket: %d, address: %s. Package length: %u. Connection will be closed by framework.", fd, NetworkUtil::getPeerName(fd).c_str(), len);
			return false;
		}

		_packageLen = len;
		_curr = 0;
		_total = len;
		_getLength = true;

		if (_dataBuffer)
			free(_dataBuffer);

		_dataBuffer = (uint8_t*)malloc(_total);
		_currBuf = _dataBuffer;

		return recvPackage(fd, needNextEvent);
	}
	else
	{
		needNextEvent = false;
		return true;
	}
}

bool EncryptedPackageReceiver::fetch(FPQuestPtr& quest, FPAnswerPtr& answer, bool &isHTTP)
{
	if (_curr != _total)
		return false;

	int dataLen = _total;
	char* buf = (char*)malloc(dataLen);
	_encryptor.decrypt((uint8_t *)buf, _dataBuffer, dataLen);

	free(_dataBuffer);
	_dataBuffer = NULL;

	_curr = 0;
	_total = sizeof(uint32_t);
	_getLength = false;
	_currBuf = (uint8_t*)&_packageLen;

	//------- begin decode -------//
	isHTTP = false;
	bool rev = false;
	const char *desc = "unknown";
	try
	{
		if (FPMessage::isQuest(buf))
		{
			desc = "TCP quest";
			quest = Decoder::decodeQuest(buf, dataLen);
			rev = (quest != nullptr);
		}
		else
		{
			desc = "TCP answer";
			answer = Decoder::decodeAnswer(buf, dataLen);
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

	free(buf);
	return rev;
}
