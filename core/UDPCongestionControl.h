#ifndef UDP_Congestion_Control_H
#define UDP_Congestion_Control_H

#include <list>
#include <stdint.h>

namespace fpnn
{
	class UDPSimpleCongestionController
	{
		struct UnconfirmedRecord
		{
			int64_t ts;
			size_t unconfirmedSize;
		};

		struct UnconfirmedLoad
		{
			int64_t ts;
			float load;
		};

		UnconfirmedRecord record;
		UnconfirmedLoad unconfirmedLoad;

		void updateUnconfirmedIndex();

	private:
		static const float unconfirmedIndexFactor;
		static const int64_t timeUnit;

	public:
		static const float halfLoadThreshold;
		static const float minLoadThreshold;

	public:
		UDPSimpleCongestionController();
		~UDPSimpleCongestionController();

		void updateUnconfirmedSize(int64_t ts, size_t unconfirmedSize);
		inline float loadIndex() { return unconfirmedLoad.load; }
	};

	class UDPResendIntervalController
	{
		struct RecordUnit
		{
			int count;
			int64_t ts;
			int64_t avgDelay;
		};

		struct CacheUnit
		{
			int64_t total;
			int count;
		};

		int64_t lastTs;
		int64_t lastDelay;
		int64_t minAvgDelay;

		RecordUnit record;
		CacheUnit cache;

	private:
		static const int64_t defaultIntervalMS;
		static const int64_t lastDelaySustainMS;
		static const int64_t lastDealyAttenuationMS;
		static const int64_t maxIntervalMS;
		static const int64_t calculationPeriodMS;
		static const float factor;

	public:
		UDPResendIntervalController();

		int64_t interval(int64_t now);
		void updateDelay(int64_t ts, int64_t totalDelay, int count);
	};
}

#endif