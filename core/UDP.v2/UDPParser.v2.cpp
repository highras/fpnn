//#include <limits>
#include "FPLog.h"
#include "msec.h"
#include "../Decoder.h"
#include "UDPParser.v2.h"

using namespace fpnn;

void SessionInvalidChecker::updateValidStatus()
{
	lastValidMsec = slack_real_msec();
	invalidCount = 0;
}

bool SessionInvalidChecker::isInvalid()
{
	if (invalidCount >= Config::UDP::_max_tolerated_count_before_valid_package_received)
		return true;

	if (invalidCount > 0 && slack_real_msec() - lastValidMsec >= threshold)
		return true;

	return false;
}

void ARQParser::configPackageEncryptor(uint8_t *key, size_t key_len, uint8_t *iv)
{
	if (!_decryptedBuffer)
		_decryptedBuffer = (uint8_t*)malloc(_decryptedBufferLen);

	if (!_encryptor)
		_encryptor = new UDPEncryptor();

	_encryptor->configPackageEncryptor(key, key_len, iv);
}
void ARQParser::configDataEncryptor(uint8_t *key, size_t key_len, uint8_t *iv)
{
	if (!_decryptedBuffer)
		_decryptedBuffer = (uint8_t*)malloc(_decryptedBufferLen);

	if (!_encryptor)
		_encryptor = new UDPEncryptor();

	_encryptor->configDataEncryptor(key, key_len, iv);

	_enableDataEnhanceEncrypt = true;
	_unorderedParse = false;

	if (!_decryptedDataBuffer)
		_decryptedDataBuffer = (uint8_t*)malloc(_decryptedBufferLen);
}

void ARQParser::configUnorderedParse(bool enable)
{
	if (_enableDataEnhanceEncrypt == false && _arqChecksum == NULL)
		_unorderedParse = enable;
}

bool ARQParser::parse(uint8_t* buffer, int len, struct ParseResult* result)
{
	_parseResult = result;

	_buffer = buffer;
	_bufferLength = len;
	_bufferOffset = 0;

	if (len < ARQConstant::PackageMimimumLength)
	{
		LOG_ERROR("Received short UDP ARQ data. len: %d. socket: %d, endpoint: %s", _bufferLength, _socket, _endpoint);
		return false;
	}

	if (_encryptor)
	{
		//-- prevent the resent ecdh package disturbs the parsing process.
		if (_ecdhCopy == NULL) ;
		else
		{
			if (ecdhCopyExpiredTMS <= slack_real_msec())
			{
				delete _ecdhCopy;
				_ecdhCopy = NULL;
			}
			else
			{
				if ((int)(_ecdhCopy->len) == len && memcmp(_ecdhCopy->data, _buffer, len) == 0)
					return true;
			}
		}

		_encryptor->packageDecrypt(_decryptedBuffer, buffer, len);
		_buffer = _decryptedBuffer;
	}

	//-- version checking
	if (*_buffer != _parseResult->protocolVersion)
	{
		if (*_buffer <= ARQConstant::Version)
			_parseResult->protocolVersion = *_buffer;
		else
		{
			uint8_t version = *_buffer;
			LOG_ERROR("Received unsupported UDP data version: %d, len: %d. socket: %d, endpoint: %s", (int)version, _bufferLength, _socket, _endpoint);
			return false;
		}
	}

	uint8_t type = *(_buffer + 1);
	uint8_t flag = *(_buffer + 2);

	if (type == (uint8_t)ARQType::ARQ_ASSEMBLED)
		return processAssembledPackage();

	bool discardable = flag & ARQFlag::ARQ_Discardable;
	bool monitored = flag & ARQFlag::ARQ_Monitored;

	bool packageIsDiscardable = discardable && !monitored;
	if (packageIsDiscardable == false)
		return processReliableAndMonitoredPackage(type, flag);
	else
		return processPackage(type, flag);
}

bool ARQParser::processAssembledPackage()
{
	int remainedBufferLength = _bufferLength - ARQConstant::AssembledPackageHeaderSize;
	uint8_t* includedPackageBufferHeader = _buffer + ARQConstant::AssembledPackageHeaderSize;

	while (remainedBufferLength > 0)
	{
		uint16_t packageLength = be16toh(*((uint16_t*)includedPackageBufferHeader));

		_buffer = includedPackageBufferHeader + ARQConstant::AssembledPackageLengthFieldSize - 1;	//-- -1: Version field size.
		_bufferLength = packageLength + 1;	//-- +1: Version field size.
		_bufferOffset = 0;

		uint8_t type = *(_buffer + 1);
		uint8_t flag = *(_buffer + 2);

		bool discardable = flag & ARQFlag::ARQ_Discardable;
		bool monitored = flag & ARQFlag::ARQ_Monitored;

		bool rev;
		bool packageIsDiscardable = discardable && !monitored;
		if (packageIsDiscardable == false)
			rev = processReliableAndMonitoredPackage(type, flag);
		else
			rev = processPackage(type, flag);
		
		if (!rev)
			return false;

		//-- Next include package.
		int includedDataLength = ARQConstant::AssembledPackageLengthFieldSize + packageLength;
		includedPackageBufferHeader += includedDataLength;
		remainedBufferLength -= includedDataLength;
	}
	return true;
}

bool ARQParser::processReliableAndMonitoredPackage(uint8_t type, uint8_t flag)
{
	uint8_t sign = *(_buffer + 3);
	uint32_t packageSeqBE = *((uint32_t*)(_buffer + 4));

	uint32_t packageSeq = be32toh(packageSeqBE);

	if (_arqChecksum)
	{
		/*
		 对于前置UDP代理，或者网关的情况，如果在收到 close 包，或者 idle 之前，代理/网关的端口被重用了，会导致
		 完全相同的四元组 (src.ip, src.port, dest.ip, dest.port) 产生，也就是 src 的 endpoint 相同。
		 如果这个 case 发生了，在这里开辟新的分支处理，判断如果设置有 ARQFlag::ARQ_FirstPackage
		 则替换当前 session，改为新的 arq check sum，并重新清理当前的cache，然后通知 UDP IO Buffer
		 及更上层的状态转换。
		 因为涉及到的操作和协同非常复杂，在这里仅作简单处理：关闭当前链接，等待对端重发 flag 为
		 ARQFlag::ARQ_FirstPackage 的 UDP 数据包。

		 完备的复杂处理请联系： wangxing.shi@ilivedata.com, 或者 swxlion@hotmail.com。

		 注意：对于公网，若允许在四元组冲突时重建连接，则 UDP ARQ 链接极易被恶意的 ARQFlag::ARQ_FirstPackage
		 UDP 包打断。因此不建议在非安全环境，或者公网，打开重建连接的选项。
		*/
		if (_replaceConnectionWhenConnectionReentry && (flag & ARQFlag::ARQ_FirstPackage) && _firstPackageUDPSeq != packageSeq)
		{
			requireClose = true;
			return false;
		}

		if (packageSeq == lastUDPSeq)
		{
			_parseResult->receivedPriorSeqs = true;
			
			if (_disorderedCache.size() > 0)
				_invalidChecker.updateInvalidPackageCount();

			return false;
		}

		{
			uint32_t a = packageSeq - lastUDPSeq;
			uint32_t b = lastUDPSeq - packageSeq;

			if (b <= a)
			{
				_parseResult->receivedPriorSeqs = true;

				if (_disorderedCache.size() > 0)
					_invalidChecker.updateInvalidPackageCount();

				return false;
			}
		}

		if (_arqChecksum->check(packageSeqBE, sign) == false)
		{
			_invalidChecker.updateInvalidPackageCount();

			LOG_WARN("Received faked UDP data seq: %u, type: %d, flag: %d, len: %d. socket: %d, endpoint: %s",
				packageSeq, (int)type, (int)flag, _bufferLength, _socket, _endpoint);

			return false;
		}

		if (packageSeq == lastUDPSeq + 1)
		{
			_invalidChecker.updateValidStatus();

			processPackage(type, flag);

			lastUDPSeq = packageSeq;
			
			if (_unorderedParse)
				checkLastUDPSeq();
			else
				processCachedPackageFromSeq();

			_parseResult->canbeFeedbackUNA = true;
			return true;
		}

		_invalidChecker.updateInvalidPackageCount();

		if (_unorderedParse)
			return aheadProcessReliableAndMonitoredPackage(type, flag, packageSeq);
		else
		{
			cacheCurrentUDPPackage(packageSeq);
			return true;
		}
	}
	else if (flag & ARQFlag::ARQ_FirstPackage)
	{
		_arqChecksum = new ARQChecksum(packageSeqBE, sign);
		lastUDPSeq = packageSeq;
		_firstPackageUDPSeq = packageSeq;

		_invalidChecker.firstPackageReceived();
		_invalidChecker.updateValidStatus();

		processPackage(type, flag);

		if (_unorderedParse)
			verifyAndProcessCachedPackages();
		else
		{
			verifyCachedPackage(packageSeq);
			processCachedPackageFromSeq();
		}
		
		_parseResult->canbeFeedbackUNA = true;
		return true;
	}
	else
	{
		_invalidChecker.startCheck();
		cacheCurrentUDPPackage(packageSeq);
		return true;
	}
}

bool ARQParser::aheadProcessReliableAndMonitoredPackage(uint8_t type, uint8_t flag, uint32_t packageSeq)
{
	if (unprocessedReceivedSeqs.find(packageSeq) != unprocessedReceivedSeqs.end())
		return true;
	
	uint32_t tolerance = lastUDPSeq + Config::UDP::_disordered_seq_tolerance;
	if (lastUDPSeq < tolerance)
	{
		if (packageSeq > tolerance)
		{
			LOG_WARN("Received future or expired UDP data seq: %u, type: %d, flag: %d, len: %d. Current last seq: %u. socket: %d, endpoint: %s",
				packageSeq, (int)type, (int)flag, _bufferLength, lastUDPSeq, _socket, _endpoint);

			return true;
		}
	}
	else
	{
		if (packageSeq > tolerance && packageSeq < lastUDPSeq)
		{
			LOG_WARN("Received future or expired UDP data seq: %u, type: %d, flag: %d, len: %d. Current last seq: %u. socket: %d, endpoint: %s",
				packageSeq, (int)type, (int)flag, _bufferLength, lastUDPSeq, _socket, _endpoint);

			return true;
		}
	}

	unprocessedReceivedSeqs.insert(packageSeq);
	return processPackage(type, flag);
}

bool ARQParser::processPackage(uint8_t type, uint8_t flag)
{
	if (type == (uint8_t)ARQType::ARQ_COMBINED)
		return parseCOMBINED();

	if (type == (uint8_t)ARQType::ARQ_DATA)
		return parseDATA();

	if (type == (uint8_t)ARQType::ARQ_ACKS)
		return parseACKS();

	if (type == (uint8_t)ARQType::ARQ_UNA)
		return parseUNA();

	if (type == (uint8_t)ARQType::ARQ_FORCESYNC)
		return parseForceSync();

	if (type == (uint8_t)ARQType::ARQ_ECDH)
		return parseECDH();

	if (type == (uint8_t)ARQType::ARQ_HEARTBEAT)
		return parseHEARTBEAT();

	if (type == (uint8_t)ARQType::ARQ_CLOSE)
	{
		requireClose = true;
		return true;
	}

	LOG_ERROR("Received unsupported UDP data type: %d, flag: %d, len: %d. socket: %d, endpoint: %s",
		(int)type, (int)flag, _bufferLength, _socket, _endpoint);

	return false;
}

void ARQParser::verifyCachedPackage(uint32_t baseUDPSeq)
{
	std::unordered_set<uint32_t> fakedPackageSeqs;

	for (auto& pp: _disorderedCache)
	{
		uint8_t sign = *(pp.second->data + 3);
		uint32_t packageSeqBE = *((uint32_t*)(pp.second->data + 4));

		if (_arqChecksum->check(packageSeqBE, sign) == false)
		{
			delete pp.second;
			fakedPackageSeqs.insert(pp.first);
		}
		else
			unprocessedReceivedSeqs.insert(pp.first);
	}

	for (auto seq: fakedPackageSeqs)
		_disorderedCache.erase(seq);


	if (!fakedPackageSeqs.empty())
	{
		LOG_WARN("Clear %u cached fake UDP packages. socket: %d, endpoint: %s",
			fakedPackageSeqs.size(), _socket, _endpoint);
	}

	unprocessedReceivedSeqs.erase(lastUDPSeq);
}

void ARQParser::cacheCurrentUDPPackage(uint32_t packageSeq)
{
	uint8_t type = *(_buffer + 1);
	uint8_t flag = *(_buffer + 2);

	if (_disorderedCache.find(packageSeq) != _disorderedCache.end())
	{
		// LOG_WARN("Received duplicated UDP data seq: %u, type: %d, flag: %d, len: %d. socket: %d, endpoint: %s",
		//	packageSeq, (int)type, (int)flag, _bufferLength, _socket, _endpoint);

		return;
	}

	if (_arqChecksum)
	{
		uint32_t tolerance = lastUDPSeq + Config::UDP::_disordered_seq_tolerance;
		if (lastUDPSeq < tolerance)
		{
			if (packageSeq > tolerance)
			{
				LOG_WARN("Received future or expired UDP data seq: %u, type: %d, flag: %d, len: %d. Current last seq: %u. socket: %d, endpoint: %s",
					packageSeq, (int)type, (int)flag, _bufferLength, lastUDPSeq, _socket, _endpoint);

				return;
			}
		}
		else
		{
			if (packageSeq > tolerance && packageSeq < lastUDPSeq)
			{
				LOG_WARN("Received future or expired UDP data seq: %u, type: %d, flag: %d, len: %d. Current last seq: %u. socket: %d, endpoint: %s",
					packageSeq, (int)type, (int)flag, _bufferLength, lastUDPSeq, _socket, _endpoint);

				return;
			}
		}

		if (_disorderedCache.size() > (size_t)Config::UDP::_disordered_seq_tolerance)
		{
			requireClose = true;

			LOG_WARN("Received disordered packages (received the first package) count (%d) tauch the limitation, virtual connection will judge as invalid. socket: %d, endpoint: %s",
					(int)_disorderedCache.size(), _socket, _endpoint);

			return;
		}
	}
	else
	{
		if (_disorderedCache.size() >= (size_t)Config::UDP::_disordered_seq_tolerance_before_first_package_received)
		{
			requireClose = true;

			LOG_WARN("Received disordered packages (without the first package) count (%d) tauch the limitation, virtual connection will judge as invalid. socket: %d, endpoint: %s",
					(int)_disorderedCache.size(), _socket, _endpoint);

			return;
		}
	}

	ClonedBuffer* cb = new ClonedBuffer(_buffer, _bufferLength);
	_disorderedCache[packageSeq] = cb;

	if (_arqChecksum)
		unprocessedReceivedSeqs.insert(packageSeq);
}

void ARQParser::processCachedPackageFromSeq()
{
	while (true)
	{
		auto it = _disorderedCache.find(lastUDPSeq + 1);
		if (it == _disorderedCache.end())
			return;

		ClonedBuffer* cb = it->second;
		_buffer = cb->data;
		_bufferLength = cb->len;
		_bufferOffset = 0;

		uint8_t type = *(_buffer + 1);
		uint8_t flag = *(_buffer + 2);

		processPackage(type, flag);

		lastUDPSeq = it->first;
		_disorderedCache.erase(it);
		delete cb;

		unprocessedReceivedSeqs.erase(lastUDPSeq);
		_parseResult->canbeFeedbackUNA = true;
	}
}

void ARQParser::verifyAndProcessCachedPackages()
{
	int fakedSeqCount = 0;

	for (auto& pp: _disorderedCache)
	{
		uint8_t sign = *(pp.second->data + 3);
		uint32_t packageSeqBE = *((uint32_t*)(pp.second->data + 4));

		if (_arqChecksum->check(packageSeqBE, sign) == true)
		{
			unprocessedReceivedSeqs.insert(pp.first);

			ClonedBuffer* cb = pp.second;
			_buffer = cb->data;
			_bufferLength = cb->len;
			_bufferOffset = 0;

			uint8_t type = *(_buffer + 1);
			uint8_t flag = *(_buffer + 2);

			processPackage(type, flag);
		}
		else
			fakedSeqCount += 1;
		
		delete pp.second;
	}

	_disorderedCache.clear();


	if (fakedSeqCount != 0)
	{
		LOG_WARN("Clear %d cached fake UDP packages. socket: %d, endpoint: %s",
			fakedSeqCount, _socket, _endpoint);
	}

	unprocessedReceivedSeqs.erase(lastUDPSeq);
	while (unprocessedReceivedSeqs.find(lastUDPSeq + 1) != unprocessedReceivedSeqs.end())
	{
		lastUDPSeq += 1;
		unprocessedReceivedSeqs.erase(lastUDPSeq);
	}
}

void ARQParser::checkLastUDPSeq()
{
	while (unprocessedReceivedSeqs.find(lastUDPSeq + 1) != unprocessedReceivedSeqs.end())
	{
		lastUDPSeq += 1;
		unprocessedReceivedSeqs.erase(lastUDPSeq);
	}
}

bool ARQParser::parseCOMBINED()
{
	if (_bufferLength < ARQConstant::CombinedPackageMimimumLength)
	{
		LOG_ERROR("Received short Combined UDP ARQ data. len: %d. socket: %d, endpoint: %s", _bufferLength, _socket, _endpoint);
		return false;
	}
	
	_bufferOffset = 8;

	while (_bufferOffset < _bufferLength)
	{
		uint8_t type = *(_buffer + _bufferOffset);
		uint8_t flag = *(_buffer + _bufferOffset + 1);
		uint16_t bytes = be16toh(*((uint16_t*)(_buffer + _bufferOffset + 2)));

		if (_bufferOffset + 4 + bytes > _bufferLength)
		{
			LOG_ERROR("Received invalid short Combined UDP ARQ data. Require len: %d, real len: %d. socket: %d, endpoint: %s",
				_bufferOffset + 4 + bytes, _bufferLength, _socket, _endpoint);
			return false;
		}

		bool rev = false;
		if (type == (uint8_t)ARQType::ARQ_DATA)
			rev = parseDATA();

		else if (type == (uint8_t)ARQType::ARQ_ACKS)
			rev = parseACKS();

		else if (type == (uint8_t)ARQType::ARQ_UNA)
			rev = parseUNA();

		else if (type == (uint8_t)ARQType::ARQ_FORCESYNC)
			rev = parseForceSync();

		else if (type == (uint8_t)ARQType::ARQ_ECDH)
			rev = parseECDH();

		else if (type == (uint8_t)ARQType::ARQ_CLOSE)
		{
			requireClose = true;
			return true;
		}

		else
		{
			LOG_ERROR("Received unsupported Combined UDP data sub type: %d, sub flag: %d, len: %d. socket: %d, endpoint: %s",
				(int)type, (int)flag, (int)bytes, _socket, _endpoint);
			return false;
		}

		if (!rev)
			return false;

		_bufferOffset += 4 + bytes;
	}

	return true;
}

bool ARQParser::assembleSegments(uint16_t packageId)
{
	auto it = _uncompletedPackages.find(packageId);
	UDPUncompletedPackage* up = it->second;

	_uncompletedPackages.erase(it);
	uncompletedPackageSegmentCount -= (int)(up->cache.size());

	size_t bufferSize = up->cachedSegmentSize;
	uint8_t* buffer = (uint8_t*)malloc(bufferSize);
	uint8_t* spos = buffer;
	for (auto& pp: up->cache)
	{
		memcpy(spos, pp.second->data, pp.second->len);
		spos += pp.second->len;
	}

	delete up;
	bool rev = decodeBuffer(buffer, bufferSize);
	free(buffer);

	return rev;
}

bool ARQParser::decodeBuffer(uint8_t* buffer, uint32_t len)
{
	if (len < (uint32_t)FPMessage::_HeaderLength)
	{
		LOG_ERROR("Invalid FPNN data, which maybe truncated. Received data length is %u.", len);
		return false;
	}

	char* buffHeader = (char*)(buffer);

	//-- FPMessage::isTCP() will changed as FPMessage::isFPNN().
	if (!FPMessage::isTCP((char *)buffHeader))
	{
		LOG_ERROR("Invalid data, which is not encoded as FPNN protocol package. Invalid length is %u.", len);
		return false;
	}

	int currPackageLen = (int)(sizeof(FPMessage::Header) + FPMessage::BodyLen((char *)buffHeader));
	if (len < (uint32_t)currPackageLen)
	{
		LOG_ERROR("Invalid data. Required package length is %d, but data length is %u.", currPackageLen, len);
		return false;
	}

	const char *desc = "unknown";
	try
	{
		if (FPMessage::isQuest(buffHeader))
		{
			desc = "UDP quest";
			FPQuestPtr quest = Decoder::decodeQuest(buffHeader, currPackageLen);
			_parseResult->questList.push_back(quest);
		}
		else// if (FPMessage::isAnswer(buffHeader))
		{
			desc = "UDP answer";
			FPAnswerPtr answer = Decoder::decodeAnswer(buffHeader, currPackageLen);
			_parseResult->answerList.push_back(answer);
		}
		/*else
		{
			LOG_ERROR("Invalid data. FPNN MType is error. Drop data length %u.", len);
			return;
		}*/
	}
	catch (const FpnnError& ex)
	{
		LOG_ERROR("Decode %s error. Drop data length %u. Code: %d, error: %s.", desc, len, ex.code(), ex.what());
		return false;
	}
	catch (...)
	{
		LOG_ERROR("Decode %s error. Drop data length %u.", desc, len);
		return false;
	}

	return true;
}

bool ARQParser::parseDATA()
{
	uint8_t flag;
	uint16_t bytes = 0;
	uint8_t* pos;

	if (_bufferOffset == 0)
	{
		flag = *(_buffer + 2);
		bytes = (uint16_t)(_bufferLength - 8);
		pos = _buffer + 8;
	}
	else	//-- Combined data.
	{
		flag = *(_buffer + _bufferOffset + 1);
		bytes = be16toh(*((uint16_t*)(_buffer + _bufferOffset + 2)));
		pos = _buffer + _bufferOffset + 4;
	}

	bool discardable = flag & ARQFlag::ARQ_Discardable;
	uint8_t segmentFlag = flag & ARQFlag::ARQ_SegmentedMask;
	if (segmentFlag == 0)
	{
		if (flag & ARQFlag::ARQ_CancelledPackage)
			return true;
		
		if (_enableDataEnhanceEncrypt == false || discardable)
			return decodeBuffer(pos, bytes);
		else
		{
			_encryptor->dataDecrypt(_decryptedDataBuffer, pos, bytes);
			return decodeBuffer(_decryptedDataBuffer, bytes);
		}
	}
	else
	{
		bool isLastSegment = flag & ARQFlag::ARQ_LastSegmentData;
		uint16_t packageId = be16toh(*((uint16_t*)pos));
		uint32_t index;
		pos += 2;
		bytes -= 2;

		if (flag & ARQFlag::ARQ_CancelledPackage)
		{
			auto it = _uncompletedPackages.find(packageId);
			if (it != _uncompletedPackages.end())
			{
				delete it->second;
				_uncompletedPackages.erase(it);
			}
			return true;
		}

		if (segmentFlag == 0x04)
		{
			index = *pos;
			pos += 1;
			bytes -= 1;
		}
		else if (segmentFlag == 0x08)
		{
			index = be16toh(*((uint16_t*)pos));
			pos += 2;
			bytes -= 2;
		}
		else	//-- if (segmentFlag == 0x0C)
		{
			index = be32toh(*((uint32_t*)pos));
			pos += 4;
			bytes -= 4;
		}

		if ((int)(_uncompletedPackages.size()) >= Config::UDP::_max_cached_uncompleted_segment_package_count)
			dropExpiredCache(slack_real_sec() + Config::UDP::_max_cached_uncompleted_segment_seconds);

		if ((int)(_uncompletedPackages.size()) >= Config::UDP::_max_cached_uncompleted_segment_package_count)
		{
			if (discardable)
				return true;

			if (!dropDiscardableCachedUncompletedPackage())
			{
				LOG_ERROR("Received new segmented package data over the uncompleted package count limitation,"
					" packageId: %u, %d segments with %u uncompleted packages for this seesion, socket: %d, endpoint: %s",
					(uint32_t)packageId, uncompletedPackageSegmentCount, _uncompletedPackages.size(), _socket, _endpoint);
				return false;
			}
		}

		//-- Chinese comment：
		//-- 为了避免被恶意构造的错误包攻击，以下代码需要先检测，再处理内存。

		auto it = _uncompletedPackages.find(packageId);
		if (it == _uncompletedPackages.end())
		{
			UDPUncompletedPackage* up = new UDPUncompletedPackage();
			
			if (_enableDataEnhanceEncrypt == false || discardable)
				up->cacheClone(index, pos, bytes);
			else
			{
				_encryptor->dataDecrypt(_decryptedDataBuffer, pos, bytes);
				up->cacheClone(index, _decryptedDataBuffer, bytes);
			}
			
			up->discardable = discardable;
			if (isLastSegment)
				up->count = index;

			_uncompletedPackages[packageId] = up;
			uncompletedPackageSegmentCount += 1;

			return true;
		}

		UDPUncompletedPackage* up = it->second;

		if (up->count != 0 && isLastSegment)
		{
			LOG_ERROR("Received conflicted UDP segmented data. PackageId: %u, old segment count %u,"
				" the new segment count %u. socket: %d, endpoint: %s",
				(uint32_t)packageId, up->count, index, _socket, _endpoint);
			return false;
		}

		//-- 防止恶意构造重复包攻击
		if (up->cache.find(index) != up->cache.end())
		{
			LOG_ERROR("Received conflicted UDP segmented data. PackageId: %u, duplicated segment incdex %u"
				" after duplicated UDP packages filter. socket: %d, endpoint: %s",
				(uint32_t)packageId, index, _socket, _endpoint);

			return false;
		}

		if (up->cachedSegmentSize + bytes > (uint32_t)Config::_max_recv_package_length)
		{
			LOG_ERROR("Received huge UDP segmented data. PackageId: %u, current size %u, current segments count %u."
				" socket: %d, endpoint: %s",
				(uint32_t)packageId, up->cachedSegmentSize + bytes, up->cache.size(), _socket, _endpoint);

			uncompletedPackageSegmentCount -= (int)(up->cache.size());
			delete up;
			_uncompletedPackages.erase(it);

			return false;
		}

		if (up->count == 0 && isLastSegment)
			up->count = index;

		if (_enableDataEnhanceEncrypt == false || discardable)
			up->cacheClone(index, pos, bytes);
		else
		{
			_encryptor->dataDecrypt(_decryptedDataBuffer, pos, bytes);
			up->cacheClone(index, _decryptedDataBuffer, bytes);
		}

		uncompletedPackageSegmentCount += 1;

		if (up->count == (uint32_t)(up->cache.size()))
			return assembleSegments(packageId);

		return true;
	}
}

bool ARQParser::parseACKS()
{
	uint16_t bytes = 0;
	uint8_t* pos;

	if (_bufferOffset == 0)
	{
		bytes = (uint16_t)(_bufferLength - 8);
		pos = _buffer + 8;
	}
	else	//-- Combined data.
	{
		bytes = be16toh(*((uint16_t*)(_buffer + _bufferOffset + 2)));
		pos = _buffer + _bufferOffset + 4;
	}
	
	if (bytes % 4)
	{
		LOG_ERROR("Received invalid UDP ACK/ACKS data. whole package len: %d, included %d bytes ack(s) data. socket: %d, endpoint: %s",
			_bufferLength, (int)bytes, _socket, _endpoint);
		return false;
	}

	for (uint16_t i = 0; i < bytes; i += 4, pos += 4)
	{
		uint32_t ackSeq = be32toh(*((uint32_t*)pos));
		_parseResult->receivedAcks.insert(ackSeq);
	}

	return true;
}

bool ARQParser::parseUNA()
{
	if (_bufferOffset == 0)
	{
		if (_bufferLength != 12)
		{
			LOG_ERROR("Received invalid UDP UNA data. len: %d. socket: %d, endpoint: %s", _bufferLength, _socket, _endpoint);
			return false;
		}

		uint32_t receivedUNA = be32toh(*((uint32_t*)(_buffer + 8)));
		_parseResult->receiveUNA(receivedUNA);
		return true;
	}
	else	//-- Combined data.
	{
		uint32_t receivedUNA = be32toh(*((uint32_t*)(_buffer + _bufferOffset + 4)));
		_parseResult->receiveUNA(receivedUNA);
		return true;
	}
}

bool ARQParser::parseForceSync()
{
	_parseResult->requireForceSync = true;
	return true;
}

bool ARQParser::parseECDH()
{
	uint16_t bytes = 0;
	uint8_t* pos;

	//----- 01. General Info ------//

	if (_bufferOffset == 0)
	{
		bytes = (uint16_t)(_bufferLength - 8);
		pos = _buffer + 8;
	}
	else	//-- Combined data.
	{
		bytes = be16toh(*((uint16_t*)(_buffer + _bufferOffset + 2)));
		pos = _buffer + _bufferOffset + 4;
	}

	//----- Config Checking ------//

	if (_parseResult->keyExchanger == NULL)
	{
		LOG_ERROR("Received UDP ECDH package, but key exchanger is NOT configurated. Package len: %d, "
			"included %d bytes ECDH data. Socket: %d, endpoint: %s",
			_bufferLength, (int)bytes, _socket, _endpoint);
		return false;
	}

	if (_encryptor)
	{
		LOG_ERROR("Received UDP ECDH data, but UDPParser encryptor has inited. socket: %d, endpoint: %s",
			_socket, _endpoint);
		return false;
	}

	//----- 02. Package Public Key ------//

	uint8_t param = *pos;
	bool reinforcePackage = ((param & 0x80) == 0x80);
	uint8_t pubKeyLen = param & 0x7f;

	if (bytes < 1 + pubKeyLen)
	{
		LOG_ERROR("Received invalid UDP ECDH data. whole package len: %d, included %d bytes ECDH data."
			" Package public Key len is %d. socket: %d, endpoint: %s",
			_bufferLength, (int)bytes, (int)pubKeyLen, _socket, _endpoint);
		return false;
	}

	pos += 1;
	std::string packagePublicKey((char*)pos, pubKeyLen);
	pos += pubKeyLen;

	UDPEncryptor::EncryptorPair encryptors;

	//----- 03. Data Public Key ------//

	if (bytes > 1 + pubKeyLen)
	{
		param = *pos;
		bool reinforceData = ((param & 0x80) == 0x80);
		uint8_t dataPubKeyLen = param & 0x7f;

		if (bytes != 1 + pubKeyLen + 1 + dataPubKeyLen)
		{
			LOG_ERROR("Received invalid UDP ECDH data. whole package len: %d, included %d bytes ECDH data."
				" Package public Key len is %d, data public Key len is %d. socket: %d, endpoint: %s",
				_bufferLength, (int)bytes, (int)pubKeyLen, (int)dataPubKeyLen, _socket, _endpoint);
			return false;
		}

		pos += 1;
		std::string dataPublicKey((char*)pos, pubKeyLen);
		pos += pubKeyLen;

		encryptors = UDPEncryptor::createPair(_parseResult->keyExchanger, packagePublicKey,
			reinforcePackage, dataPublicKey, reinforceData);

		_unorderedParse = false;
		_enableDataEnhanceEncrypt = true;
		_parseResult->enableDataEncryption = true;

		if (!_decryptedDataBuffer)
			_decryptedDataBuffer = (uint8_t*)malloc(_decryptedBufferLen);
	}
	else
	{
		encryptors = UDPEncryptor::createPair(_parseResult->keyExchanger, packagePublicKey, reinforcePackage);
	}

	//----- 04. Process encryptors ------//

	_encryptor = encryptors.receiver;
	_parseResult->sendingEncryptor = encryptors.sender;

	if (!_decryptedBuffer)
		_decryptedBuffer = (uint8_t*)malloc(_decryptedBufferLen);

	_ecdhCopy = new ClonedBuffer(_buffer, _bufferLength);
	ecdhCopyExpiredTMS = slack_real_msec() + Config::UDP::_ecdh_copy_retained_milliseconds;
	
	return true;
}

bool ARQParser::parseHEARTBEAT()
{
	requireKeepLink = true;
	//-- Ignored. This package just unsing for keep NAT port mapping or test network speed.
	return true;
}

bool ARQParser::dropDiscardableCachedUncompletedPackage()
{
	bool found = false;
	uint16_t packageId;

	for (auto& pp: _uncompletedPackages)
	{
		if (pp.second->discardable)
		{
			found = true;
			packageId = pp.first;
			uncompletedPackageSegmentCount -= (int)(pp.second->cache.size());
			delete pp.second;
			break;
		}
	}

	if (found)
		_uncompletedPackages.erase(packageId);

	return found;
}

void ARQParser::dropExpiredCache(int64_t threshold)
{
	std::unordered_set<uint16_t> packageIds;
	for (auto& pp: _uncompletedPackages)
	{
		if (pp.second->createSeconds <= threshold)
		{
			LOG_ERROR("Uncompleted package %u with %u sewgments will be dropped by expired. socket: %d, endpoint: %s",
				(uint32_t)(pp.first), pp.second->cache.size(), _socket, _endpoint);

			packageIds.insert(pp.first);
			uncompletedPackageSegmentCount -= (int)(pp.second->cache.size());
			delete pp.second;
		}
	}

	for (uint16_t packageId: packageIds)
		_uncompletedPackages.erase(packageId);
}
