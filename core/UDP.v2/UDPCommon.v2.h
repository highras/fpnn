#ifndef FPNN_UDP_Common_v2_h
#define FPNN_UDP_Common_v2_h

#include <stdint.h>
#include <unordered_map>
#include <unordered_set>
#ifdef __APPLE__
	#include "Endian.h"
#else
	#include <endian.h>
#endif
#include "msec.h"
#include "../Encryptor.h"
#include "../KeyExchange.h"

namespace fpnn
{
	enum class ARQType: uint8_t
	{
		ARQ_COMBINED = 0x80,
		ARQ_ASSEMBLED = 0x81,
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
		static const uint8_t ARQ_LastSegmentData = 0x10;
		static const uint8_t ARQ_FirstPackage = 0x20;
		static const uint8_t ARQ_CancelledPackage = 0x80;

		static const uint8_t ARQ_1Byte_SegmentIndex = 0x04;
		static const uint8_t ARQ_2Bytes_SegmentIndex = 0x08;
		static const uint8_t ARQ_4Bytes_SegmentIndex = 0x0C;
	};

	struct ARQConstant
	{
		static const uint8_t Version = 2;
		static const int PackageMimimumLength = 8;
		static const int CombinedPackageMimimumLength = 16;
		static const int PackageHeaderSize = 8;
		static const int SectionHeaderSize = 4;
		static const int SegmentPackageSeqSize = 2;
		static const int UDPSeqOffset = 4;
		static const int AssembledPackageHeaderSize = 2;
		static const int AssembledPackageLengthFieldSize = 2;
	};

	struct ARQChecksum
	{
		uint32_t _preprossedFirstSeq;
		uint8_t _sign;

	public:
		ARQChecksum(uint32_t firstSeqBE, uint8_t sign);
		bool check(uint32_t udpSeqBE, uint8_t checksum);
		uint8_t genChecksum(uint32_t udpSeqBE);
		inline bool isSame(uint8_t sign) { return _sign == sign; }
	};

	struct ARQPeerSeqManager		//-- 需要回复的 ACK 和 UNA
	{
		bool unaAvailable;
		bool unaUpdated;
		bool repeatUNA;
		bool requireForceSync;
		uint32_t lastUNA;
		uint64_t lastSyncMsec;
		std::unordered_set<uint32_t> receivedSeqs;
		std::unordered_map<uint32_t, int64_t> feedbackedSeqs;		//-- 已经 发送了 Ack 的 peer 的 seq。
		
		ARQPeerSeqManager(): unaAvailable(false), unaUpdated(false), repeatUNA(false), requireForceSync(false)
		{
			lastSyncMsec = slack_real_msec();
		}

		void updateLastUNA(uint32_t lastUNA);
		void newReceivedSeqs(const std::unordered_set<uint32_t>& newSeqs);
		bool needSyncSeqStatus();
		bool needSyncUNA()
		{
			return unaAvailable && (unaUpdated || repeatUNA);
		}
		bool needSyncAcks()
		{
			return receivedSeqs.size() > 0;
		}
		void cleanReceivedSeqs();
	};

	class UDPEncryptor
	{
		PackageEncryptor* _packageEncryptor;
		StreamEncryptor* _dataEncryptor;

	public:
		struct EncryptorPair
		{
			UDPEncryptor* sender;
			UDPEncryptor* receiver;

			EncryptorPair(): sender(NULL), receiver(NULL) {}
		};

		static struct EncryptorPair createPair(ECCKeyExchange* keyExchanger, const std::string& publicKey, bool reinforce);
		static struct EncryptorPair createPair(ECCKeyExchange* keyExchanger, const std::string& packagePublicKey,
			bool reinforcePackage, const std::string& dataPublicKey, bool reinforceData);

		UDPEncryptor(): _packageEncryptor(NULL), _dataEncryptor(NULL) {}
		virtual ~UDPEncryptor();
		
		void configPackageEncryptor(uint8_t *key, size_t key_len, uint8_t *iv);
		void configDataEncryptor(uint8_t *key, size_t key_len, uint8_t *iv);

		virtual void packageEncrypt(void* dest, void* src, int len);
		virtual void packageDecrypt(void* dest, void* src, int len);

		virtual void dataEncrypt(void* dest, void* src, int len);
		virtual void dataDecrypt(void* dest, void* src, int len);
	};
}

#endif
