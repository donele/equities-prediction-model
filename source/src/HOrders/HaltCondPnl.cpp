#include <HOrders/HaltCondPnl.h>
#include <jl_lib/jlutil.h>
using namespace std;
using namespace gtlib;

HaltCondPnl::HaltCondPnl(int haltLenSec, int pastSec, double stopBpt, bool debug)
	:debug_(debug),
	haltLenSec_(haltLenSec),
	pastSec_(pastSec),
	stopBpt_(stopBpt)
{
}

string HaltCondPnl::getDesc()
{
	char buf[200];
	sprintf(buf, "for %d s. | %d secPnl < %.1f bpt",
			haltLenSec_, pastSec_, stopBpt_);
	return buf;
}

void HaltCondPnl::updateSched(HaltSched& hSched, const string& ticker, const vector<MercatorTrade>& mts,
		const QuoteSample& qSample, int minMsecs)
{
	int NT = mts.size();
	for( int iT = 0; iT < NT; ++iT )
	{
		const MercatorTrade& mt = mts[iT];
		if( mt.msecs < minMsecs )
			continue;

		int haltMsecs = mt.msecs + pastSec_ * 1000;
		double prcPost = qSample.getMid(haltMsecs);
		if( prcPost > 0. && !hSched.isHalted(mt.msecs) && !hSched.isHalted(haltMsecs) )
		{
			double pastPnl = mt.sign * (prcPost / mt.price - 1.) * basis_pts_;
			if( pastPnl < stopBpt_ )
			{
				hSched.addHalt(haltMsecs);
				if( debug_ )
				{
					printf("%s o: %d h: %d pred: %.1f, prc0: %.2f prc%d: %.2f ret: %.1f\n",
							ticker.c_str(), mt.msecs, haltMsecs, mt.pred, mt.price, pastSec_, prcPost, pastPnl);
				}
			}
		}
	}
}
