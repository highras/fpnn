#ifndef FPNN_UDP_Parser_v2_h
#define FPNN_UDP_Parser_v2_h

#include <stdlib.h>
#include <string.h>
#include <list>
#include <map>
#include <unordered_set>
#include <vector>
#include <atomic>
#include "FPMessage.h"
#include "../Config.h"
#include "UDPCommon.v2.h"

namespace fpnn
{
	struct ClonedBuffer
	{
		uint8_t* data;
		uint16_t len;

		ClonedBuffer(const void* srcBuffer, int length): len(length)
		{
			data = (uint8_t*)malloc(length);
			memcpy(data, srcBuffer, length);
		}
		~ClonedBuffer() { free(data); }
	};

	struct UDPUncompletedPackage
	{
		uint32_t count;
		uint32_t cachedSegmentSize;
		int64_t createSeconds;
		bool discardable;
		std::map<uint32_t, ClonedBuffer*> cache;

		UDPUncompletedPackage(): count(0), cachedSegmentSize(0), discardable(false)
		{
			createSeconds = slack_real_sec();
		}

		~UDPUncompletedPackage()
		{
			for (auto& p: cache)
				delete p.second;
		}

		void cacheClone(uint32_t segmentIndex, const void* srcBuffer, uint32_t length)
		{
			cache[segmentIndex] = new ClonedBuffer(srcBuffer, length);
			cachedSegmentSize += length;
		}
	};

	class SessionInvalidChecker
	{
		std::atomic<uint64_t> lastValidMsec;
		std::atomic<uint64_t> threshold;
		std::atomic<int> invalidCount;
	public:
		SessionInvalidChecker(): lastValidMsec(0), threshold(0), invalidCount(0)
		{
			threshold = Config::UDP::_max_tolerated_milliseconds_before_first_package_received;
		}
		inline void startCheck()
		{
			if (lastValidMsec == 0)
				lastValidMsec = slack_real_msec();
		}
		inline void firstPackageReceived() { threshold = Config::UDP::_max_tolerated_milliseconds_before_valid_package_received; }
		inline void updateInvalidPackageCount() { invalidCount++; }
		void updateValidStatus();
		bool isInvalid();
	};

	struct ParseResult
	{
		//-- received
		std::list<FPQuestPtr> questList;		//-- 批量 parse 重置，每次调用 UDPIOBuffer::recvData() 后需要及时处理。
		std::list<FPAnswerPtr> answerList;		//-- 批量 parse 重置，每次调用 UDPIOBuffer::recvData() 后需要及时处理。
		//-- received: feedback that peer sent to me.
		std::unordered_set<uint32_t> receivedAcks;		//-- 批量 parse 重置
		std::vector<uint32_t> receivedUNA;		//-- 批量 parse 重置

		//-- feedback we will send to peer.
		uint8_t protocolVersion;
		bool canbeFeedbackUNA;					//-- 单次 parse 重置
		bool receivedPriorSeqs;
		bool requireForceSync;

		ECCKeyExchange* keyExchanger;
		UDPEncryptor* sendingEncryptor;
		bool enableDataEncryption;

		ParseResult(): canbeFeedbackUNA(false), receivedPriorSeqs(false), requireForceSync(false),
			keyExchanger(NULL), sendingEncryptor(NULL), enableDataEncryption(false)
		{
			protocolVersion = ARQConstant::Version;
		}

		void reset()
		{
			questList.clear();
			answerList.clear();
			receivedAcks.clear();
			receivedUNA.clear();
			canbeFeedbackUNA = false;
			receivedPriorSeqs = false;
			requireForceSync = false;
			sendingEncryptor = NULL;
		}

		void receiveUNA(uint32_t una)
		{
			if (receivedUNA.empty())
				receivedUNA.push_back(una);
			else
			{
				uint32_t a = una - receivedUNA[0];
				uint32_t b = receivedUNA[0] - una;
				if (a < b)
					receivedUNA[0] = una;
			}
		}
	};

	class ARQParser
	{
	public:
		std::unordered_set<uint32_t> unprocessedReceivedSeqs;		//-- 历史有效, 不含UNA.
		bool requireClose;						//-- 历史有效
		bool requireKeepLink;					//-- 历史有效
		uint32_t lastUDPSeq;					//-- 历史有效
		int uncompletedPackageSegmentCount;		//-- 历史有效

	private:
		ARQChecksum* _arqChecksum;
		std::map<uint32_t, ClonedBuffer*> _disorderedCache;		//-- 历史有效
		std::map<uint16_t, UDPUncompletedPackage*> _uncompletedPackages;		//-- 历史有效

		//-- 每次 parse() 调用重置
		uint8_t* _buffer;						//-- 单次 parse 重置
		int _bufferLength;						//-- 单次 parse 重置
		int _bufferOffset;						//-- 单次 parse 重置
		struct ParseResult* _parseResult;		//-- 单次 parse 重置

		//-- error log info
		int _socket;							//-- 历史有效
		const char* _endpoint;						//-- 历史有效

		bool _unorderedParse;
		bool _replaceConnectionWhenConnectionReentry;
		SessionInvalidChecker _invalidChecker;
		UDPEncryptor* _encryptor;
		bool _enableDataEnhanceEncrypt;
		int _decryptedBufferLen;
		uint8_t* _decryptedBuffer;
		uint8_t* _decryptedDataBuffer;

		ClonedBuffer* _ecdhCopy;
		int64_t ecdhCopyExpiredTMS;
		uint32_t _firstPackageUDPSeq;

		bool processAssembledPackage();
		bool processPackage(uint8_t type, uint8_t flag);
		bool parseCOMBINED();
		bool parseDATA();
		bool parseACKS();
		bool parseUNA();
		bool parseECDH();
		bool parseHEARTBEAT();
		bool parseForceSync();

		bool assembleSegments(uint16_t packageId);
		bool decodeBuffer(uint8_t* buffer, uint32_t len);
		bool processReliableAndMonitoredPackage(uint8_t type, uint8_t flag);
		void processCachedPackageFromSeq();
		void verifyAndProcessCachedPackages();
		void cacheCurrentUDPPackage(uint32_t packageSeq);
		void verifyCachedPackage(uint32_t baseUDPSeq);
		bool dropDiscardableCachedUncompletedPackage();
		bool aheadProcessReliableAndMonitoredPackage(uint8_t type, uint8_t flag, uint32_t packageSeq);
		void checkLastUDPSeq();

	public:
		ARQParser(): requireClose(false), requireKeepLink(false), uncompletedPackageSegmentCount(0),
			_arqChecksum(NULL), _socket(0), _endpoint("<unknown>"), _unorderedParse(true),
			_replaceConnectionWhenConnectionReentry(false), _encryptor(NULL),
			_enableDataEnhanceEncrypt(false), _decryptedBuffer(NULL), _decryptedDataBuffer(NULL),
			_ecdhCopy(NULL) {}
		~ARQParser()
		{
			if (_arqChecksum)
				delete _arqChecksum;

			for (auto& pp: _disorderedCache)
				delete pp.second;

			for (auto& pp: _uncompletedPackages)
				delete pp.second;

			if (_encryptor)
				delete _encryptor;

			if (_decryptedBuffer)
				free(_decryptedBuffer);

			if (_decryptedDataBuffer)
				free(_decryptedDataBuffer);

			if (_ecdhCopy)
				delete _ecdhCopy;
		}

		void configPackageEncryptor(uint8_t *key, size_t key_len, uint8_t *iv);
		void configDataEncryptor(uint8_t *key, size_t key_len, uint8_t *iv);
		void configUnorderedParse(bool enable);			//-- Retention interface. Current on case using.
		inline void configConnectionReentry(bool replace) { _replaceConnectionWhenConnectionReentry = replace; }

		inline void setDecryptedBufferLen(int len) { _decryptedBufferLen = len; }
		inline void changeLogInfo(int socket, const char* endpoint)
		{
			_socket = socket;
			_endpoint = endpoint ? endpoint : "<unknown>";
		}
		bool parse(uint8_t* buffer, int len, struct ParseResult* result);
		void dropExpiredCache(int64_t threshold);		//-- MUST in mutex and got the recv token.
		bool invalidSession() { return _invalidChecker.isInvalid(); }
	};
}

#endif
