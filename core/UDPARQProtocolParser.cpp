#include <stdlib.h>
#include <string.h>
#include <limits>
#include "FPLog.h"
#include "msec.h"
#include "Config.h"
#include "Decoder.h"
#include "UDPARQProtocolParser.h"

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

ARQChecksum::ARQChecksum(uint32_t firstSeqBE, uint8_t factor): _factor(factor)
{
	uint8_t* fseqBE = (uint8_t*)&firstSeqBE;
	fseqBE[1] ^= factor;
	fseqBE[3] ^= factor;

	uint8_t cyc = factor % 32;
	_preprossedFirstSeq = (*fseqBE >> cyc) | (*fseqBE << (32 - cyc));
}

bool ARQChecksum::check(uint32_t udpSeqBE, uint8_t checksum)
{
	return genChecksum(udpSeqBE) == checksum;
}

uint8_t ARQChecksum::genChecksum(uint32_t udpSeqBE)
{
	uint8_t rfactor = ~_factor;
	uint8_t* cseqBE = (uint8_t*)&udpSeqBE;

	cseqBE[0] ^= rfactor;
	cseqBE[2] ^= rfactor;

	uint8_t cyc = rfactor % 32;
	uint32_t newC = (*cseqBE << cyc) | (*cseqBE >> (32 - cyc));
	uint32_t res = _preprossedFirstSeq ^ newC;
	uint8_t* resByte = (uint8_t*)&res;

	uint8_t value = resByte[0] + resByte[1] + resByte[2] + resByte[3];
	return value;
}

bool ARQChecksum::isSame(uint8_t factor)
{
	return _factor == factor;
}

bool ARQParser::parse(uint8_t* buffer, int len, struct ParseResult* result)
{
	_parseResult = result;

	_buffer = buffer;
	_bufferLength = len;
	_bufferOffset = 0;

	if (len < 8)
	{
		LOG_ERROR("Received short UDP ARQ data. len: %d. socket: %d, endpoint: %s", _bufferLength, _socket, _endpoint);
		return false;
	}

	//-- version checking
	if (*buffer != 1)
	{
		uint8_t version = *buffer;
		LOG_ERROR("Received unsupported UDP data version: %d, len: %d. socket: %d, endpoint: %s", (int)version, _bufferLength, _socket, _endpoint);
		return false;
	}

	uint8_t type = *(buffer + 1);
	uint8_t flag = *(buffer + 2);

	bool discardable = flag & ARQFlag::ARQ_Discardable;
	bool monitored = flag & ARQFlag::ARQ_Monitored;

	bool packageIsDiscardable = discardable && !monitored;
	if (packageIsDiscardable == false)
		return processReliableAndMonitoredPackage(type, flag);
	else
		return processPackage(type, flag);
}

bool ARQParser::processPackage(uint8_t type, uint8_t flag)
{
	if (type & (uint8_t)ARQType::ARQ_COMBINED)
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
		uint8_t factor = *(pp.second->data + 3);
		uint32_t packageSeqBE = *((uint32_t*)(pp.second->data + 4));

		if (_arqChecksum->check(packageSeqBE, factor) == false)
		{
			delete pp.second;
			fakedPackageSeqs.insert(pp.first);
		}
		else
			receivedSeqs.insert(pp.first);
	}

	for (auto seq: fakedPackageSeqs)
		_disorderedCache.erase(seq);


	if (!fakedPackageSeqs.empty())
	{
		LOG_WARN("Clear %u cached fake UDP packages. socket: %d, endpoint: %s",
			fakedPackageSeqs.size(), _socket, _endpoint);
	}
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
		receivedSeqs.insert(packageSeq);
}

void ARQParser::EndpointQuaternionConflictError(uint8_t factor, uint8_t type, uint8_t flag)
{
	if (!_arqChecksum->isSame(factor) && (flag & ARQFlag::ARQ_FirstPackageMask))
	{
		requireClose = true;

		LOG_ERROR("Endpoint Quaternion Conflict. type: %d, flag: %d, endpoint: %s. If this error occurs in large numbers, "
			"please refering wangxing.shi@ilivedata.com or swxlion@hotmail.com to tell you server & business architecture, "
			"add a patch for the rare case.",
				(int)type, (int)flag, _endpoint);
	}
}

bool ARQParser::processReliableAndMonitoredPackage(uint8_t type, uint8_t flag)
{
	uint8_t factor = *(_buffer + 3);
	uint32_t packageSeqBE = *((uint32_t*)(_buffer + 4));

	uint32_t packageSeq = be32toh(packageSeqBE);

	if (_arqChecksum)
	{
		/*
		 * 对于前置UDP代理的情况，如果在 idle 之前，代理的端口被重用了，会导致完全相同的四元组
		 (src.ip, src.port, dest.ip, dest.port) 产生，也就是 src 的 endpoint 相同。
		 如果这个 case 发生了，在这里开辟新的分支处理，判断如果设置有 ARQFlag::ARQ_FirstPackageMask，
		 则替换当前 session，改为新的 arq check sum，并重新清理当前的cache，然后通知 UDP IO Buffer
		 及更上层的状态转换。
		 具体请联系： wangxing.shi@ilivedata.com, 或者 swxlion@hotmail.com。
		*/

		if (packageSeq == lastUDPSeq)
		{
			_parseResult->receivedPriorSeqs = true;
			
			if (_disorderedCache.size() > 0)
				_invalidChecker.updateInvalidPackageCount();

			EndpointQuaternionConflictError(factor, type, flag);
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

				EndpointQuaternionConflictError(factor, type, flag);
				return false;
			}
		}

		if (_arqChecksum->check(packageSeqBE, factor) == false)
		{
			_invalidChecker.updateInvalidPackageCount();

			LOG_WARN("Received faked UDP data seq: %u, type: %d, flag: %d, len: %d. socket: %d, endpoint: %s",
				packageSeq, (int)type, (int)flag, _bufferLength, _socket, _endpoint);

			EndpointQuaternionConflictError(factor, type, flag);
			return false;
		}

		if (packageSeq == lastUDPSeq + 1)
		{
			_invalidChecker.updateValidStatus();

			processPackage(type, flag);

			lastUDPSeq = packageSeq;
			processCachedPackageFromSeq();
			_parseResult->canbeFeedbackUNA = true;
			return true;
		}

		_invalidChecker.updateInvalidPackageCount();
		cacheCurrentUDPPackage(packageSeq);
		return true;
	}
	else if (flag & ARQFlag::ARQ_FirstPackageMask)
	{
		_arqChecksum = new ARQChecksum(packageSeqBE, factor);
		lastUDPSeq = packageSeq;

		_invalidChecker.firstPackageReceived();
		_invalidChecker.updateValidStatus();

		processPackage(type, flag);

		verifyCachedPackage(packageSeq);
		processCachedPackageFromSeq();
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

void ARQParser::processCachedPackageFromSeq()
{
	receivedSeqs.erase(lastUDPSeq);

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

		receivedSeqs.erase(lastUDPSeq);
		_parseResult->canbeFeedbackUNA = true;
	}
}

bool ARQParser::parseCOMBINED()
{
	if (_bufferLength < 16)
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

	uint8_t segmentFlag = flag & ARQFlag::ARQ_SegmentedMask;
	if (segmentFlag == 0)
	{
		return decodeBuffer(pos, bytes);
	}
	else
	{
		bool isLastSegment = flag & ARQFlag::ARQ_LastSegmentMask;
		uint16_t packageId = be16toh(*((uint16_t*)pos));
		uint32_t index;
		pos += 2;
		bytes -= 2;

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

		bool discardable = flag & ARQFlag::ARQ_Discardable;
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
			up->createSeconds = slack_real_sec();
			up->cachedSegmentSize = bytes;
			up->count = isLastSegment ? index : 0;
			up->discardable = discardable;
			up->cache[index] = new ClonedBuffer(pos, bytes);

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

		up->cache[index] = new ClonedBuffer(pos, bytes);
		up->cachedSegmentSize += bytes;
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
	//-- TODO in next version.
	LOG_ERROR("Received UDP ECDH data! Current this version hasn't supported the encrypted communication."
		" Please tell swxlion to add the supporting functions. Package len: %d. socket: %d, endpoint: %s",
		_bufferLength, _socket, _endpoint);
	return false;
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
