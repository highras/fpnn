#ifndef FPNN_UDP_UnconformedMap_v2_h
#define FPNN_UDP_UnconformedMap_v2_h

#include <set>
#include <map>
#include <deque>
#include <unordered_map>
#include "UDPAssembler.v2.h"

namespace fpnn
{
	struct ARQSelfSeqManager
	{
		bool unaAvailable;
		uint32_t una;

		ARQSelfSeqManager(): unaAvailable(false) {}
	};

	struct ResendTracer
	{
		uint32_t una;
		uint32_t lastSeq;
		uint32_t count;
		uint32_t step;

		ResendTracer(): una(0), lastSeq(0), count(0), step(0) {}
		void update(uint32_t una, size_t limitedCount);
		void reset();
	};

	class UDPUnconformedMap
	{
		struct PackageNode
		{
			uint32_t seqNum;
			UDPPackage* package;

			PackageNode(uint32_t seq, UDPPackage* pkg): seqNum(seq), package(pkg) {}
		};

		std::unordered_map<uint32_t, PackageNode*> _unconformedMap;
		std::deque<PackageNode*> _sentQueue;

		ARQSelfSeqManager _selfSeqManager;
		ResendTracer _resendTracer;
		bool _enableExpireCheck;

		void fetchResendPackages(int freeSpace, int64_t threshold, std::list<PackageNode*>& canbeAssembledPackages);
		void assemblePackages(UDPPackage* package, std::list<PackageNode*>& canbeAssembledPackages,
			CurrentSendingBuffer* sendingBuffer);
		void assemblePackages(std::set<PackageNode*>& selectedPackages,
			std::list<PackageNode*>& supplementaryPackages, CurrentSendingBuffer* sendingBuffer);
		
	public:
		UDPUnconformedMap(): _enableExpireCheck(true) {}
		~UDPUnconformedMap();
		inline size_t size() { return _unconformedMap.size(); }
		inline void disableExpireCheck() { _enableExpireCheck = false; }
		
		inline void updateUNA(uint32_t una)
		{
			_selfSeqManager.una = una;
			_selfSeqManager.unaAvailable = true;
		}

		//-- Only can insert reliable package.
		void insert(uint32_t seqNum, UDPPackage* package);
		bool prepareSendingBuffer(int MTU, int64_t threshold, UDPPackage* package, CurrentSendingBuffer* sendingBuffer);
		bool prepareSendingBuffer(int MTU, int64_t threshold, CurrentSendingBuffer* sendingBuffer);

		void cleanByUNA(uint32_t una, int64_t now, int &count, int64_t &totalDelay);
		void cleanByAcks(const std::unordered_set<uint32_t>& acks, int64_t now, int &count, int64_t &totalDelay);

		//-- Compatible for protocol version 1.
		UDPPackage* v1_fetchResentPackage_normalMode(int64_t threshold, uint32_t& seqNum);
	};
}

#endif
