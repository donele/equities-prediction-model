#include <HOrders/HaltSimCumPred.h>
using namespace std;
using namespace gtlib;

HaltSimCumPred::HaltSimCumPred(int id, double minPrice, int minMsecs,
		int pastSec, int haltLenSec, int minNtrades, double stopBpt)
	:HaltSim(id, minPrice, minMsecs),
	pastSec_(pastSec),
	stopBpt_(stopBpt),
	haltLenSec_(haltLenSec),
	minNtrades_(minNtrades)
{
}

HaltCondition* HaltSimCumPred::getHaltCondition(const string& ticker,
		const vector<MercatorTrade>& mts, const QuoteSample& qSample)
{
	HaltCondition* hc = new HaltConditionCumPred(ticker, mts, qSample,
			minPrice_, minMsecs_, pastSec_, haltLenSec_, minNtrades_, stopBpt_, debug_);
	return hc;
}

string HaltSimCumPred::getDesc()
{
	char buf[200];
	sprintf(buf, "for %d s. | %d secCumPred%d > %.1f bpt",
			haltLenSec_, pastSec_, minNtrades_, stopBpt_);
	return buf;
}

//
// HaltCondition
//

HaltConditionCumPred::HaltConditionCumPred()
{
}

HaltConditionCumPred::HaltConditionCumPred(const string& ticker, const vector<MercatorTrade>& mts,
		const QuoteSample& qSample, double minPrice, int minMsecs,
		int pastSec, int haltLenSec, int minNtrades, double stopBpt, bool debug)
	:HaltCondition(haltLenSec)
{
	int NT = mts.size();
	for( int iT = 0; iT < NT; ++iT )
	{
		const MercatorTrade& mt = mts[iT];
		if( mt.price < minPrice )
			break;
		if( isHalted(mt.msecs) || mt.msecs < minMsecs )
			continue;

		int ntrades = 1;
		double cumpred = mt.pred;
		for( int i2 = iT - 1; i2 >= 0; --i2 )
		{
			const MercatorTrade& mtPast = mts[i2];
			if( mtPast.msecs >= mt.msecs - pastSec * 1000 && !isHalted(mtPast.msecs))
			{
				if( mtPast.sign == mt.sign )
				{
					++ntrades;
					cumpred += mtPast.pred;
				}
				else
				{
					ntrades = 0;
					cumpred = 0.;
					break;
				}
			}
		}
		if( ntrades >= minNtrades && fabs(cumpred) >= stopBpt )
		{
			int haltMsecs = mt.msecs;
			vStopMsecs_.push_back(haltMsecs);
			if( debug )
			{
				printf("%s h: %d pred: %.1f\n",
						ticker.c_str(), haltMsecs, mt.pred);
			}
		}
	}
}
