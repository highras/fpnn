#include "../Config.h"
#include "UDPCommon.v2.h"

using namespace fpnn;

ARQChecksum::ARQChecksum(uint32_t firstSeqBE, uint8_t sign): _sign(sign)
{
	uint8_t* fseqBE = (uint8_t*)&firstSeqBE;
	fseqBE[1] ^= sign;
	fseqBE[3] ^= sign;

	uint8_t cyc = sign % 32;
	_preprossedFirstSeq = (*fseqBE >> cyc) | (*fseqBE << (32 - cyc));
}

bool ARQChecksum::check(uint32_t udpSeqBE, uint8_t checksum)
{
	return genChecksum(udpSeqBE) == checksum;
}

uint8_t ARQChecksum::genChecksum(uint32_t udpSeqBE)
{
	uint8_t rsign = ~_sign;
	uint8_t* cseqBE = (uint8_t*)&udpSeqBE;

	cseqBE[0] ^= rsign;
	cseqBE[2] ^= rsign;

	uint8_t cyc = rsign % 32;
	uint32_t newC = (*cseqBE << cyc) | (*cseqBE >> (32 - cyc));
	uint32_t res = _preprossedFirstSeq ^ newC;
	uint8_t* resByte = (uint8_t*)&res;

	uint8_t value = resByte[0] + resByte[1] + resByte[2] + resByte[3];
	return value;
}

//=====================================================================//
//--                           UDP Assembler                         --//
//=====================================================================//
/*
	TODO:
	可能考虑使用跳表，或者动态有序链表，优化操作
*/

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
				else if (it->second <= threshold)	//-- 如果超过重回应间隔，还在 feedbackedSeqs 集合里，说明对方还为同步UNA。可能需要重新应答。
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
			else if (it->second <= threshold)	//-- 如果超过重回应间隔，还在 feedbackedSeqs 集合里，说明对方还为同步UNA。可能需要重新应答。
				extracted.insert(seq);
		}
	}

	receivedSeqs.swap(extracted);
}

//=====================================================================//
//--                           UDP Encryptor                         --//
//=====================================================================//
UDPEncryptor::EncryptorPair UDPEncryptor::createPair(ECCKeyExchange* keyExchanger, const std::string& publicKey, bool reinforce)
{
	struct EncryptorPair pair;

	uint8_t key[32];
	uint8_t iv[16];
	int aesKeyLen = reinforce ? 32 : 16;
	if (keyExchanger->calcKey(key, iv, aesKeyLen, publicKey))
	{
		pair.sender = new UDPEncryptor();
		pair.sender->configPackageEncryptor(key, aesKeyLen, iv);

		pair.receiver = new UDPEncryptor();
		pair.receiver->configPackageEncryptor(key, aesKeyLen, iv);
	}

	return pair;
}

UDPEncryptor::EncryptorPair UDPEncryptor::createPair(ECCKeyExchange* keyExchanger, const std::string& packagePublicKey,
	bool reinforcePackage, const std::string& dataPublicKey, bool reinforceData)
{
	struct EncryptorPair pair;

	uint8_t packageKey[32];
	uint8_t packageIV[16];
	int packageKeyLen = reinforcePackage ? 32 : 16;
	if (keyExchanger->calcKey(packageKey, packageIV, packageKeyLen, packagePublicKey) == false)
		return pair;

	uint8_t dataKey[32];
	uint8_t dataIV[16];
	int dataKeyLen = reinforceData ? 32 : 16;
	if (keyExchanger->calcKey(dataKey, dataIV, dataKeyLen, dataPublicKey) == false)
		return pair;

	pair.sender = new UDPEncryptor();
	pair.sender->configPackageEncryptor(packageKey, packageKeyLen, packageIV);
	pair.sender->configDataEncryptor(dataKey, dataKeyLen, dataIV);

	pair.receiver = new UDPEncryptor();
	pair.receiver->configPackageEncryptor(packageKey, packageKeyLen, packageIV);
	pair.receiver->configDataEncryptor(dataKey, dataKeyLen, dataIV);

	return pair;
}

void UDPEncryptor::configPackageEncryptor(uint8_t *key, size_t key_len, uint8_t *iv)
{
	if (_packageEncryptor)
		delete _packageEncryptor;

	_packageEncryptor = new PackageEncryptor(key, key_len, iv);
}
void UDPEncryptor::configDataEncryptor(uint8_t *key, size_t key_len, uint8_t *iv)
{
	if (_dataEncryptor)
		delete _dataEncryptor;

	_dataEncryptor = new StreamEncryptor(key, key_len, iv);
}

UDPEncryptor::~UDPEncryptor()
{
	delete _packageEncryptor;

	if (_dataEncryptor)
		delete _dataEncryptor;
}

void UDPEncryptor::packageEncrypt(void* dest, void* src, int len)
{
	_packageEncryptor->encrypt((uint8_t*)dest, (uint8_t*)src, len);
}

void UDPEncryptor::packageDecrypt(void* dest, void* src, int len)
{
	_packageEncryptor->decrypt((uint8_t*)dest, (uint8_t*)src, len);
}

void UDPEncryptor::dataEncrypt(void* dest, void* src, int len)
{
	_dataEncryptor->encrypt((uint8_t*)dest, (uint8_t*)src, len);
}

void UDPEncryptor::dataDecrypt(void* dest, void* src, int len)
{
	_dataEncryptor->decrypt((uint8_t*)dest, (uint8_t*)src, len);
}
