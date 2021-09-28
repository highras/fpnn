#include <errno.h>
#include <memory>
#include <string.h>
#include "msec.h"
#include "Config.h"
#include "Decoder.h"
#include "UDPIOBuffer.h"
#include <sstream>
using namespace fpnn;

struct ARQConstant
{
	static const int PackageHeaderSize = 8;
	static const int SectionHeaderSize = 4;
	static const int SegmentPackageSeqSize = 2;

	static const uint8_t Version = 1;

	static const int UDPSeqOffset = 4;
};

CurrentSendingBuffer::CurrentSendingBuffer():
	rawBuffer(0), bufferLength(0), dataBuffer(0), dataLength(0),
	sendDone(false), requireUpdateSeq(false), resendingPackage(0)
{
}

void CurrentSendingBuffer::init(int bufferSize)
{
	if (rawBuffer)
		free(rawBuffer);

	rawBuffer = (uint8_t*)malloc(bufferSize);
	bufferLength = (size_t)bufferSize;

	reset();
}

void CurrentSendingBuffer::reset()
{
	includeUNA = false;
	dataBuffer = rawBuffer;
	dataLength = 0;

	discardable = true;
	sentCount = 0;

	sendDone = false;
	requireUpdateSeq = false;
	resendingPackage = NULL;

	dataBuffer[0] = ARQConstant::Version;
	dataBuffer[1] = 0;
	dataBuffer[2] = 0;
}

void CurrentSendingBuffer::updateSendingInfo()
{
	if (resendingPackage)
		resendingPackage->updateSendingInfo();
	else
	{
		sentCount += 1;
		lastSentMsec = slack_real_msec();
	}
}

UDPPackage* CurrentSendingBuffer::dumpPackage()
{
	if (discardable)
		return NULL;

	if (resendingPackage)
	{
		if (resendingPackage->requireDeleted)
			delete resendingPackage;
		else
			resendingPackage->resending = false;

		return NULL;
	}

	UDPPackage* package = new UDPPackage();
	package->buffer = malloc(dataLength);
	package->len = dataLength;
	package->sentCount = sentCount;
	package->firstSentMsec = lastSentMsec;
	package->lastSentMsec = lastSentMsec;
	package->includeUNA = includeUNA;

	memcpy(package->buffer, dataBuffer, dataLength);
	
	return package;
}

void CurrentSendingBuffer::resendPackage(uint32_t seq, UDPPackage* package)
{
	resendingPackage = package;
	dataBuffer = (uint8_t*)(package->buffer);
	dataLength = package->len;

	discardable = false;
	packageSeq = seq;
	
	sendDone = false;
	requireUpdateSeq = false;

	resendingPackage->resending = true;
}

void CurrentSendingBuffer::setType(uint8_t type)
{
	*(dataBuffer + 1) = type;
}
void CurrentSendingBuffer::setType(ARQType type)
{
	*(dataBuffer + 1) = (uint8_t)type;
}
void CurrentSendingBuffer::addFlag(uint8_t flag)
{
	*(dataBuffer + 2) |= flag;
}
void CurrentSendingBuffer::setFlag(uint8_t flag)
{
	*(dataBuffer + 2) = flag;
}
void CurrentSendingBuffer::setFactor(uint8_t factor)
{
	*(dataBuffer + 3) = factor;
}
void CurrentSendingBuffer::setUDPSeq(uint32_t seqBE)
{
	uint32_t* seqBuffer = (uint32_t*)(dataBuffer + ARQConstant::UDPSeqOffset);
	*seqBuffer = seqBE;
}
void CurrentSendingBuffer::setComponentType(uint8_t* componentBegin, uint8_t type)
{
	*(componentBegin) = type;
}
void CurrentSendingBuffer::setComponentType(uint8_t* componentBegin, ARQType type)
{
	*(componentBegin) = (uint8_t)type;
}
void CurrentSendingBuffer::setComponentFlag(uint8_t* componentBegin, uint8_t flag)
{
	*(componentBegin + 1) = flag;
}
void CurrentSendingBuffer::setComponentBytes(uint8_t* componentBegin, size_t bytes)
{
	uint16_t* bytesBuffer = (uint16_t*)(componentBegin + 2);
	*bytesBuffer = htobe16((uint16_t)bytes);
}
void CurrentSendingBuffer::setDataComponentPackageSeq(uint8_t* segmentBegin, uint16_t seq)
{
	uint16_t* seqBuffer = (uint16_t*)segmentBegin;
	*seqBuffer = htobe16(seq);
}
size_t CurrentSendingBuffer::setDataComponentSegmentIndex(uint8_t* segmentBegin, uint32_t index)
{
	size_t size = 4;

	if (index < 0xFF)
	{
		size = 1;
		*(segmentBegin + ARQConstant::SegmentPackageSeqSize) = (uint8_t)index;
	}
	else if (index < 0xFFFF)
	{
		size = 2;
		uint16_t* idxBuffer = (uint16_t*)(segmentBegin + ARQConstant::SegmentPackageSeqSize);
		*idxBuffer = htobe16((uint16_t)index);
	}
	else
	{
		uint32_t* idxBuffer = (uint32_t*)(segmentBegin + ARQConstant::SegmentPackageSeqSize);
		*idxBuffer = htobe32(index);
	}

	return size;
}

bool CurrentSendingBuffer::changeCombinedPackageToSinglepackage()
{
	if (dataBuffer != rawBuffer)
		return false;

	uint8_t type = *(dataBuffer + ARQConstant::PackageHeaderSize);
	uint8_t flag = *(dataBuffer + ARQConstant::PackageHeaderSize + 1);

	uint32_t* secteion1 = (uint32_t*)dataBuffer;
	uint32_t* secteion2 = (uint32_t*)(dataBuffer + 4);
	uint32_t* secteion3 = (uint32_t*)(dataBuffer + 8);

	*secteion3 = *secteion2;
	*secteion2 = *secteion1;

	dataBuffer += 4;
	dataLength -= 4;
	setType(type);
	setFlag(flag);

	return true;
}

void ARQPeerSeqManager::updateLastUNA(uint32_t una)
{
	lastUNA = una;
	unaUpdated = true;
	unaAvailable = true;

	std::unordered_map<uint32_t, int64_t> remainedSeqs;
	for (auto& pp: feedbackedSeqs)
	{
		uint32_t a = pp.first - lastUNA;
		uint32_t b = lastUNA - pp.first;

		if (a < b)
			remainedSeqs[pp.first] = pp.second;
	}
	feedbackedSeqs.swap(remainedSeqs);
}

void ARQPeerSeqManager::newReceivedSeqs(const std::unordered_set<uint32_t>& newSeqs)
{
	//-- 需要优化
	receivedSeqs = newSeqs;
}

bool ARQPeerSeqManager::needSyncSeqStatus()
{
	//-- 如果上次 Acks 没有发送完，比如 MTU 1500， 但需要确认的 seqs 超过 400 条。所以需要判断 receivedSeqs.size() > 0。
	if (receivedSeqs.size() > 0 || slack_real_msec() - lastSyncMsec >= Config::UDP::_arq_seqs_sync_interval_milliseconds)
	{
		cleanReceivedSeqs();

		if (unaUpdated || repeatUNA || receivedSeqs.size() > 0)
			return true;
	}

	return false;
}

void ARQPeerSeqManager::cleanReceivedSeqs()
{
	if (receivedSeqs.empty())
		return;

	std::unordered_set<uint32_t> extracted;
	int64_t threshold = slack_real_msec() - Config::UDP::_arq_reAck_interval_milliseconds;

	if (unaAvailable)
	{
		for (auto seq: receivedSeqs)
		{
			uint32_t a = seq - lastUNA;
			uint32_t b = lastUNA - seq;

			if (a < b)
			{
				auto it = feedbackedSeqs.find(seq);
				if (it == feedbackedSeqs.end())
					extracted.insert(seq);
				else if (it->second <= threshold)
					extracted.insert(seq);
			}
		}
	}
	else
	{
		for (auto seq: receivedSeqs)
		{
			auto it = feedbackedSeqs.find(seq);
			if (it == feedbackedSeqs.end())
				extracted.insert(seq);
			else if (it->second <= threshold)
				extracted.insert(seq);
		}
	}

	receivedSeqs.swap(extracted);
}

UDPIOBuffer::UDPIOBuffer(std::mutex* mutex, int socket, int MTU):
	_socket(socket), _MTU(MTU), _requireKeepAlive(false), _requireClose(false), /*_socketReadyForSending(true),*/
	_lastSentSec(0), _lastRecvSec(0), _activeCloseStatus(ActiveCloseStep::None), _arqChecksum(NULL),
	_sendToken(true), _recvToken(true), _mutex(mutex), /*_unaIncludeIndex(0),*/ _lastUrgentMsec(0)
{
	//-- Adjust availd MTU.
	_MTU -= 20;		//-- IP header size
	_MTU -= 8;		//-- UDP header size

	_currentSendingBuffer.init(_MTU + ARQConstant::SectionHeaderSize);

	_packageIdNumber = (uint16_t)slack_mono_msec();
	_untransmittedSeconds = Config::UDP::_max_untransmitted_seconds;
	_lastRecvSec = slack_real_sec();

	_recvBufferLen = FPNN_UDP_MAX_DATA_LENGTH;
	if (_recvBufferLen > Config::_max_recv_package_length)
		_recvBufferLen = Config::_max_recv_package_length;

	_recvBuffer = (uint8_t*)malloc(_recvBufferLen);

	_arqParser.changeLogInfo(socket, NULL);

	_UDPSeqBase = (uint32_t)slack_mono_msec();
}

UDPIOBuffer::~UDPIOBuffer()
{
	if (_arqChecksum)
		delete _arqChecksum;

	//-- clean receiving infos
	free(_recvBuffer);

	//-- clean sending info
	while (_dataQueue.size() > 0)
	{
		delete _dataQueue.front();
		_dataQueue.pop();
	}

	for (auto& pp: _unconformedMap)
		delete pp.second;
}

void UDPIOBuffer::updateEndpointInfo(const std::string& endpoint)
{
	_endpoint = endpoint;
	_arqParser.changeLogInfo(_socket, _endpoint.c_str());
}

void UDPIOBuffer::enableKeepAlive()
{
	//std::unique_lock<std::mutex> lck(*_mutex);
	_requireKeepAlive = true;
}

void UDPIOBuffer::setUntransmittedSeconds(int untransmittedSeconds)
{
	_untransmittedSeconds = untransmittedSeconds;
}

bool UDPIOBuffer::isTransmissionStopped()
{
	if (_lastRecvSec == 0 || _untransmittedSeconds < 0)
		return false;
	
	return slack_real_sec() - _untransmittedSeconds > _lastRecvSec;
}

void UDPIOBuffer::markActiveCloseSignal()
{
	{
		std::unique_lock<std::mutex> lck(*_mutex);
		if (_activeCloseStatus == ActiveCloseStep::None)
			_activeCloseStatus = ActiveCloseStep::Required;
	}
}

void UDPIOBuffer::sendCloseSignal(bool& needWaitSendEvent)
{
	{
		std::unique_lock<std::mutex> lck(*_mutex);
		if (_activeCloseStatus == ActiveCloseStep::None)
			_activeCloseStatus = ActiveCloseStep::Required;
	}

	bool blockByFlowControl;
	sendCachedData(needWaitSendEvent, blockByFlowControl);
}

uint8_t UDPIOBuffer::genChecksum(uint32_t udpSeqBE)
{
	if (_arqChecksum)
		return _arqChecksum->genChecksum(udpSeqBE);

	return (uint8_t)slack_real_msec();
}

void UDPIOBuffer::preparePackageCompleted(bool discardable, uint32_t udpSeq, uint32_t udpSeqBE, uint8_t factor)
{
	_currentSendingBuffer.discardable = discardable;
	_currentSendingBuffer.packageSeq = udpSeq;
	_currentSendingBuffer.requireUpdateSeq = false;

	if (!discardable && _arqChecksum == NULL)
	{
		_arqChecksum = new ARQChecksum(udpSeqBE, factor);
		_currentSendingBuffer.addFlag(ARQFlag::ARQ_FirstPackageMask);
	}

	if (discardable)
		_currentSendingBuffer.addFlag(ARQFlag::ARQ_Discardable);
}

void UDPIOBuffer::prepareClosePackage()
{
	_currentSendingBuffer.dataLength = ARQConstant::PackageHeaderSize;

	uint32_t randomSeq = (uint32_t)slack_real_msec();
	uint32_t seqBE = htobe32(randomSeq);

	_currentSendingBuffer.setType(ARQType::ARQ_CLOSE);
	_currentSendingBuffer.setFlag(ARQFlag::ARQ_Discardable);
	_currentSendingBuffer.setFactor(genChecksum(seqBE));

	_currentSendingBuffer.setUDPSeq(seqBE);

	preparePackageCompleted(true, randomSeq, seqBE, 0);
}

void UDPIOBuffer::prepareHeartbeatPackage()
{
	uint32_t udpSeq = (uint32_t)slack_real_msec();
	
	uint32_t seqBE = htobe32(udpSeq);
	uint8_t factor = genChecksum(seqBE);

	_currentSendingBuffer.setFlag(ARQFlag::ARQ_Discardable);
	_currentSendingBuffer.setType(ARQType::ARQ_HEARTBEAT);
	_currentSendingBuffer.setFactor(factor);

	_currentSendingBuffer.setUDPSeq(seqBE);

	uint64_t* timestamp = (uint64_t*)(_currentSendingBuffer.dataBuffer + ARQConstant::PackageHeaderSize);
	*timestamp = htobe64(slack_real_msec());

	_currentSendingBuffer.dataLength = ARQConstant::PackageHeaderSize + sizeof(uint64_t);

	preparePackageCompleted(true, udpSeq, seqBE, factor);
}

bool UDPIOBuffer::prepareUrgentARQSyncPackage(bool& blocked)
{
	blocked = false;
	bool feedbackForceSync = false;
	bool includeForceSyncSection = false;

	if (_unconformedMap.size() >= Config::UDP::_arq_urgent_seqs_sync_triggered_threshold
		&& _lastUrgentMsec <= slack_real_msec() - Config::UDP::_arq_urgnet_seqs_sync_interval_milliseconds)
	{
		includeForceSyncSection = true;
		_seqManager.requireForceSync = false;
	}
	else if (_seqManager.requireForceSync)
	{
		feedbackForceSync = true;
		_seqManager.requireForceSync = false;
	}
	else if (!_seqManager.needSyncSeqStatus())
		return false;

	int sectionCount = 0;

	_currentSendingBuffer.dataLength = ARQConstant::PackageHeaderSize;
	_currentSendingBuffer.requireUpdateSeq = true;

	_currentSendingBuffer.setType(ARQType::ARQ_COMBINED);

	if (includeForceSyncSection)
	{
		prepareForceSyncSection();
		sectionCount += 1;

		_lastUrgentMsec = slack_real_msec();
	}

	if ((feedbackForceSync && _seqManager.unaAvailable) || _seqManager.needSyncUNA())
	{
		prepareUNASection();
		sectionCount += 1;
	}

	_seqManager.cleanReceivedSeqs();
	if (_seqManager.needSyncAcks())
	{
		prepareAcksSection();
		sectionCount += 1;
	}

	_seqManager.lastSyncMsec = slack_real_msec();

	bool rev = completeCommonPackage(sectionCount);
	if (rev == false && sectionCount > 0)
		blocked = true;

	return rev;
}

bool UDPIOBuffer::prepareCommonPackage()
{
	int sectionCount = 0;

	_currentSendingBuffer.dataLength = ARQConstant::PackageHeaderSize;
	_currentSendingBuffer.requireUpdateSeq = true;

	_currentSendingBuffer.setType(ARQType::ARQ_COMBINED);

	if (_seqManager.needSyncSeqStatus())
	{
		if (_seqManager.needSyncUNA())
		{
			prepareUNASection();
			sectionCount += 1;
		}

		if (_seqManager.needSyncAcks())
		{
			prepareAcksSection();
			sectionCount += 1;
		}

		_seqManager.lastSyncMsec = slack_real_msec();
	}
	/*else
	{
		if (++_unaIncludeIndex >= Config::UDP::_arq_una_include_rate)
		{
			_unaIncludeIndex = 0;
			
			if (_seqManager.unaAvailable)
			{
				prepareUNASection();
				sectionCount += 1;
			}
		}
	}*/

	if (_sendingSegmentInfo.data)
	{
		if (prepareSegmentedDataSection(sectionCount))
		{
			sectionCount += 1;
		}
		else
			return completeCommonPackage(sectionCount);
	}

	while (_dataQueue.size() > 0)
	{
		if (_currentSendingBuffer.dataLength >= (size_t)_MTU)
		{
			return completeCommonPackage(sectionCount);
		}

		if (_dataQueue.front()->expiredMS < slack_real_msec())
		{
			delete _dataQueue.front();
			_dataQueue.pop();

			if (_dataQueue.empty())
				return completeCommonPackage(sectionCount);
		}

		if (prepareDataSection(sectionCount))
			sectionCount += 1;
		else
			return completeCommonPackage(sectionCount);
	}

	return completeCommonPackage(sectionCount);
}

bool UDPIOBuffer::completeCommonPackage(int sectionCount)
{
	if (sectionCount == 0)
	{
		_currentSendingBuffer.reset();
		return false;
	}

	if (sectionCount == 1)
		_currentSendingBuffer.changeCombinedPackageToSinglepackage();

	return updateUDPSeq();
}

void UDPIOBuffer::prepareForceSyncSection()
{
	uint8_t* componentBegin = _currentSendingBuffer.dataBuffer + _currentSendingBuffer.dataLength;

	_currentSendingBuffer.setComponentType(componentBegin, ARQType::ARQ_FORCESYNC);
	_currentSendingBuffer.setComponentFlag(componentBegin, ARQFlag::ARQ_Discardable);
	_currentSendingBuffer.setComponentBytes(componentBegin, 0);

	_currentSendingBuffer.dataLength += (size_t)ARQConstant::SectionHeaderSize;
}

void UDPIOBuffer::prepareUNASection()
{
	uint8_t* componentBegin = _currentSendingBuffer.dataBuffer + _currentSendingBuffer.dataLength;

	_currentSendingBuffer.setComponentType(componentBegin, ARQType::ARQ_UNA);
	_currentSendingBuffer.setComponentFlag(componentBegin, ARQFlag::ARQ_Discardable);
	_currentSendingBuffer.setComponentBytes(componentBegin, sizeof(uint32_t));
	
	uint32_t* unaBuffer = (uint32_t*)(componentBegin + ARQConstant::SectionHeaderSize);
	*unaBuffer = htobe32(_seqManager.lastUNA);

	_seqManager.unaUpdated = false;
	_seqManager.repeatUNA = false;
	_currentSendingBuffer.dataLength += (size_t)ARQConstant::SectionHeaderSize + sizeof(uint32_t);

	_currentSendingBuffer.includeUNA = true;
}

void UDPIOBuffer::prepareAcksSection()
{
	uint8_t* componentBegin = _currentSendingBuffer.dataBuffer + _currentSendingBuffer.dataLength;

	_currentSendingBuffer.setComponentType(componentBegin, ARQType::ARQ_ACKS);
	_currentSendingBuffer.setComponentFlag(componentBegin, ARQFlag::ARQ_Discardable);

	size_t remainedSize = _MTU - _currentSendingBuffer.dataLength - ARQConstant::SectionHeaderSize;

	size_t acksCount = remainedSize/sizeof(uint32_t);
	if (acksCount > _seqManager.receivedSeqs.size())
		acksCount = _seqManager.receivedSeqs.size();

	size_t bytes = acksCount * sizeof(uint32_t);
	_currentSendingBuffer.setComponentBytes(componentBegin, bytes);

	uint8_t* acksBuffer = componentBegin + ARQConstant::SectionHeaderSize;

	int64_t now = slack_real_msec();
	for (size_t i = 0; i < acksCount; i++)
	{
		auto it = _seqManager.receivedSeqs.begin();
		*((uint32_t*)acksBuffer) = htobe32(*it);
		acksBuffer += sizeof(uint32_t);

		_seqManager.feedbackedSeqs[*it] = now;
		_seqManager.receivedSeqs.erase(it);
	}

	_currentSendingBuffer.dataLength += (size_t)ARQConstant::SectionHeaderSize + bytes;
}

bool UDPIOBuffer::prepareSegmentedDataSection(int sectionCount)
{
	size_t idxSize = 4;
	uint8_t flag = ARQFlag::ARQ_4Bytes_SegmentIndex;

	//---------- checking -----------//

	if (_sendingSegmentInfo.nextIndex < 0xff)
	{
		idxSize = 1;
		flag = ARQFlag::ARQ_1Byte_SegmentIndex;
	}
	else if (_sendingSegmentInfo.nextIndex < 0xffff)
	{
		idxSize = 2;
		flag = ARQFlag::ARQ_2Bytes_SegmentIndex;
	}

	size_t segmentInfoSize = ARQConstant::SectionHeaderSize + ARQConstant::SegmentPackageSeqSize + idxSize;
	size_t remainedSpaceCorrectionSize = (sectionCount == 0) ? ARQConstant::SectionHeaderSize : 0;

	if ((size_t)_MTU <= _currentSendingBuffer.dataLength + segmentInfoSize - remainedSpaceCorrectionSize)
		return false;

	//-- 无符号数计算，+ remainedSpaceCorrectionSize 必须在减法运算之前，避免计算溢出。 
	size_t remainedSpace = (size_t)_MTU + remainedSpaceCorrectionSize - _currentSendingBuffer.dataLength - segmentInfoSize;
	size_t remainedDataSize = _sendingSegmentInfo.data->data->length() - _sendingSegmentInfo.offset;

	size_t bytes = remainedDataSize;
	if (remainedDataSize > remainedSpace)
		bytes = remainedSpace;
	else
		flag |= ARQFlag::ARQ_LastSegmentMask;

	//---------- assemble -----------//

	uint8_t* componentBegin = _currentSendingBuffer.dataBuffer + _currentSendingBuffer.dataLength;
	_currentSendingBuffer.setComponentType(componentBegin, ARQType::ARQ_DATA);

	if (_sendingSegmentInfo.data->discardable == false)
		_currentSendingBuffer.discardable = false;

	if (_currentSendingBuffer.discardable)
		flag |= ARQFlag::ARQ_Discardable;

	_currentSendingBuffer.setComponentFlag(componentBegin, flag);
	_currentSendingBuffer.setComponentBytes(componentBegin, bytes + ARQConstant::SegmentPackageSeqSize + idxSize);

	uint8_t* segmentBegin = componentBegin + ARQConstant::SectionHeaderSize;
	_currentSendingBuffer.setDataComponentPackageSeq(segmentBegin, _packageIdNumber);
	_currentSendingBuffer.setDataComponentSegmentIndex(segmentBegin, _sendingSegmentInfo.nextIndex);

	_sendingSegmentInfo.nextIndex += 1;

	uint8_t* dataBegin = segmentBegin + ARQConstant::SegmentPackageSeqSize + idxSize;
	memcpy(dataBegin, _sendingSegmentInfo.data->data->data() + _sendingSegmentInfo.offset, bytes);

	_sendingSegmentInfo.offset += bytes;

	if (_sendingSegmentInfo.offset >= _sendingSegmentInfo.data->data->length())
		_sendingSegmentInfo.reset();

	_currentSendingBuffer.dataLength += segmentInfoSize + bytes;
	return true;
}

bool UDPIOBuffer::prepareFirstSegmentedDataSection(size_t availableSpace)
{
	uint8_t* componentBegin = _currentSendingBuffer.dataBuffer + _currentSendingBuffer.dataLength;
	_currentSendingBuffer.setComponentType(componentBegin, ARQType::ARQ_DATA);

	UDPDataUnit* dataUnit = _dataQueue.front();
	uint8_t flag = ARQFlag::ARQ_1Byte_SegmentIndex;

	if (dataUnit->discardable)
		flag |= ARQFlag::ARQ_Discardable;
	else
		_currentSendingBuffer.discardable = false;
	
	_currentSendingBuffer.setComponentFlag(componentBegin, flag);

	const size_t indexSize = 1;
	if (availableSpace < ARQConstant::SegmentPackageSeqSize + indexSize)
		return false;

	size_t bytes = availableSpace - ARQConstant::SegmentPackageSeqSize - indexSize;

	_currentSendingBuffer.setComponentBytes(componentBegin, bytes + ARQConstant::SegmentPackageSeqSize + indexSize);

	_packageIdNumber += 1;
	//---------- assemble -----------//

	uint8_t* segmentBegin = componentBegin + ARQConstant::SectionHeaderSize;
	_currentSendingBuffer.setDataComponentPackageSeq(segmentBegin, _packageIdNumber);
	_currentSendingBuffer.setDataComponentSegmentIndex(segmentBegin, 1);

	uint8_t* dataBegin = segmentBegin + ARQConstant::SegmentPackageSeqSize + indexSize;
	memcpy(dataBegin, dataUnit->data->data(), bytes);

	_currentSendingBuffer.dataLength += ARQConstant::SectionHeaderSize + ARQConstant::SegmentPackageSeqSize + indexSize + bytes;

	//-- Update _sendingSegmentInfo
	_dataQueue.pop();
	_sendingSegmentInfo.data = dataUnit;
	_sendingSegmentInfo.nextIndex = 2;
	_sendingSegmentInfo.offset = bytes;

	return true;
}

void UDPIOBuffer::prepareSingleDataSection(size_t availableSpace)
{
	uint8_t* componentBegin = _currentSendingBuffer.dataBuffer + _currentSendingBuffer.dataLength;
	_currentSendingBuffer.setComponentType(componentBegin, ARQType::ARQ_DATA);

	UDPDataUnit* dataUnit = _dataQueue.front();
	if (dataUnit->discardable)
		_currentSendingBuffer.setComponentFlag(componentBegin, ARQFlag::ARQ_Discardable);
	else
	{
		_currentSendingBuffer.discardable = false;
		_currentSendingBuffer.setComponentFlag(componentBegin, 0);
	}

	size_t bytes = dataUnit->data->length();

	_currentSendingBuffer.setComponentBytes(componentBegin, bytes);

	uint8_t* dataBegin = componentBegin + ARQConstant::SectionHeaderSize;
	memcpy(dataBegin, dataUnit->data->data(), bytes);

	_currentSendingBuffer.dataLength += ARQConstant::SectionHeaderSize + bytes;
	
	_dataQueue.pop();
	delete dataUnit;
}

bool UDPIOBuffer::prepareDataSection(int sectionCount)
{
	if (_dataQueue.empty())
		return false;

	UDPDataUnit* dataUnit = _dataQueue.front();

	//-- 无符号数计算，+ remainedSpaceCorrectionSize 必须在减法运算之前，避免计算溢出。 
	size_t remainedSpaceCorrectionSize = (sectionCount == 0) ? ARQConstant::SectionHeaderSize : 0;
	if ((size_t)_MTU <= _currentSendingBuffer.dataLength + (size_t)ARQConstant::SectionHeaderSize - remainedSpaceCorrectionSize)
		return false;

	size_t remainedSpace = _MTU + remainedSpaceCorrectionSize - _currentSendingBuffer.dataLength - ARQConstant::SectionHeaderSize;

	bool segmented = dataUnit->data->length() > remainedSpace;
	if (segmented && sectionCount > 0)
	{
		size_t fullSinglePackageSpace = _MTU - ARQConstant::PackageHeaderSize;
		if (dataUnit->data->length() <= fullSinglePackageSpace)
			return false;				//-- 下个包整包发出，不做分段，简化收方处理。
	}

	if (!segmented)
		prepareSingleDataSection(remainedSpace);
	else
		return prepareFirstSegmentedDataSection(remainedSpace);

	return true;
}

bool ARQSelfSeqManager::checkResentSeq(uint32_t seq)
{
	if (unaAvailable)
	{
		uint32_t a = seq - una;
		uint32_t b = una - seq;

		if (a < b)
			return true;
	}
	else
		return true;

	return false;
}

bool UDPIOBuffer::prepareResentPackage_normalMode()
{
	std::set<uint32_t> conformedSeqs;
	int64_t threshold = slack_real_msec() - _resendControl.interval(slack_real_msec());

	for (auto& p: _unconformedMap)
	{
		if (_selfSeqManager.checkResentSeq(p.first))
		{
			if (p.second->lastSentMsec <= threshold)
			{
				_currentSendingBuffer.resendPackage(p.first, p.second);

				for (auto seq: conformedSeqs)
					_unconformedMap.erase(seq);

				return true;
			}
		}
		else
		{
			delete p.second;
			conformedSeqs.insert(p.first);
		}
	}

	for (auto seq: conformedSeqs)
		_unconformedMap.erase(seq);
	
	return false;
}

void ResendTracer::update(uint32_t currentUna, size_t limitedCount)
{
	if (count != 0)
	{
		count = (uint32_t)limitedCount;

		uint32_t diff;

		if (currentUna >= una)
			diff = currentUna - una;
		else
			diff = una - currentUna;

		una = currentUna;

		debug = diff;

		if (step <= diff)
		{
			lastSeq = una;
			step = 0;
		}
		else
		{
			step -= diff;

			if (step >= count)
				step = 0;

			lastSeq = una + step;
		}
	}
	else
	{
		count = (uint32_t)limitedCount;
		una = currentUna;
		lastSeq = currentUna;
		step = 0;

		debug = 0;
	}
}

void ResendTracer::reset()
{
	lastSeq = una;
	step = 0;
}

bool UDPIOBuffer::prepareResentPackage_limitedMode(size_t count)
{
	if (_selfSeqManager.unaAvailable)
	{
		if (_unconformedMap.size() < count)
			count = _unconformedMap.size();

		if (count == 0)
			return false;

		_resendTracer.update(_selfSeqManager.una, count);

		int64_t now = slack_real_msec();
		int64_t threshold = now - _resendControl.interval(slack_real_msec());

		size_t idx = 0;
		for (; idx < count && _resendTracer.step < count; idx++)
		{
			_resendTracer.step += 1;
			_resendTracer.lastSeq += 1;

			auto it = _unconformedMap.find(_resendTracer.lastSeq);
			if (it != _unconformedMap.end())
			{
				if (it->second->lastSentMsec <= threshold)
				{
					_currentSendingBuffer.resendPackage(it->first, it->second);

					return true;
				}
			}
		}

		_resendTracer.reset();

		for (; idx < count; idx++)
		{
			_resendTracer.step += 1;
			_resendTracer.lastSeq += 1;

			auto it = _unconformedMap.find(_resendTracer.lastSeq);
			if (it != _unconformedMap.end())
			{
				if (it->second->lastSentMsec <= threshold)
				{
					_currentSendingBuffer.resendPackage(it->first, it->second);
					return true;
				}
			}
		}

		return false;
	}
	else
	{
		return prepareResentPackage_normalMode();
	}
}

bool UDPIOBuffer::prepareSendingPackage(bool& blockByFlowControl)
{
	if (_activeCloseStatus == ActiveCloseStep::Required)
	{
		prepareClosePackage();
		_activeCloseStatus = ActiveCloseStep::GenPackage;
		return true;
	}

	if (!_sendingAdjustor.sendingCheck())		//-- TODO：加了流控后，也许可以去掉
	{
		blockByFlowControl = true;
		return false;
	}

	if (_currentSendingBuffer.dataLength > 0)
	{
		if (_currentSendingBuffer.requireUpdateSeq)
		{
			if (updateUDPSeq())
				return true;
			else
			{
				blockByFlowControl = true;
				return false;
			}

		}
		else
			return true;
	}

	bool blocked;
	if (prepareUrgentARQSyncPackage(blocked))
		return true;
	if (blocked)
	{
		blockByFlowControl = true;
		return false;
	}

	if (_resentCount > Config::UDP::_max_resent_count_per_call)		//-- TODO：加了流控后，也许可以去掉
	{
		blockByFlowControl = true;
		return false;
	}

	_congestionControl.updateUnconfirmedSize(slack_real_msec(), _unconformedMap.size());
	float loadIndex = _congestionControl.loadIndex();
	if (_unconformedMap.size() > 0)
	{
		if (loadIndex < UDPSimpleCongestionController::halfLoadThreshold)
		{
			if (prepareResentPackage_normalMode())
			{
				_resentCount += 1;
				return true;
			}
		}
		else if (loadIndex < UDPSimpleCongestionController::minLoadThreshold)
		{
			if (prepareResentPackage_halfMode())
			{
				_resentCount += 1;
				return true;
			}
		}
		else
		{
			if (prepareResentPackage_minMode())
			{
				_resentCount += 1;
				return true;
			}
		}
	}

	if (_unconformedMap.size() >= Config::UDP::_unconfiremed_package_limitation)		//-- TODO：加了流控后，也许可以去掉
	{
		blockByFlowControl = true;
		return false;
	}

	if (prepareCommonPackage())
		return true;
	else if (_currentSendingBuffer.dataLength > 0)
	{
		blockByFlowControl = true;
		return false;		//-- 发送被流控阻止。
	}

	if (_requireKeepAlive && slack_real_sec() - _lastSentSec >= Config::UDP::_heartbeat_interval_seconds)
	{
		prepareHeartbeatPackage();
		return true;
	}

	return false;
}

bool UDPIOBuffer::updateUDPSeq()
{
	uint32_t udpSeq = 0;

	if (_currentSendingBuffer.discardable)
		udpSeq = (uint32_t)slack_real_msec();
	else
		udpSeq = _UDPSeqBase++;

	uint32_t seqBE = htobe32(udpSeq);
	uint8_t factor = genChecksum(seqBE);

	_currentSendingBuffer.setFactor(factor);
	_currentSendingBuffer.setUDPSeq(seqBE);

	//if (_currentSendingBuffer.discardable && !notCare)
	//	_currentSendingBuffer.addFlag(ARQFlag::ARQ_Monitored);

	preparePackageCompleted(_currentSendingBuffer.discardable, udpSeq, seqBE, factor);
	return true;
}

void UDPIOBuffer::realSend(bool& needWaitSendEvent, bool& blockByFlowControl)
{
	blockByFlowControl = false;
	needWaitSendEvent = false;
	bool retry = false;

	_resentCount = 0;

	while (true)
	{
		if (!retry)
		{
			std::unique_lock<std::mutex> lck(*_mutex);
			if (_activeCloseStatus == ActiveCloseStep::GenPackage)
			{
				_activeCloseStatus = ActiveCloseStep::PackageSent;
				_requireClose = true;
				_sendToken = true;
				return;
			}

			if (_currentSendingBuffer.dataLength > 0)
			{
				if (_currentSendingBuffer.sendDone)
				{
					UDPPackage* package = _currentSendingBuffer.dumpPackage();
					if (package)
						_unconformedMap[_currentSendingBuffer.packageSeq] = package;

					_currentSendingBuffer.reset();
				}
			}

			if (!prepareSendingPackage(blockByFlowControl))
			{
				_sendingAdjustor.revoke();
				_sendToken = true;

				return;
			}

			retry = true;
		}

		ssize_t sendBytes = ::send(_socket, _currentSendingBuffer.dataBuffer, _currentSendingBuffer.dataLength, 0);
		if (sendBytes == -1)
		{
			if (errno == EAGAIN || errno == EWOULDBLOCK || errno == ENOBUFS)
			{
				needWaitSendEvent = true;
				std::unique_lock<std::mutex> lck(*_mutex);
				_sendToken = true;
				// _socketReadyForSending = false;

				return;
			}
			if (errno == EINTR)
				continue;

			if (errno == ECONNREFUSED)
			{
				std::unique_lock<std::mutex> lck(*_mutex);
				// _socketReadyForSending = false;
				_requireClose = true;
				_sendToken = true;
				return;
			}

			LOG_ERROR("Send UDP data on socket(%d) endpoint: %s, unprocessed error: %d", _socket, _endpoint.c_str(), errno);

			std::unique_lock<std::mutex> lck(*_mutex);
			//-- 暂时不考虑使用 _requireClose。因为：
			//--   1. Server 使用时，会忽略，而且必须忽略该属性。如果后续改动，不能因此导致 server 关闭端口。
			//--   2. ClientEngine 的周期发送和 Client 的按需发送，暂时没有处理该 case （毕竟概率超低），只有 IOWroker 里面有处理。
			//--      如果处理， UDPClient 相关的状态需要按照具体 errno 处理。
			// _requireClose = true;
			_sendToken = true;
			// _socketReadyForSending = false;

			return;
		}
		else if ((size_t)sendBytes != _currentSendingBuffer.dataLength)
		{
			_lastSentSec = slack_real_sec();
			_currentSendingBuffer.updateSendingInfo();

			LOG_ERROR("Send UDP data on socket(%d) endpoint: %s error. Want to send %d bytes, real sent %d bytes.",
				_socket, _endpoint.c_str(), (int)(_currentSendingBuffer.dataLength), (int)sendBytes);
		}
		else
		{
			retry = false;
			_lastSentSec = slack_real_sec();
			_currentSendingBuffer.sendDone = true;
			_currentSendingBuffer.updateSendingInfo();
		}

		//_socketReadyForSending = true;
	}
}

void UDPIOBuffer::sendCachedData(bool& needWaitSendEvent, bool& blockByFlowControl, bool socketReady)
{
	needWaitSendEvent = false;
	{
		std::unique_lock<std::mutex> lck(*_mutex);

		// if (socketReady)
		// {
		// 	_socketReadyForSending = true;
		// }

		if (!_sendToken /*|| !_socketReadyForSending*/)
		{
			blockByFlowControl = false;
			return;
		}

		_sendToken = false;
	}

	realSend(needWaitSendEvent, blockByFlowControl);
}

void UDPIOBuffer::sendData(bool& needWaitSendEvent, bool& blockByFlowControl, std::string* data, int64_t expiredMS, bool discardable)
{
	needWaitSendEvent = false;
	blockByFlowControl = false;
	UDPDataUnit* unit = new UDPDataUnit(data, discardable, expiredMS);

	{
		std::unique_lock<std::mutex> lck(*_mutex);
		_dataQueue.push(unit);

		//if (_socketReadyForSending == false)
		//	return;

		if (!_sendToken)
			return;

		_sendToken = false;
	}

	realSend(needWaitSendEvent, blockByFlowControl);
}

bool UDPIOBuffer::getRecvToken()
{
	std::unique_lock<std::mutex> lck(*_mutex);
	if (!_recvToken)
		return false;

	_recvToken = false;
	return true;
}

void UDPIOBuffer::cleaningFeedbackAcks(uint32_t una, std::unordered_set<uint32_t>& acks)
{
	std::unordered_set<uint32_t> remainedAcks;
	for (auto ack: acks)
	{
		uint32_t a = una - ack;
		uint32_t b = ack - una;
		if (a > b)
			remainedAcks.insert(ack);
	}
	remainedAcks.swap(acks);
}

void UDPIOBuffer::conformFeedbackSeqs()
{
	int64_t now = slack_real_msec();

	if (_parseResult.receivedUNA.size() > 0)
	{
		_selfSeqManager.una = _parseResult.receivedUNA[0];
		_selfSeqManager.unaAvailable = true;

		cleaningFeedbackAcks(_selfSeqManager.una, _parseResult.receivedAcks);
		cleanConformedPackageByUNA(now, _selfSeqManager.una);
	}

	if (_parseResult.receivedAcks.size())
	{
		cleanConformedPackageByAcks(now, _parseResult.receivedAcks);
	}
}

void UDPIOBuffer::cleanConformedPackageByUNA(int64_t now, uint32_t una)
{
	int count = 0;
	int64_t totalDelay = 0;

	std::unordered_map<uint32_t, UDPPackage*> remainedCache;
	for (auto& pp: _unconformedMap)
	{
		uint32_t a = una - pp.first;
		uint32_t b = pp.first - una;
		if (a <= b)
		{
			totalDelay += now - pp.second->firstSentMsec;
			count += 1;

			if (pp.second->resending == false)
				delete pp.second;
			else
				pp.second->requireDeleted = true;
		}
		else
			remainedCache[pp.first] = pp.second;
	}
	_unconformedMap.swap(remainedCache);

	_resendControl.updateDelay(now, totalDelay, count);
}

void UDPIOBuffer::cleanConformedPackageByAcks(int64_t now, std::unordered_set<uint32_t>& acks)
{
	int count = 0;
	int64_t totalDelay = 0;

	for (auto ack: acks)
	{
		auto it = _unconformedMap.find(ack);
		if (it != _unconformedMap.end())
		{
			totalDelay += now - it->second->firstSentMsec;
			count += 1;

			if (it->second->resending == false)
				delete it->second;
			else
				it->second->requireDeleted = true;

			_unconformedMap.erase(it);
		}
	}

	_resendControl.updateDelay(now, totalDelay, count);
}

void UDPIOBuffer::SyncARQStatus()
{
	//-- 同步状态数据
	if (_parseResult.canbeFeedbackUNA)
	{
		_seqManager.updateLastUNA(_arqParser.lastUDPSeq);
	}

	if (_parseResult.receivedPriorSeqs)
		_seqManager.repeatUNA = true;

	_seqManager.newReceivedSeqs(_arqParser.receivedSeqs);
	_seqManager.requireForceSync = _parseResult.requireForceSync;

	conformFeedbackSeqs();

	if (_arqParser.requireKeepLink)
		_requireKeepAlive = true;
}

void UDPIOBuffer::returnRecvToken()
{
	std::unique_lock<std::mutex> lck(*_mutex);
	_recvToken = true;

	SyncARQStatus();

	_parseResult.reset();
}

bool UDPIOBuffer::recvData()
{
	ssize_t readBytes = ::recv(_socket, _recvBuffer, _recvBufferLen, 0);
	if (readBytes > 0)
	{
		if (_arqParser.parse(_recvBuffer, (int)readBytes, &_parseResult))
		{
			if (_arqParser.requireClose)
			{
				_requireClose = true;
				return false;
			}
			_lastRecvSec = slack_real_sec();
		}
		else
		{
			if (_arqParser.requireClose)
			{
				_requireClose = true;
				return false;
			}
		}
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

bool UDPIOBuffer::parseReceivedData(uint8_t* buffer, int len, UDPIOReceivedResult& result)
{
	bool resultAvailable = true;
	if (_arqParser.parse(buffer, len, &_parseResult))
	{
		if (_arqParser.requireClose)
		{
			_requireClose = true;
			result.requireClose = true;
		}
		else
		{
			result.questList.swap(_parseResult.questList);
			result.answerList.swap(_parseResult.answerList);

			std::unique_lock<std::mutex> lck(*_mutex);
			SyncARQStatus();
		}

		_lastRecvSec = slack_real_sec();
	}
	else
	{
		resultAvailable = false;

		if (_arqParser.requireClose)
		{
			_requireClose = true;
			result.requireClose = true;
		}
	}

	_parseResult.reset();

	return resultAvailable;
}
