#ifndef UDP_ARQ_Protocol_Parser_h
#define UDP_ARQ_Protocol_Parser_h

#include <stdlib.h>
#include <string.h>
#include <list>
#include <map>
#include <unordered_set>
#include <vector>
#include <atomic>
#include "FPMessage.h"

namespace fpnn
{
	enum class ARQType: uint8_t
	{
		ARQ_COMBINED = 0x80,
		ARQ_DATA = 0x01,
		ARQ_ACKS = 0x02,
		ARQ_UNA = 0x03,
		ARQ_ECDH = 0x04,
		ARQ_HEARTBEAT = 0x05,
		ARQ_FORCESYNC = 0x06,
		ARQ_CLOSE = 0x0F,
	};

	struct ARQFlag
	{
		static const uint8_t ARQ_Discardable = 0x01;
		static const uint8_t ARQ_Monitored = 0x02;
		static const uint8_t ARQ_SegmentedMask = 0x0c;
		static const uint8_t ARQ_LastSegmentMask = 0x10;
		static const uint8_t ARQ_FirstPackageMask = 0x20;

		static const uint8_t ARQ_1Byte_SegmentIndex = 0x04;
		static const uint8_t ARQ_2Bytes_SegmentIndex = 0x08;
		static const uint8_t ARQ_4Bytes_SegmentIndex = 0x0C;
	};

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

		~UDPUncompletedPackage()
		{
			for (auto& p: cache)
				delete p.second;
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
		void firstPackageReceived() { threshold = Config::UDP::_max_tolerated_milliseconds_before_valid_package_received; }
		void updateValidStatus();
		void updateInvalidPackageCount() { invalidCount++; }
		bool isInvalid();
	};

	struct ARQChecksum
	{
		uint32_t _preprossedFirstSeq;
		uint8_t _factor;

	public:
		ARQChecksum(uint32_t firstSeqBE, uint8_t factor);
		bool check(uint32_t udpSeqBE, uint8_t checksum);
		uint8_t genChecksum(uint32_t udpSeqBE);
		bool isSame(uint8_t factor);
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
		bool canbeFeedbackUNA;					//-- 单次 parse 重置
		bool receivedPriorSeqs;
		bool requireForceSync;

		ParseResult(): canbeFeedbackUNA(false), receivedPriorSeqs(false), requireForceSync(false)
		{
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
		std::unordered_set<uint32_t> receivedSeqs;		//-- 历史有效, 不含UNA.
		bool requireClose;						//-- 历史有效
		bool requireKeepLink;					//-- 历史有效
		uint32_t lastUDPSeq;					//-- 历史有效
		int uncompletedPackageSegmentCount;		//-- 历史有效

	private:
		ARQChecksum* _arqChecksum;
		std::map<uint32_t, ClonedBuffer*> _disorderedCache;		//-- 历史有效
		std::map<uint16_t, UDPUncompletedPackage*> _uncompletedPackages;		//-- 历史有效
		uint8_t* _buffer;						//-- 单次 parse 重置
		int _bufferLength;						//-- 单次 parse 重置
		int _bufferOffset;						//-- 单次 parse 重置
		struct ParseResult* _parseResult;		//-- 单次 parse 重置

		//-- error log info
		int _socket;							//-- 历史有效
		const char* _endpoint;						//-- 历史有效

		SessionInvalidChecker _invalidChecker;

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
		void cacheCurrentUDPPackage(uint32_t packageSeq);
		void verifyCachedPackage(uint32_t baseUDPSeq);
		bool dropDiscardableCachedUncompletedPackage();
		void EndpointQuaternionConflictError(uint8_t factor, uint8_t type, uint8_t flag);

	public:
		ARQParser(): requireClose(false), requireKeepLink(false), uncompletedPackageSegmentCount(0),
			_arqChecksum(NULL), _socket(0), _endpoint("<unknown>") {}
		~ARQParser()
		{
			if (_arqChecksum)
				delete _arqChecksum;

			for (auto& pp: _disorderedCache)
				delete pp.second;

			for (auto& pp: _uncompletedPackages)
				delete pp.second;
		}

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