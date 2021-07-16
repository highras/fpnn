#ifndef UDP_IO_Buffer_h
#define UDP_IO_Buffer_h

#include <stdlib.h>
#include <stdint.h>
#include <string>
#include <queue>
#include <mutex>
#include <list>
#include <unordered_map>
#include <unordered_set>
#include "msec.h"
#include "FPMessage.h"
#include "Config.h"
#include "UDPARQProtocolParser.h"
#include "UDPCongestionControl.h"
#include "IQuestProcessor.h"

//-- MTU: 548 = 576 - 20 (IP header) - 8 (UDP header); 1472 = 1500 - 20 - 8.
//-- MTU: FDDI: 4352, Ethernet: 1500, X.25/Internet: 576
//-- IEEE 802.3/PPPoE: Ethernet - 2 bytes(PPP) - 6 bytes(oE)
//-- MTU: https://tools.ietf.org/html/rfc1191

namespace fpnn
{
	struct MTUSPEC
	{
		static const int FDDI = 4352;
		static const int Ethernet = 1500;
		static const int Internet = 576;
		static const int X25 = 576;
		static const int PPPoE = 1492;
	};

	struct UDPIOReceivedResult
	{
		bool requireClose;
		std::list<FPQuestPtr> questList;
		std::list<FPAnswerPtr> answerList;

		UDPIOReceivedResult(): requireClose(false) {}
		
		void reset()
		{
			requireClose = false;
			questList.clear();
			answerList.clear();
		}
	};

	struct UDPDataUnit
	{
		bool discardable;
		std::string* data;
		int64_t expiredMS;

		UDPDataUnit(std::string* data_, bool discardable_, int64_t expiredMS_):
			discardable(discardable_), data(data_), expiredMS(expiredMS_) {}

		~UDPDataUnit()
		{
			delete data;
		}
	};

	struct SegmentInfo
	{
		UDPDataUnit* data;
		uint32_t nextIndex;
		size_t offset;

		SegmentInfo(): data(NULL), nextIndex(1), offset(0) {}
		~SegmentInfo()
		{
			if (data)
				delete data;
		}

		void reset()
		{
			if (data)
			{
				delete data;
				data = NULL;
			}
			nextIndex = 1;
			offset = 0;
		}
	};

	struct UDPPackage
	{
		void* buffer;
		size_t len;
		int sentCount;
		int64_t firstSentMsec;
		int64_t lastSentMsec;
		bool resending;
		bool requireDeleted;
		bool includeUNA;

		UDPPackage(): sentCount(0), resending(false), requireDeleted(false) { includeUNA = false; }
		~UDPPackage() { free(buffer); }
		void updateSendingInfo()
		{
			sentCount += 1;
			lastSentMsec = slack_real_msec();
		}
	};

	struct CurrentSendingBuffer
	{
		uint8_t* rawBuffer;
		size_t bufferLength;

		uint8_t* dataBuffer;
		size_t dataLength;

		bool discardable;
		uint32_t packageSeq;

		bool sendDone;
		bool requireUpdateSeq;

		int sentCount;
		int64_t lastSentMsec;
		bool includeUNA;

		UDPPackage* resendingPackage;

		CurrentSendingBuffer();
		~CurrentSendingBuffer()
		{
			if (rawBuffer)
				free(rawBuffer);
		}

		void init(int bufferSize);
		void reset();

		void updateSendingInfo();
		UDPPackage* dumpPackage();
		void resendPackage(uint32_t seq, UDPPackage* package);
		void setType(uint8_t type);
		void setType(ARQType type);
		void addFlag(uint8_t flag);
		void setFlag(uint8_t flag);
		void setFactor(uint8_t factor);
		void setUDPSeq(uint32_t seqBE);
		void setComponentType(uint8_t* componentBegin, uint8_t type);
		void setComponentType(uint8_t* componentBegin, ARQType type);
		void setComponentFlag(uint8_t* componentBegin, uint8_t flag);
		void setComponentBytes(uint8_t* componentBegin, size_t bytes);
		void setDataComponentPackageSeq(uint8_t* segmentBegin, uint16_t seq);
		size_t setDataComponentSegmentIndex(uint8_t* segmentBegin, uint32_t index);		//-- return index space size.
		bool changeCombinedPackageToSinglepackage();
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

	//private:
		void cleanReceivedSeqs();
	};

	struct ARQSelfSeqManager
	{
		bool unaAvailable;
		uint32_t una;

		ARQSelfSeqManager(): unaAvailable(false) {}
		bool checkResentSeq(uint32_t seq);
	};

	struct ResendTracer
	{
		uint32_t una;
		uint32_t lastSeq;
		uint32_t count;
		uint32_t step;

		uint32_t debug;

		ResendTracer(): una(0), lastSeq(0), count(0), step(0) {}
		void update(uint32_t una, size_t limitedCount);
		void reset();
	};

	struct SendingAdjustor
	{
	private:
		bool canRevoke;
		uint64_t recordingSec;
		size_t sentPackageCount;

	public:
		bool sendingCheck()
		{
			canRevoke = true;

			uint64_t now = slack_mono_sec();
			if (recordingSec == now)
			{
				if (sentPackageCount >= Config::UDP::_max_package_sent_limitation_per_connection_second)
				{
					canRevoke = false;
					return false;
				}

				sentPackageCount += 1;
				return true;
			}
			else
			{
				recordingSec = now;
				sentPackageCount = 1;
				return true;
			}
		}

		void revoke()
		{
			if (canRevoke && sentPackageCount > 0)
				sentPackageCount -= 1;
		}

		SendingAdjustor()
		{
			sentPackageCount = 0;
			recordingSec = slack_mono_sec();
		}
	};

	class UDPIOBuffer
	{
		enum class ActiveCloseStep
		{
			None,
			Required,
			GenPackage,
			PackageSent
		};

		int _socket;
		int _MTU;

		volatile bool _requireKeepAlive;
		volatile bool _requireClose;
		// bool _socketReadyForSending;
		uint16_t _packageIdNumber;
		int _untransmittedSeconds;
		int64_t _lastSentSec;		//-- Keep alive using
		std::atomic<int64_t> _lastRecvSec;		//-- Check alive using
		ActiveCloseStep _activeCloseStatus;

		//-- in
		int _recvBufferLen;
		uint8_t* _recvBuffer;
		ARQParser _arqParser;
		ParseResult _parseResult;

		//-- out
		ARQChecksum* _arqChecksum;
		CurrentSendingBuffer _currentSendingBuffer;
		SegmentInfo _sendingSegmentInfo;
		std::queue<UDPDataUnit*> _dataQueue;
		std::unordered_map<uint32_t, UDPPackage*> _unconformedMap;

		UDPResendIntervalController _resendControl;
		UDPSimpleCongestionController _congestionControl;
		ResendTracer _resendTracer;
		ARQSelfSeqManager _selfSeqManager;

		//-- Feedback we will send to peer.
		ARQPeerSeqManager _seqManager;

		bool _sendToken;
		bool _recvToken;
		std::mutex* _mutex;
		std::string _endpoint;

		SendingAdjustor _sendingAdjustor;

		uint32_t _UDPSeqBase;

	private:

		int _resentCount;
		int _unaIncludeIndex;
		int64_t _lastUrgentMsec;

		uint8_t genChecksum(uint32_t udpSeqBE);
		void prepareClosePackage();
		void prepareHeartbeatPackage();
		bool prepareUrgentARQSyncPackage(bool& blocked);
		bool prepareCommonPackage();
		bool completeCommonPackage(int sectionCount);
		void prepareForceSyncSection();
		void prepareUNASection();
		void prepareAcksSection();
		void prepareSingleDataSection(size_t availableSpace);
		void prepareFirstSegmentedDataSection(size_t availableSpace);
		bool prepareSegmentedDataSection(int sectionCount);
		bool prepareDataSection(int sectionCount);
		bool prepareResentPackage_normalMode();
		bool prepareResentPackage_halfMode() { return prepareResentPackage_limitedMode(Config::UDP::_unconfiremed_package_limitation/2); }
		bool prepareResentPackage_minMode() { return prepareResentPackage_limitedMode(10); }
		bool prepareResentPackage_limitedMode(size_t count);
		bool prepareSendingPackage(bool& blockByFlowControl);
		void preparePackageCompleted(bool discardable, uint32_t udpSeq, uint32_t udpSeqBE, uint8_t factor);

		bool updateUDPSeq();
		void realSend(bool& needWaitSendEvent, bool& blockByFlowControl);
		void conformFeedbackSeqs();						//-- require under mutex.
		void SyncARQStatus();							//-- require under mutex.
		void cleaningFeedbackAcks(uint32_t una, std::unordered_set<uint32_t>& acks);
		void cleanConformedPackageByUNA(int64_t now, uint32_t una);
		void cleanConformedPackageByAcks(int64_t now, std::unordered_set<uint32_t>& acks);

	public:
		UDPIOBuffer(std::mutex* mutex, int socket, int MTU);
		~UDPIOBuffer();

		void initMutex(std::mutex* mutex) { _mutex = mutex; }

		void sendCachedData(bool& needWaitSendEvent, bool& blockByFlowControl, bool socketReady = false);
		void sendData(bool& needWaitSendEvent, bool& blockByFlowControl, std::string* data, int64_t expiredMS, bool discardable);
		bool parseReceivedData(uint8_t* buffer, int len, UDPIOReceivedResult& result);	//-- true: result is available.
		bool getRecvToken();
		void returnRecvToken();
		bool recvData();	//-- true: continue; false: stop.
		inline std::list<FPQuestPtr>& getReceivedQuestList() { return _parseResult.questList; }
		inline std::list<FPAnswerPtr>& getReceivedAnswerList() { return _parseResult.answerList; }

		inline bool isRequireClose() { return _requireClose || _arqParser.invalidSession(); }
		inline bool invalidSession() { return _arqParser.invalidSession(); }
		bool isTransmissionStopped();
		void enableKeepAlive();
		void setUntransmittedSeconds(int untransmittedSeconds);
		void markActiveCloseSignal();
		void sendCloseSignal(bool& needWaitSendEvent);
		void updateEndpointInfo(const std::string& endpoint);
	};
}

#endif