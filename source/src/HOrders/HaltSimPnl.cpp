#include <HOrders/HaltSimPnl.h>
using namespace std;
using namespace gtlib;

HaltSimPnl::HaltSimPnl(int id, double minPrice, int minMsecs,
		int pastSec, int haltLenSec, double stopBpt)
	:HaltSim(id, minPrice, minMsecs),
	pastSec_(pastSec),
	stopBpt_(stopBpt),
	haltLenSec_(haltLenSec)
{
}

HaltCondition* HaltSimPnl::getHaltCondition(const string& ticker,
		const vector<MercatorTrade>& mts, const QuoteSample& qSample)
{
	HaltCondition* hc = new HaltConditionPnl(ticker, mts, qSample, minPrice_, minMsecs_,
			pastSec_, haltLenSec_, stopBpt_, debug_);
	return hc;
}

string HaltSimPnl::getDesc()
{
	char buf[200];
	sprintf(buf, "for %d s. | %d secPnl < %.1f bpt",
			haltLenSec_, pastSec_, stopBpt_);
	return buf;
}

//
// HaltCondition
//

HaltConditionPnl::HaltConditionPnl()
{
}

HaltConditionPnl::HaltConditionPnl(const string& ticker, const vector<MercatorTrade>& mts,
		const QuoteSample& qSample, double minPrice, int minMsecs,
		int pastSec, int haltLenSec, double stopBpt, bool debug)
	:HaltCondition(haltLenSec)
{
	int NT = mts.size();
	for( int iT = 0; iT < NT; ++iT )
	{
		const MercatorTrade& mt = mts[iT];
		if( mt.price < minPrice )
			break;
		if( mt.msecs < minMsecs )
			continue;

		int haltMsecs = mt.msecs + pastSec * 1000;
		double prcPost = qSample.getMid(haltMsecs);
		if( prcPost > 0. && !isHalted(mt.msecs) && !isHalted(haltMsecs) )
		{
			double pastPnl = mt.sign * (prcPost / mt.price - 1.) * basis_pts_;
			if( pastPnl < stopBpt )
			{
				vStopMsecs_.push_back(haltMsecs);
				if( debug )
				{
					printf("%s o: %d h: %d pred: %.1f, prc0: %.2f prc%d: %.2f ret: %.1f\n",
							ticker.c_str(), mt.msecs, haltMsecs, mt.pred, mt.price, pastSec, prcPost, pastPnl);
				}
			}
		}
	}
}
