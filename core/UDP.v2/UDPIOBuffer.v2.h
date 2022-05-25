#ifndef FPNN_UDP_IOBuffer_v2_h
#define FPNN_UDP_IOBuffer_v2_h

#include <stdlib.h>
#include <stdint.h>
#include <string>
#include <mutex>
#include "../Config.h"
#include "../UDPCongestionControl.h"
#include "UDPUnconformedMap.v2.h"
#include "UDPAssembler.v2.h"
#include "UDPParser.v2.h"

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
		uint8_t _protocolVersion;

		volatile bool _requireKeepAlive;
		volatile bool _requireClose;
		int _untransmittedSeconds;
		int64_t _lastSentSec;					//-- Keep alive using
		std::atomic<int64_t> _lastRecvSec;		//-- Check alive using
		ActiveCloseStep _activeCloseStatus;

		//-- in
		int _recvBufferLen;
		uint8_t* _recvBuffer;
		ARQParser _arqParser;
		ParseResult _parseResult;

		//-- out
		UDPAssembler _packageAssembler;
		CurrentSendingBuffer* _currentSendingBuffer;
		UDPUnconformedMap _unconformedMap;
		UDPEncryptor* _sendingEncryptor;
		uint8_t* _encryptBuffer;

		UDPPackage* _ecdhPackageReference;
		uint32_t _ecdhSeq;

		UDPResendIntervalController _resendControl;

		// //-- Feedback we will send to peer.
		ARQPeerSeqManager _seqManager;

		bool _sendToken;
		bool _recvToken;
		std::mutex* _mutex;
		std::string _endpoint;

		SendingAdjustor _sendingAdjustor;

		int _resentCount;
		int64_t _lastUrgentMsec;
		int64_t _resendThreshold;

	private:
		void configSendingEncryptor(UDPEncryptor* encryptor, bool enableDataEncryption);
		void configProtocolVersion(uint8_t version);
		bool checkECDHResending();
		bool prepareUrgentARQSyncPackage();
		bool prepareResentPackage_normalMode();
		bool prepareSendingPackage(bool& blockByFlowControl);
		void realSend(bool& needWaitSendEvent, bool& blockByFlowControl);
		void paddingResendPackages();
		void updateResendTolerance();

		void checkEcdhSeq();
		void SyncARQStatus();							//-- require under mutex.
		void conformFeedbackSeqs();						//-- require under mutex.
		void cleaningFeedbackAcks(uint32_t una, std::unordered_set<uint32_t>& acks);
		void cleanConformedPackageByUNA(int64_t now, uint32_t una);
		void cleanConformedPackageByAcks(int64_t now, std::unordered_set<uint32_t>& acks);

	public:
		UDPIOBuffer(std::mutex* mutex, int socket, int MTU);
		~UDPIOBuffer();

		void initMutex(std::mutex* mutex) { _mutex = mutex; }
		void setKeyExchanger(ECCKeyExchange* exchanger) { _parseResult.keyExchanger = exchanger; }
		
		bool enableEncryptorAsInitiator(const std::string& curve, const std::string& peerPublicKey, bool reinforce);
		bool enableEncryptorAsInitiator(const std::string& curve, const std::string& peerPublicKey,
			bool reinforcePackage, bool reinforceData);
		inline bool isEncrypted() { return _sendingEncryptor != NULL; }
		
		void enableKeepAlive();
		void markActiveCloseSignal();
		void sendCloseSignal(bool& needWaitSendEvent);
		void updateEndpointInfo(const std::string& endpoint);
		inline void configConnectionReentry(bool replace) { _arqParser.configConnectionReentry(replace); }

		void setUntransmittedSeconds(int untransmittedSeconds);
		bool isTransmissionStopped();
		inline bool isRequireClose() { return _requireClose || _arqParser.invalidSession(); }

		inline std::list<FPQuestPtr>& getReceivedQuestList() { return _parseResult.questList; }
		inline std::list<FPAnswerPtr>& getReceivedAnswerList() { return _parseResult.answerList; }

		void sendCachedData(bool& needWaitSendEvent, bool& blockByFlowControl, bool socketReady = false);
		void sendData(bool& needWaitSendEvent, bool& blockByFlowControl, std::string* data, int64_t expiredMS, bool discardable);

		bool getRecvToken();
		void returnRecvToken();

		bool recvData();	//-- true: continue; false: stop.
		bool parseReceivedData(uint8_t* buffer, int len, UDPIOReceivedResult& result);	//-- true: result is available.
	};
}

#endif
