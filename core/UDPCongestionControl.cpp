#include "msec.h"
#include "Config.h"
#include "UDPCongestionControl.h"

using namespace fpnn;

//=================================================================//
//--              UDP Resend Interval Controller                 --//
//=================================================================//
//=================================================================//
//-- !!! 以下参数不可配置 !!!
//-- Magic Number 来源: 参数矩阵上千cases数百小时对比测试结果
//=================================================================//
const int64_t UDPResendIntervalController::defaultIntervalMS = 20;
const int64_t UDPResendIntervalController::lastDelaySustainMS = 2000;			//-- last delay 使用原始值的时间。
const int64_t UDPResendIntervalController::lastDealyAttenuationMS = 20 * 1000;	//-- last delay 衰减使用的时间(包含使用原始值的时间)。
const int64_t UDPResendIntervalController::maxIntervalMS = 150;
const int64_t UDPResendIntervalController::calculationPeriodMS = 250;
const float UDPResendIntervalController::factor = 1.2;

UDPResendIntervalController::UDPResendIntervalController(): lastTs(0), lastDelay(defaultIntervalMS), minAvgDelay(defaultIntervalMS)
{
	record.count = 0;
	record.ts = 0;
	record.avgDelay = defaultIntervalMS;

	cache.total = 0;
	cache.count = 0;
}

void UDPResendIntervalController::updateDelay(int64_t ts, int64_t totalDelay, int count)
{
	if (count == 0)
		return;

	if (ts - record.ts < calculationPeriodMS)
	{
		int64_t total = record.count * record.avgDelay + totalDelay;
		record.count += count;
		record.avgDelay = total / record.count;
	}
	else
	{
		if (record.avgDelay < minAvgDelay)
			minAvgDelay = record.avgDelay;

		//-- 无需额外判断，毕竟相关判断在 interval() 函数执行时，还需要再判断一次。因此直接简单处理。
		{
			lastTs = record.ts;
			lastDelay = record.avgDelay;
		}

		record.ts = ts;
		record.count = count;
		record.avgDelay = totalDelay / count;
	}
}

int64_t UDPResendIntervalController::interval(int64_t now)
{
	int64_t rev = minAvgDelay;
	int64_t timeDiff = now - lastTs;
	if (timeDiff <= lastDelaySustainMS)
	{
		if (lastDelay < maxIntervalMS)
			rev = lastDelay;
		else
			rev = maxIntervalMS;
	}
	else if (timeDiff < lastDealyAttenuationMS)
	{
		int64_t diff = lastDelay - minAvgDelay;
		int64_t refDelay = lastDelay - diff * timeDiff / lastDealyAttenuationMS;

		if (refDelay < maxIntervalMS)
			rev = refDelay;
		else
			rev = maxIntervalMS;
	}

	rev *= factor;
	
	if (rev == 0)
		rev = 1;

	return rev;
}
