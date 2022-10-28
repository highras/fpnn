#include "Setting.h"
#include "../KeyExchange.h"
#include "UDPIOBuffer.v2.h"

using namespace fpnn;

//=====================================================================//
//--                        UDP IO Buffer                            --//
//=====================================================================//

UDPIOBuffer::UDPIOBuffer(std::mutex* mutex, int socket, int MTU):
	_socket(socket), _MTU(MTU), _requireKeepAlive(false), _requireClose(false),
	_lastSentSec(0), _lastRecvSec(0), _activeCloseStatus(ActiveCloseStep::None),
	_packageAssembler(), _sendingEncryptor(NULL), _encryptBuffer(NULL), _ecdhPackageReference(NULL),
	_sendToken(true), _recvToken(true), _mutex(mutex),  _lastUrgentMsec(0)
{
	//-- Adjust availd MTU.
	_MTU -= 20;		//-- IP header size
	_MTU -= 8;		//-- UDP header size

	_protocolVersion = ARQConstant::Version;
	_untransmittedSeconds = Config::UDP::_max_untransmitted_seconds;
	_lastRecvSec = slack_real_sec();

	_recvBufferLen = FPNN_UDP_MAX_DATA_LENGTH;
	if (_recvBufferLen > Config::_max_recv_package_length)
		_recvBufferLen = Config::_max_recv_package_length;

	_recvBuffer = (uint8_t*)malloc(_recvBufferLen);

	_arqParser.changeLogInfo(socket, NULL);
	_arqParser.setDecryptedBufferLen(_recvBufferLen);

	_packageAssembler.init(_MTU);
	_packageAssembler.configARQPeerSeqManager(&_seqManager);
	_currentSendingBuffer = _packageAssembler.getSendingBuffer();
}

UDPIOBuffer::~UDPIOBuffer()
{
	free(_recvBuffer);

	if (_sendingEncryptor)
		delete _sendingEncryptor;

	if (_encryptBuffer)
		free(_encryptBuffer);
}

void UDPIOBuffer::configProtocolVersion(uint8_t version)
{
	_protocolVersion = version;
	_packageAssembler.configProtocolVersion(version);
}

void UDPIOBuffer::enableKeepAlive()
{
	//std::unique_lock<std::mutex> lck(*_mutex);
	_requireKeepAlive = true;
}

bool UDPIOBuffer::isTransmissionStopped()
{
	if (_lastRecvSec == 0 || _untransmittedSeconds < 0)
		return false;
	
	return slack_real_sec() - _untransmittedSeconds > _lastRecvSec;
}

void UDPIOBuffer::setUntransmittedSeconds(int untransmittedSeconds)
{
	_untransmittedSeconds = untransmittedSeconds;
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

void UDPIOBuffer::updateEndpointInfo(const std::string& endpoint)
{
	_endpoint = endpoint;
	_arqParser.changeLogInfo(_socket, _endpoint.c_str());
}

void UDPIOBuffer::configSendingEncryptor(UDPEncryptor* encryptor, bool enableDataEncryption)
{
	_sendingEncryptor = encryptor;
	_currentSendingBuffer->encryptor = encryptor;

	if (enableDataEncryption)
	{
		_packageAssembler.configDataEncryptor(encryptor);
		_unconformedMap.disableExpireCheck();
	}

	if (!_encryptBuffer)
		_encryptBuffer = (uint8_t*)malloc(_MTU);
}

bool UDPIOBuffer::enableEncryptorAsInitiator(const std::string& curve, const std::string& peerPublicKey, bool reinforce)
{
	//-- 01: prepare params

	ECCKeysMaker keysMaker;
	keysMaker.setPeerPublicKey(peerPublicKey);
	if (keysMaker.setCurve(curve) == false)
	{
		LOG_ERROR("Unsupport ecc curve %d.", curve.c_str());
		return false;
	}

	std::string selfPublicKey = keysMaker.publicKey(true);
	if (selfPublicKey.empty())
	{
		LOG_ERROR("Gen package public key with ecc curve %d failed.", curve.c_str());
		return false;
	}

	uint8_t key[32];
	uint8_t iv[16];
	int aesKeyLen = reinforce ? 32 : 16;
	if (keysMaker.calcKey(key, iv, aesKeyLen) == false)
	{
		LOG_ERROR("Generate AES key for package encryptor failed.");
		return false;
	}

	//-- 02: Init UDPEncryptors

	if (!_encryptBuffer)
		_encryptBuffer = (uint8_t*)malloc(_MTU);

	if (_sendingEncryptor == NULL)
		_sendingEncryptor = new UDPEncryptor();

	_sendingEncryptor->configPackageEncryptor(key, aesKeyLen, iv);
	_currentSendingBuffer->encryptor = _sendingEncryptor;

	_arqParser.configPackageEncryptor(key, aesKeyLen, iv);

	//-- 03: Prepare ECDH package

	_packageAssembler.prepareECDHPackageAsInitiator(reinforce, &selfPublicKey, reinforce, NULL);
	UDPPackage* package = _currentSendingBuffer->dumpPackage();

	//-- Prevent ECDH package be encrypted when resending.
	package->encryptedBuffer = malloc(package->len);
	memcpy(package->encryptedBuffer, package->buffer, package->len);

	uint32_t seqNum = _currentSendingBuffer->packageSeq;
	_unconformedMap.insert(seqNum, package);

	_ecdhPackageReference = package;
	_ecdhSeq =seqNum;

	return true;
}

bool UDPIOBuffer::enableEncryptorAsInitiator(const std::string& curve, const std::string& peerPublicKey,
	bool reinforcePackage, bool reinforceData)
{
	//-- 01: prepare params

	ECCKeysMaker keysMaker;
	keysMaker.setPeerPublicKey(peerPublicKey);
	if (keysMaker.setCurve(curve) == false)
	{
		LOG_ERROR("Unsupport ecc curve %d.", curve.c_str());
		return false;
	}

	std::string selfPackagePublicKey = keysMaker.publicKey(true);
	if (selfPackagePublicKey.empty())
	{
		LOG_ERROR("Gen package public key with ecc curve %d failed.", curve.c_str());
		return false;
	}

	uint8_t packageKey[32];
	uint8_t packageIV[16];
	int packageKeyLen = reinforcePackage ? 32 : 16;

	if (keysMaker.calcKey(packageKey, packageIV, packageKeyLen) == false)
	{
		LOG_ERROR("Generate AES key for package encryptor failed.");
		return false;
	}


	std::string selfDataPublicKey = keysMaker.publicKey(true);
	if (selfDataPublicKey.empty())
	{
		LOG_ERROR("Gen data public key with ecc curve %d failed.", curve.c_str());
		return false;
	}

	uint8_t dataKey[32];
	uint8_t dataIV[16];
	int dataKeyLen = reinforceData ? 32 : 16;
	if (keysMaker.calcKey(dataKey, dataIV, dataKeyLen) == false)
	{
		LOG_ERROR("Generate AES key for data encryptor failed.");
		return false;
	}

	//-- 02: Init UDPEncryptors

	if (!_encryptBuffer)
		_encryptBuffer = (uint8_t*)malloc(_MTU);

	if (_sendingEncryptor == NULL)
		_sendingEncryptor = new UDPEncryptor();

	_sendingEncryptor->configPackageEncryptor(packageKey, packageKeyLen, packageIV);
	_sendingEncryptor->configDataEncryptor(dataKey, dataKeyLen, dataIV);
	_currentSendingBuffer->encryptor = _sendingEncryptor;

	_arqParser.configPackageEncryptor(packageKey, packageKeyLen, packageIV);
	_arqParser.configDataEncryptor(dataKey, dataKeyLen, dataIV);

	//-- 02-1: Config UDPAssembler for Data enhance encrypting.
	_packageAssembler.configDataEncryptor(_sendingEncryptor);
	_unconformedMap.disableExpireCheck();

	//-- 03: Prepare ECDH package

	_packageAssembler.prepareECDHPackageAsInitiator(reinforcePackage, &selfPackagePublicKey,
		reinforceData, &selfDataPublicKey);
	
	UDPPackage* package = _currentSendingBuffer->dumpPackage();

	//-- Prevent ECDH package be encrypted when resending.
	package->encryptedBuffer = malloc(package->len);
	memcpy(package->encryptedBuffer, package->buffer, package->len);

	uint32_t seqNum = _currentSendingBuffer->packageSeq;
	_unconformedMap.insert(seqNum, package);

	_ecdhPackageReference = package;
	_ecdhSeq =seqNum;

	return true;
}

//--------------------------------------------//
//--    UDP IO Buffer: Sending Functions    --//
//--------------------------------------------//
void UDPIOBuffer::sendCachedData(bool& needWaitSendEvent, bool& blockByFlowControl, bool socketReady)
{
	needWaitSendEvent = false;
	blockByFlowControl = false;

	{
		std::unique_lock<std::mutex> lck(*_mutex);

		if (!_sendToken)
			return;

		_sendToken = false;
	}

	realSend(needWaitSendEvent, blockByFlowControl);
}

void UDPIOBuffer::sendData(bool& needWaitSendEvent, bool& blockByFlowControl, std::string* data, int64_t expiredMS, bool discardable)
{
	needWaitSendEvent = false;
	blockByFlowControl = false;

	{
		std::unique_lock<std::mutex> lck(*_mutex);

		_packageAssembler.pushDataToSendingQueue(data, expiredMS, discardable);

		if (!_sendToken)
			return;

		_sendToken = false;
	}

	realSend(needWaitSendEvent, blockByFlowControl);
}

void UDPIOBuffer::updateResendTolerance()
{
	int64_t interval = _resendControl.interval(slack_real_msec());
	_resendThreshold = slack_real_msec() - interval;
}

void UDPIOBuffer::realSend(bool& needWaitSendEvent, bool& blockByFlowControl)
{
	blockByFlowControl = false;
	needWaitSendEvent = false;
	bool retry = false;

	if ((int)_unconformedMap.size() < Config::UDP::_max_resent_count_per_call)
		_resentCount = (int)_unconformedMap.size();
	else
		_resentCount = Config::UDP::_max_resent_count_per_call;

	updateResendTolerance();

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

			if (!prepareSendingPackage(blockByFlowControl))
			{
				_sendingAdjustor.revoke();
				_sendToken = true;

				return;
			}

			retry = true;
		}

		ssize_t sendBytes;
		if (_sendingEncryptor == NULL || _currentSendingBuffer->resendingPackage)
		{
			sendBytes = ::send(_socket, _currentSendingBuffer->dataBuffer, _currentSendingBuffer->dataLength, 0);
		}
		else
		{
			_sendingEncryptor->packageEncrypt(_encryptBuffer, _currentSendingBuffer->dataBuffer, _currentSendingBuffer->dataLength);
			sendBytes = ::send(_socket, _encryptBuffer, _currentSendingBuffer->dataLength, 0);
		}

		if ((size_t)sendBytes == _currentSendingBuffer->dataLength)
		{
			retry = false;
			_lastSentSec = slack_real_sec();
			_currentSendingBuffer->sendDone = true;
			_currentSendingBuffer->updateSendingInfo();
		}
		else if (sendBytes == -1)
		{
			if (errno == EAGAIN || errno == EWOULDBLOCK || errno == ENOBUFS)
			{
				needWaitSendEvent = true;
				std::unique_lock<std::mutex> lck(*_mutex);
				_sendToken = true;
				return;
			}
			if (errno == EINTR)
				continue;

			if (errno == ECONNREFUSED)
			{
				std::unique_lock<std::mutex> lck(*_mutex);
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
			return;
		}
		else
		{
			_lastSentSec = slack_real_sec();
			_currentSendingBuffer->updateSendingInfo();

			LOG_ERROR("Send UDP data on socket(%d) endpoint: %s error. Want to send %d bytes, real sent %d bytes.",
				_socket, _endpoint.c_str(), (int)(_currentSendingBuffer->dataLength), (int)sendBytes);
		}
	}
}

void UDPIOBuffer::paddingResendPackages()
{
	uint32_t seqNum = _currentSendingBuffer->packageSeq;
	bool discardable = _currentSendingBuffer->discardable;
	UDPPackage* package = _currentSendingBuffer->dumpPackage();

	if (!discardable)
		_unconformedMap.insert(seqNum, package);

	if (_protocolVersion > 1 && _unconformedMap.prepareSendingBuffer(_MTU, _resendThreshold, package, _currentSendingBuffer))
	{
		_resentCount -= ((int)(_currentSendingBuffer->assembledPackages.size()) - 1);

		if (discardable)
			package->requireDeleted = true;
	}
	else
	{
		if (discardable)
			delete package;
	}
}

bool UDPIOBuffer::prepareSendingPackage(bool& blockByFlowControl)
{
	if (_activeCloseStatus == ActiveCloseStep::Required)
	{
		_currentSendingBuffer->reset();
		_packageAssembler.prepareClosePackage();
		_activeCloseStatus = ActiveCloseStep::GenPackage;
		return true;
	}

	if (!_sendingAdjustor.sendingCheck())		//-- TODO：加了流控后，也许可以去掉
	{
		blockByFlowControl = true;
		return false;
	}

	if (_ecdhPackageReference == NULL) ;
	else
		return checkECDHResending();

	if (_currentSendingBuffer->dataLength > 0)
	{
		if (_currentSendingBuffer->sendDone)
			_currentSendingBuffer->reset();
		else
			return true;
	}

	if (prepareUrgentARQSyncPackage())
	{
		paddingResendPackages();
		return true;
	}
	
	if (_resentCount > 0)
	{
		if (_unconformedMap.size() > 0)
		{
			if (prepareResentPackage_normalMode())
				return true;
		}
	}

	if (_unconformedMap.size() >= Config::UDP::_unconfiremed_package_limitation)		//-- TODO：加了流控后，也许可以去掉
	{
		blockByFlowControl = true;
		return false;
	}

	if (_packageAssembler.prepareCommonPackage())
	{
		paddingResendPackages();
		return true;
	}

	if (_requireKeepAlive && slack_real_sec() - _lastSentSec >= Config::UDP::_heartbeat_interval_seconds)
	{
		_packageAssembler.prepareHeartbeatPackage();
		return true;
	}

	return false;
}

bool UDPIOBuffer::prepareUrgentARQSyncPackage()
{
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

	bool fillDataSections = _unconformedMap.size() <= Config::UDP::_unconfiremed_package_limitation/2;

	bool rev = _packageAssembler.prepareUrgentARQSyncPackage(includeForceSyncSection, feedbackForceSync, fillDataSections);
	if (includeForceSyncSection && rev)
		_lastUrgentMsec = slack_real_msec();

	return rev;
}

bool UDPIOBuffer::prepareResentPackage_normalMode()
{
	uint32_t seqNum;
	UDPPackage* package;
	if (_protocolVersion > 1)
	{
		bool requireSingleResending;
		if (_unconformedMap.prepareSendingBuffer(_MTU, _resendThreshold, _currentSendingBuffer, requireSingleResending))
		{
			_resentCount -= (int)(_currentSendingBuffer->assembledPackages.size());
			return true;
		}
		else if (requireSingleResending)
		{
			package = _unconformedMap.fetchFirstResendPackage(_resendThreshold, seqNum);
			if (package)
			{
				_currentSendingBuffer->resendPackage(seqNum, package);
				_resentCount -= 1;
				return true;
			}
		}
	}
	else
	{
		package = _unconformedMap.v1_fetchResentPackage_normalMode(_resendThreshold, seqNum);
		if (package)
		{
			_currentSendingBuffer->resendPackage(seqNum, package);
			_resentCount -= 1;
			return true;
		}
	}

	return false;
}

bool UDPIOBuffer::checkECDHResending()
{
	if (_ecdhPackageReference->lastSentMsec > _resendThreshold)
		return false;

	_currentSendingBuffer->resendPackage(_ecdhSeq, _ecdhPackageReference);
	return true;
}

//--------------------------------------------//
//- UDP IO Buffer: Receive Utility Functions -//
//--------------------------------------------//
bool UDPIOBuffer::getRecvToken()
{
	std::unique_lock<std::mutex> lck(*_mutex);
	if (!_recvToken)
		return false;

	_recvToken = false;
	return true;
}

void UDPIOBuffer::returnRecvToken()
{
	std::unique_lock<std::mutex> lck(*_mutex);
	_recvToken = true;

	SyncARQStatus();

	_parseResult.reset();
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

	_seqManager.newReceivedSeqs(_arqParser.unprocessedReceivedSeqs);
	_seqManager.requireForceSync = _parseResult.requireForceSync;

	conformFeedbackSeqs();

	if (_arqParser.requireKeepLink)
		_requireKeepAlive = true;
}

void UDPIOBuffer::conformFeedbackSeqs()
{
	int64_t now = slack_real_msec();

	if (_parseResult.receivedUNA.size() > 0)
	{
		uint32_t una = _parseResult.receivedUNA[0];
		_unconformedMap.updateUNA(una);

		cleaningFeedbackAcks(una, _parseResult.receivedAcks);
		cleanConformedPackageByUNA(now, una);
	}

	if (_parseResult.receivedAcks.size())
	{
		cleanConformedPackageByAcks(now, _parseResult.receivedAcks);
	}
}

void UDPIOBuffer::cleaningFeedbackAcks(uint32_t una, std::unordered_set<uint32_t>& acks)
{
	/*
		TODO:
		可能考虑使用跳表，或者动态有序链表，优化操作
	*/
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

void UDPIOBuffer::cleanConformedPackageByUNA(int64_t now, uint32_t una)
{
	int count = 0;
	int64_t totalDelay = 0;

	_unconformedMap.cleanByUNA(una, now, count, totalDelay);

	_resendControl.updateDelay(now, totalDelay, count);
}

void UDPIOBuffer::cleanConformedPackageByAcks(int64_t now, std::unordered_set<uint32_t>& acks)
{
	int count = 0;
	int64_t totalDelay = 0;

	_unconformedMap.cleanByAcks(acks, now, count, totalDelay);

	_resendControl.updateDelay(now, totalDelay, count);
}

void UDPIOBuffer::checkEcdhSeq()
{
	if (_parseResult.receivedUNA.size() > 0)
	{
		uint32_t una = _parseResult.receivedUNA[0];
		if (una == _ecdhSeq || una - _ecdhSeq < _ecdhSeq - una)
		{
			_ecdhPackageReference = NULL;
			return;
		}
	}

	if (_parseResult.receivedAcks.find(_ecdhSeq) != _parseResult.receivedAcks.end())
	{
		_ecdhPackageReference = NULL;
		return;
	}
}

//--------------------------------------------//
//--    UDP IO Buffer: Receive Functions    --//
//--------------------------------------------//
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

			if (_parseResult.protocolVersion == _protocolVersion) ;
			else configProtocolVersion(_parseResult.protocolVersion);

			if (_parseResult.sendingEncryptor == NULL) ; else
			{
				/*
					Sending encryptor of UDPIOBuffer maybe configured.
					But sending encryptor is configured with receiving ecnryptor.
					Parser has checked the receiving ecnryptor, so we don't check sending encryptor here.
				*/
				configSendingEncryptor(_parseResult.sendingEncryptor, _parseResult.enableDataEncryption);
				_parseResult.sendingEncryptor = NULL;
			}

			if (_ecdhPackageReference == NULL) ;
			else checkEcdhSeq();
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
			if (_parseResult.protocolVersion == _protocolVersion) ;
			else configProtocolVersion(_parseResult.protocolVersion);

			if (_parseResult.sendingEncryptor == NULL) ; else
			{
				/*
					Sending encryptor of UDPIOBuffer maybe configured.
					But sending encryptor is configured with receiving ecnryptor.
					Parser has checked the receiving ecnryptor, so we don't check sending encryptor here.
				*/
				configSendingEncryptor(_parseResult.sendingEncryptor, _parseResult.enableDataEncryption);
				_parseResult.sendingEncryptor = NULL;
			}

			if (_ecdhPackageReference == NULL) ;
			else checkEcdhSeq();
			
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
