#include "UDPAssembler.v2.h"

using namespace fpnn;

void UDPPackage::expireCheck(int64_t now)
{
	if (expiredMsec >= now)
		return;

	expiredMsec = 0x7FFFFFFFFFFFFFFF;

	uint8_t* data = (uint8_t*)buffer;
	uint8_t type = *(data + 1);

	if (type == (uint8_t)ARQType::ARQ_COMBINED)
	{
		expireCombinedPackage();
	}
	else if (type == (uint8_t)ARQType::ARQ_DATA)
	{
		uint8_t flag = *(data + 2);
		uint8_t segmentFlag = flag & ARQFlag::ARQ_SegmentedMask;

		*(data + 2) = flag | ARQFlag::ARQ_CancelledPackage;
		len = ARQConstant::PackageMimimumLength;

		if (segmentFlag == 0x04)
			len += ARQConstant::SegmentPackageSeqSize + 1;
		else if (segmentFlag == 0x08)
			len += ARQConstant::SegmentPackageSeqSize + 2;
		else if (segmentFlag == 0x0C)
			len += ARQConstant::SegmentPackageSeqSize + 4;
	}
}

void UDPPackage::expireCombinedPackage()
{
	if (!encryptedBuffer)
		encryptedBuffer = malloc(len);

	uint8_t* orgPos = (uint8_t*)buffer;
	uint8_t* newPos = (uint8_t*)encryptedBuffer;

	memcpy(newPos, orgPos, ARQConstant::PackageHeaderSize);

	orgPos += ARQConstant::PackageHeaderSize;
	newPos += ARQConstant::PackageHeaderSize;

	size_t checkedLen = ARQConstant::PackageHeaderSize;
	size_t cutSize = 0;

	while (checkedLen < len)
	{
		uint8_t type = *orgPos;
		uint16_t bytes = be16toh(*((uint16_t*)(orgPos + 2)));

		int sectionSize = ARQConstant::SectionHeaderSize + bytes;

		if (type == (uint8_t)ARQType::ARQ_DATA)
		{
			uint8_t flag = *(orgPos + 1);
			uint8_t segmentFlag = flag & ARQFlag::ARQ_SegmentedMask;

			if (segmentFlag == 0)
			{
				cutSize += sectionSize;
			}
			else
			{
				flag |= ARQFlag::ARQ_CancelledPackage;

				int segmentHeadSize = ARQConstant::SegmentPackageSeqSize;
				if (segmentFlag == 0x04)
					segmentHeadSize += 1;
				else if (segmentFlag == 0x08)
					segmentHeadSize += 2;
				else
					segmentHeadSize += 4;

				*newPos = *orgPos;
				*(newPos + 1) = flag;

				uint16_t* bytesBuffer = (uint16_t*)(newPos + 2);
				*bytesBuffer = htobe16((uint16_t)segmentHeadSize);

				memcpy(newPos + ARQConstant::SectionHeaderSize, orgPos + ARQConstant::SectionHeaderSize, segmentHeadSize);
				cutSize += (sectionSize - ARQConstant::SectionHeaderSize - segmentHeadSize);

				newPos += ARQConstant::SectionHeaderSize + segmentHeadSize;
			}
		}
		else
		{
			memcpy(newPos, orgPos, sectionSize);
			newPos += sectionSize;
		}

		checkedLen += sectionSize;
		orgPos += sectionSize;
	}

	len -= cutSize;

	free(buffer);
	buffer = encryptedBuffer;
	encryptedBuffer = NULL;
}

CurrentSendingBuffer::CurrentSendingBuffer():
	rawBuffer(0), bufferLength(0), dataBuffer(0), dataLength(0), encryptor(NULL), resendingPackage(NULL)
{
	protocolVersion = ARQConstant::Version;
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
	dataBuffer = rawBuffer;
	dataLength = 0;

	discardable = true;
	sentCount = 0;

	dataExpiredMsec = 0;
	newPackage = false;
	sendDone = false;
	//requireUpdateSeq = false;
	if (resendingPackage)
	{
		resendingPackage->resending = false;
		resendingPackage = NULL;
	}

	dataBuffer[0] = protocolVersion;
	dataBuffer[1] = 0;
	dataBuffer[2] = 0;

	for (auto package: assembledPackages)
	{
		if (package->requireDeleted)
			delete package;
		else
			package->resending = false;
	}

	assembledPackages.clear();
}

void CurrentSendingBuffer::updateSendingInfo()
{
	if (assembledPackages.size() > 0)
	{
		for (auto package: assembledPackages)
			package->updateSendingInfo();
	}
	else if (resendingPackage)
		resendingPackage->updateSendingInfo();
	else
	{
		sentCount += 1;
		lastSentMsec = slack_real_msec();
	}
}

UDPPackage* CurrentSendingBuffer::dumpPackage()
{
	if (!newPackage)
		return NULL;

	//if (discardable)
	//	return NULL;

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

	lastSentMsec = slack_real_msec();
	package->firstSentMsec = lastSentMsec;
	package->lastSentMsec = lastSentMsec;

	if (dataExpiredMsec)
		package->expiredMsec = dataExpiredMsec;

	memcpy(package->buffer, dataBuffer, dataLength);
	
	return package;
}

void CurrentSendingBuffer::resendPackage(uint32_t seq, UDPPackage* package)
{
	resendingPackage = package;
	
	if (package->encryptedBuffer == NULL)
	{
		if (encryptor == NULL)
			dataBuffer = (uint8_t*)(package->buffer);
		else
		{
			package->encryptedBuffer = malloc(package->len);
			encryptor->packageEncrypt(package->encryptedBuffer, package->buffer, package->len);

			dataBuffer = (uint8_t*)(package->encryptedBuffer);
		}
	}
	else
		dataBuffer = (uint8_t*)(package->encryptedBuffer);

	dataLength = package->len;

	discardable = false;
	packageSeq = seq;
	
	sendDone = false;
	//requireUpdateSeq = false;

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
void CurrentSendingBuffer::setSign(uint8_t sign)
{
	*(dataBuffer + 3) = sign;
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

void CurrentSendingBuffer::changeForPackageAssembling()
{
	dataBuffer[0] = protocolVersion;
	dataBuffer[1] = (uint8_t)ARQType::ARQ_ASSEMBLED;
	dataLength = 2;
}

void CurrentSendingBuffer::assemblePackage(UDPPackage* package)
{
	uint8_t* begin = dataBuffer + dataLength;
	*(uint16_t*)begin = htobe16((uint16_t)(package->len - 1));

	begin += 2;
	dataLength += 2 + (package->len - 1);
	memcpy(begin, (uint8_t*)(package->buffer) + 1, package->len - 1);

	assembledPackages.push_back(package);
	package->resending = true;
}

void CurrentSendingBuffer::updateDataExpiredMsec(int64_t expireMsec)
{
	if (dataExpiredMsec < expireMsec)
		dataExpiredMsec = expireMsec;
}

//=====================================================================//
//--                           UDP Assembler                         --//
//=====================================================================//

UDPAssembler::UDPAssembler(): _arqChecksum(NULL), _dataEncryptor(NULL)
{
	_UDPSeqBase = (uint32_t)slack_mono_msec();
	_packageIdNumber = (uint16_t)slack_mono_msec();
	_protocolVersion = ARQConstant::Version;
}

UDPAssembler::~UDPAssembler()
{
	if (_arqChecksum)
		delete _arqChecksum;

	while (_dataQueue.size() > 0)
	{
		delete _dataQueue.front();
		_dataQueue.pop_front();
	}
}

void UDPAssembler::init(int MTU)
{
	_MTU = MTU;
	_currentSendingBuffer.init(MTU + ARQConstant::SectionHeaderSize);
}

uint8_t UDPAssembler::genChecksum(uint32_t udpSeqBE)
{
	if (_arqChecksum)
		return _arqChecksum->genChecksum(udpSeqBE);

	return (uint8_t)slack_real_msec();
}

void UDPAssembler::configProtocolVersion(uint8_t version)
{
	_protocolVersion = version;
	_currentSendingBuffer.configProtocolVersion(version);
}

//--------------------------------------------------//
//--           Cannot be combined data             -//
//--------------------------------------------------//
void UDPAssembler::prepareClosePackage()
{
	_currentSendingBuffer.dataLength = ARQConstant::PackageHeaderSize;

	uint32_t randomSeq = (uint32_t)slack_real_msec();
	uint32_t seqBE = htobe32(randomSeq);

	_currentSendingBuffer.setType(ARQType::ARQ_CLOSE);
	_currentSendingBuffer.setFlag(ARQFlag::ARQ_Discardable);
	_currentSendingBuffer.setSign(genChecksum(seqBE));

	_currentSendingBuffer.setUDPSeq(seqBE);

	preparePackageCompleted(true, randomSeq, seqBE, 0);
}

void UDPAssembler::prepareHeartbeatPackage()
{
	uint32_t udpSeq = (uint32_t)slack_real_msec();
	
	uint32_t seqBE = htobe32(udpSeq);
	uint8_t sign = genChecksum(seqBE);

	_currentSendingBuffer.setFlag(ARQFlag::ARQ_Discardable);
	_currentSendingBuffer.setType(ARQType::ARQ_HEARTBEAT);
	_currentSendingBuffer.setSign(sign);

	_currentSendingBuffer.setUDPSeq(seqBE);

	uint64_t* timestamp = (uint64_t*)(_currentSendingBuffer.dataBuffer + ARQConstant::PackageHeaderSize);
	*timestamp = htobe64(slack_real_msec());

	_currentSendingBuffer.dataLength = ARQConstant::PackageHeaderSize + sizeof(uint64_t);

	preparePackageCompleted(true, udpSeq, seqBE, sign);
}

void UDPAssembler::preparePackageCompleted(bool discardable, uint32_t udpSeq, uint32_t udpSeqBE, uint8_t sign)
{
	_currentSendingBuffer.discardable = discardable;
	_currentSendingBuffer.packageSeq = udpSeq;
	//_currentSendingBuffer.requireUpdateSeq = false;

	if (!discardable && _arqChecksum == NULL)
	{
		_arqChecksum = new ARQChecksum(udpSeqBE, sign);
		_currentSendingBuffer.addFlag(ARQFlag::ARQ_FirstPackage);
	}

	if (discardable)
		_currentSendingBuffer.addFlag(ARQFlag::ARQ_Discardable);

	_currentSendingBuffer.newPackage = true;
}

//--------------------------------------------------//
//--               Combinable data                 -//
//--------------------------------------------------//
void UDPAssembler::prepareForceSyncSection()
{
	uint8_t* componentBegin = _currentSendingBuffer.dataBuffer + _currentSendingBuffer.dataLength;

	_currentSendingBuffer.setComponentType(componentBegin, ARQType::ARQ_FORCESYNC);
	_currentSendingBuffer.setComponentFlag(componentBegin, ARQFlag::ARQ_Discardable);
	_currentSendingBuffer.setComponentBytes(componentBegin, 0);

	_currentSendingBuffer.dataLength += (size_t)ARQConstant::SectionHeaderSize;
}

void UDPAssembler::prepareUNASection()
{
	uint8_t* componentBegin = _currentSendingBuffer.dataBuffer + _currentSendingBuffer.dataLength;

	_currentSendingBuffer.setComponentType(componentBegin, ARQType::ARQ_UNA);
	_currentSendingBuffer.setComponentFlag(componentBegin, ARQFlag::ARQ_Discardable);
	_currentSendingBuffer.setComponentBytes(componentBegin, sizeof(uint32_t));
	
	uint32_t* unaBuffer = (uint32_t*)(componentBegin + ARQConstant::SectionHeaderSize);
	*unaBuffer = htobe32(_seqManager->lastUNA);

	_seqManager->unaUpdated = false;
	_seqManager->repeatUNA = false;
	_currentSendingBuffer.dataLength += (size_t)ARQConstant::SectionHeaderSize + sizeof(uint32_t);
}

void UDPAssembler::prepareAcksSection()
{
	uint8_t* componentBegin = _currentSendingBuffer.dataBuffer + _currentSendingBuffer.dataLength;

	_currentSendingBuffer.setComponentType(componentBegin, ARQType::ARQ_ACKS);
	_currentSendingBuffer.setComponentFlag(componentBegin, ARQFlag::ARQ_Discardable);

	size_t remainedSize = _MTU - _currentSendingBuffer.dataLength - ARQConstant::SectionHeaderSize;

	size_t acksCount = remainedSize/sizeof(uint32_t);
	if (acksCount > _seqManager->receivedSeqs.size())
		acksCount = _seqManager->receivedSeqs.size();

	size_t bytes = acksCount * sizeof(uint32_t);
	_currentSendingBuffer.setComponentBytes(componentBegin, bytes);

	uint8_t* acksBuffer = componentBegin + ARQConstant::SectionHeaderSize;

	int64_t now = slack_real_msec();
	for (size_t i = 0; i < acksCount; i++)
	{
		auto it = _seqManager->receivedSeqs.begin();
		*((uint32_t*)acksBuffer) = htobe32(*it);
		acksBuffer += sizeof(uint32_t);

		_seqManager->feedbackedSeqs[*it] = now;
		_seqManager->receivedSeqs.erase(it);
	}

	_currentSendingBuffer.dataLength += (size_t)ARQConstant::SectionHeaderSize + bytes;
}

bool UDPAssembler::prepareDataSection(int sectionCount)
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

void UDPAssembler::prepareSingleDataSection(size_t availableSpace)
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
	_currentSendingBuffer.updateDataExpiredMsec(dataUnit->expiredMS);

	uint8_t* dataBegin = componentBegin + ARQConstant::SectionHeaderSize;

	if (!_dataEncryptor || dataUnit->discardable)
		memcpy(dataBegin, dataUnit->data->data(), bytes);
	else
		_dataEncryptor->dataEncrypt(dataBegin, (void*)(dataUnit->data->data()), bytes);

	_currentSendingBuffer.dataLength += ARQConstant::SectionHeaderSize + bytes;
	
	_dataQueue.pop_front();
	delete dataUnit;
}

bool UDPAssembler::prepareFirstSegmentedDataSection(size_t availableSpace)
{
	const size_t indexSize = 1;
	if (availableSpace < ARQConstant::SegmentPackageSeqSize + indexSize)
		return false;

	uint8_t* componentBegin = _currentSendingBuffer.dataBuffer + _currentSendingBuffer.dataLength;
	_currentSendingBuffer.setComponentType(componentBegin, ARQType::ARQ_DATA);

	UDPDataUnit* dataUnit = _dataQueue.front();
	uint8_t flag = ARQFlag::ARQ_1Byte_SegmentIndex;

	if (dataUnit->discardable)
		flag |= ARQFlag::ARQ_Discardable;
	else
		_currentSendingBuffer.discardable = false;
	
	_currentSendingBuffer.setComponentFlag(componentBegin, flag);
	_currentSendingBuffer.updateDataExpiredMsec(dataUnit->expiredMS);

	size_t bytes = availableSpace - ARQConstant::SegmentPackageSeqSize - indexSize;

	_currentSendingBuffer.setComponentBytes(componentBegin, bytes + ARQConstant::SegmentPackageSeqSize + indexSize);

	_packageIdNumber += 1;
	//---------- assemble -----------//

	uint8_t* segmentBegin = componentBegin + ARQConstant::SectionHeaderSize;
	_currentSendingBuffer.setDataComponentPackageSeq(segmentBegin, _packageIdNumber);
	_currentSendingBuffer.setDataComponentSegmentIndex(segmentBegin, 1);

	uint8_t* dataBegin = segmentBegin + ARQConstant::SegmentPackageSeqSize + indexSize;

	if (!_dataEncryptor || dataUnit->discardable)
		memcpy(dataBegin, dataUnit->data->data(), bytes);
	else
		_dataEncryptor->dataEncrypt(dataBegin, (void*)(dataUnit->data->data()), bytes);

	_currentSendingBuffer.dataLength += ARQConstant::SectionHeaderSize + ARQConstant::SegmentPackageSeqSize + indexSize + bytes;

	//-- Update _sendingSegmentInfo
	_dataQueue.pop_front();
	_sendingSegmentInfo.data = dataUnit;
	_sendingSegmentInfo.nextIndex = 2;
	_sendingSegmentInfo.offset = bytes;

	return true;
}

bool UDPAssembler::prepareSegmentedDataSection(int sectionCount)
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

	if (_sendingSegmentInfo.data->expiredMS <= slack_real_msec() && _protocolVersion > 1)
	{
		prepareSegmentedDataExpiredSection(flag, idxSize);
		return true;
	}

	//-- 无符号数计算，+ remainedSpaceCorrectionSize 必须在减法运算之前，避免计算溢出。 
	size_t remainedSpace = (size_t)_MTU + remainedSpaceCorrectionSize - _currentSendingBuffer.dataLength - segmentInfoSize;
	size_t remainedDataSize = _sendingSegmentInfo.data->data->length() - _sendingSegmentInfo.offset;

	size_t bytes = remainedDataSize;
	if (remainedDataSize > remainedSpace)
		bytes = remainedSpace;
	else
		flag |= ARQFlag::ARQ_LastSegmentData;

	//---------- assemble -----------//

	uint8_t* componentBegin = _currentSendingBuffer.dataBuffer + _currentSendingBuffer.dataLength;
	_currentSendingBuffer.setComponentType(componentBegin, ARQType::ARQ_DATA);
	_currentSendingBuffer.updateDataExpiredMsec(_sendingSegmentInfo.data->expiredMS);

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
	
	if (!_dataEncryptor || _sendingSegmentInfo.data->discardable)
		memcpy(dataBegin, _sendingSegmentInfo.data->data->data() + _sendingSegmentInfo.offset, bytes);
	else
		_dataEncryptor->dataEncrypt(dataBegin, (void*)(_sendingSegmentInfo.data->data->data() + _sendingSegmentInfo.offset), bytes);

	_sendingSegmentInfo.offset += bytes;

	if (_sendingSegmentInfo.offset >= _sendingSegmentInfo.data->data->length())
		_sendingSegmentInfo.reset();

	_currentSendingBuffer.dataLength += segmentInfoSize + bytes;
	return true;
}

void UDPAssembler::prepareSegmentedDataExpiredSection(uint8_t flag, size_t idxSize)
{
	flag |= ARQFlag::ARQ_CancelledPackage;

	uint8_t* componentBegin = _currentSendingBuffer.dataBuffer + _currentSendingBuffer.dataLength;
	_currentSendingBuffer.setComponentType(componentBegin, ARQType::ARQ_DATA);

	if (_sendingSegmentInfo.data->discardable == false)
		_currentSendingBuffer.discardable = false;

	if (_currentSendingBuffer.discardable)
		flag |= ARQFlag::ARQ_Discardable;

	_currentSendingBuffer.setComponentFlag(componentBegin, flag);
	_currentSendingBuffer.setComponentBytes(componentBegin, ARQConstant::SegmentPackageSeqSize + idxSize);

	uint8_t* segmentBegin = componentBegin + ARQConstant::SectionHeaderSize;
	_currentSendingBuffer.setDataComponentPackageSeq(segmentBegin, _packageIdNumber);
	_currentSendingBuffer.setDataComponentSegmentIndex(segmentBegin, _sendingSegmentInfo.nextIndex);

	_sendingSegmentInfo.reset();

	_currentSendingBuffer.dataLength += ARQConstant::SectionHeaderSize + ARQConstant::SegmentPackageSeqSize + idxSize;
}

bool UDPAssembler::prepareCommonPackage()
{
	int sectionCount = 0;

	_currentSendingBuffer.dataLength = ARQConstant::PackageHeaderSize;
	//_currentSendingBuffer.requireUpdateSeq = true;

	_currentSendingBuffer.setType(ARQType::ARQ_COMBINED);

	if (_seqManager->needSyncSeqStatus())
	{
		if (_seqManager->needSyncUNA())
		{
			prepareUNASection();
			sectionCount += 1;
		}

		if (_seqManager->needSyncAcks())
		{
			prepareAcksSection();
			sectionCount += 1;
		}

		_seqManager->lastSyncMsec = slack_real_msec();
	}

	fillDataSection(sectionCount);

	return completeCommonPackage(sectionCount);
}

bool UDPAssembler::completeCommonPackage(int sectionCount)
{
	if (sectionCount == 0)
	{
		_currentSendingBuffer.reset();
		return false;
	}

	if (sectionCount == 1)
		_currentSendingBuffer.changeCombinedPackageToSinglepackage();

	updateUDPSeq();
	return true;
}

void UDPAssembler::updateUDPSeq()
{
	uint32_t udpSeq = 0;

	if (_currentSendingBuffer.discardable)
		udpSeq = (uint32_t)slack_real_msec();
	else
		udpSeq = _UDPSeqBase++;

	uint32_t seqBE = htobe32(udpSeq);
	uint8_t sign = genChecksum(seqBE);

	_currentSendingBuffer.setSign(sign);
	_currentSendingBuffer.setUDPSeq(seqBE);

	//if (_currentSendingBuffer.discardable && !notCare)
	//	_currentSendingBuffer.addFlag(ARQFlag::ARQ_Monitored);

	preparePackageCompleted(_currentSendingBuffer.discardable, udpSeq, seqBE, sign);
}

void UDPAssembler::fillDataSection(int& sectionCount)
{
	if (_sendingSegmentInfo.data)
	{
		if (prepareSegmentedDataSection(sectionCount))
			sectionCount += 1;
		else
			return;
	}

	while (_dataQueue.size() > 0)
	{
		if (_currentSendingBuffer.dataLength >= (size_t)_MTU)
			return;

		while (_dataQueue.front()->expiredMS < slack_real_msec())
		{
			delete _dataQueue.front();
			_dataQueue.pop_front();

			if (_dataQueue.empty())
				return;
		}

		if (prepareDataSection(sectionCount))
			sectionCount += 1;
		else
			return;
	}
}

bool UDPAssembler::prepareUrgentARQSyncPackage(bool includeForceSyncSection, bool feedbackForceSync, bool canFillDataSections)
{
	int sectionCount = 0;

	_currentSendingBuffer.dataLength = ARQConstant::PackageHeaderSize;
	//_currentSendingBuffer.requireUpdateSeq = true;

	_currentSendingBuffer.setType(ARQType::ARQ_COMBINED);

	if (includeForceSyncSection)
	{
		prepareForceSyncSection();
		sectionCount += 1;
	}

	if ((feedbackForceSync && _seqManager->unaAvailable) || _seqManager->needSyncUNA())
	{
		prepareUNASection();
		sectionCount += 1;
	}

	_seqManager->cleanReceivedSeqs();
	if (_seqManager->needSyncAcks())
	{
		prepareAcksSection();
		sectionCount += 1;
	}

	_seqManager->lastSyncMsec = slack_real_msec();

	//-- 充分利用 MTU 剩余空间
	if (canFillDataSections)
		fillDataSection(sectionCount);

	return completeCommonPackage(sectionCount);
}

void UDPAssembler::prepareECDHPackageAsInitiator(bool reinforcePackage, const std::string* packagePubKey, bool reinforceData, const std::string* dataPubKey)
{
	_currentSendingBuffer.dataLength = ARQConstant::PackageHeaderSize;
	_currentSendingBuffer.setType(ARQType::ARQ_ECDH);
	_currentSendingBuffer.discardable = false;

	uint8_t params = (uint8_t)(packagePubKey->length());
	if (reinforcePackage)
		params |= 0x80;

	uint8_t* buff = _currentSendingBuffer.dataBuffer + _currentSendingBuffer.dataLength;
	*buff = params;
	memcpy(buff + 1, packagePubKey->data(), packagePubKey->length());
	_currentSendingBuffer.dataLength += 1 + packagePubKey->length();

	if (dataPubKey)
	{
		params = (uint8_t)(dataPubKey->length());
		if (reinforceData)
			params |= 0x80;

		buff = _currentSendingBuffer.dataBuffer + _currentSendingBuffer.dataLength;
		*buff = params;
		memcpy(buff + 1, dataPubKey->data(), dataPubKey->length());
		_currentSendingBuffer.dataLength += 1 + dataPubKey->length();
	}

	updateUDPSeq();
}

void UDPAssembler::pushDataToSendingQueue(std::string* data, int64_t expiredMS, bool discardable)
{
	UDPDataUnit* unit = new UDPDataUnit(data, discardable, expiredMS);
	_dataQueue.emplace_back(unit);
}
