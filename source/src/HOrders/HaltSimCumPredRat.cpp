#include <HOrders/HaltSimCumPredRat.h>
using namespace std;
using namespace gtlib;

HaltSimCumPredRat::HaltSimCumPredRat(int id, double minPrice, int minMsecs,
		int pastSec, int haltLenSec, int minNtrades, double stopRat)
	:HaltSim(id, minPrice, minMsecs),
	pastSec_(pastSec),
	stopRat_(stopRat),
	haltLenSec_(haltLenSec),
	minNtrades_(minNtrades)
{
}

HaltCondition* HaltSimCumPredRat::getHaltCondition(const string& ticker,
		const vector<MercatorTrade>& mts, const QuoteSample& qSample)
{
	HaltCondition* hc = new HaltConditionCumPredRat(ticker, mts, qSample, minPrice_, minMsecs_,
			pastSec_, haltLenSec_, minNtrades_, stopRat_, debug_);
	return hc;
}

string HaltSimCumPredRat::getDesc()
{
	char buf[200];
	sprintf(buf, "for %d s. | %d secCumPredRat%d > %.1f",
			haltLenSec_, pastSec_, minNtrades_, stopRat_);
	return buf;
}

//
// HaltCondition
//

HaltConditionCumPredRat::HaltConditionCumPredRat()
{
}

HaltConditionCumPredRat::HaltConditionCumPredRat(const string& ticker, const vector<MercatorTrade>& mts,
		const QuoteSample& qSample, double minPrice, int minMsecs,
		int pastSec, int haltLenSec, int minNtrades, double stopRat, bool debug)
	//:haltLenSec_(haltLenSec)
	:HaltCondition(haltLenSec)
{
	int NT = mts.size();
	for( int iT = 0; iT < NT; ++iT )
	{
		const MercatorTrade& mt = mts[iT];
		if( mt.price < minPrice )
			break;
		if( mt.msecs < minMsecs || isHalted(mt.msecs) )
			continue;

		int ntrades = 1;
		int prevMsecs = mt.msecs;
		if( mt.cost > 0. )
		{
			double cumrat = mt.pred / mt.cost;
			for( int i2 = iT - 1; i2 >= 0; --i2 )
			{
				const MercatorTrade& mtPast = mts[i2];
				//if( mtPast.msecs < prevMsecs - 1000 && mtPast.msecs >= mt.msecs - pastSec * 1000 && !isHalted(mtPast.msecs))
				if( mtPast.msecs >= mt.msecs - pastSec * 1000 && !isHalted(mtPast.msecs))
				{
					if( mtPast.sign == mt.sign && mtPast.cost > 0. )
					{
						++ntrades;
						cumrat += mtPast.pred / mt.cost;
					}
					else
					{
						ntrades = 0;
						cumrat = 0.;
						break;
					}
				}
				prevMsecs = mtPast.msecs;
			}
			if( ntrades >= minNtrades && fabs(cumrat) >= stopRat )
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
}
