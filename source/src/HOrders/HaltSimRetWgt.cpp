#include <HOrders/HaltSimRetWgt.h>
#include <jl_lib/GODBC.h>
using namespace std;
using namespace gtlib;

HaltSimRetWgt::HaltSimRetWgt(int id, double minPrice, int minMsecs, int pastMsec, int haltLenSec, double stopRat)
	:HaltSim(id, minPrice, minMsecs),
	pastMsec_(pastMsec),
	stopRat_(stopRat),
	haltLenSec_(haltLenSec)
{
}

HaltCondition* HaltSimRetWgt::getHaltCondition(const string& ticker, const vector<MercatorTrade>& mts, const QuoteSample& qSample)
{
	double volat = tickersVolat_.getVolat(ticker);
	HaltCondition* hc = new HaltConditionRetWgt(ticker, mts, qSample,
			minPrice_, minMsecs_, pastMsec_, haltLenSec_, stopRat_, volat, debug_);
	return hc;
}

string HaltSimRetWgt::getDesc()
{
	char buf[200];
	sprintf(buf, "for %d s. | %d msecRetRat > %.1f",
			haltLenSec_, pastMsec_, stopRat_);
	return buf;
}

void HaltSimRetWgt::beginDay(int idate)
{
	tickersVolat_.read(idate);
}

//
// HaltCondition
//

HaltConditionRetWgt::HaltConditionRetWgt()
{
}

HaltConditionRetWgt::HaltConditionRetWgt(const string& ticker, const vector<MercatorTrade>& mts,
		const QuoteSample& qSample, double minPrice, int minMsecs,
		int pastMsec, int haltLenSec, double stopRat, double volat, bool debug)
	//:haltLenSec_(haltLenSec)
	:HaltCondition(haltLenSec)
{
	int NT = mts.size();
	int prevMsecs = 0;
	double volatAdj = volat * sqrt(pastMsec / 1000. / (6.5*60));
	for( int iT = 0; iT < NT; ++iT )
	{
		const MercatorTrade& mt = mts[iT];
		if( mt.price < minPrice )
			break;
		if( mt.msecs < minMsecs || isHalted(mt.msecs) )
			continue;

		double pastMid = qSample.getMid(mt.msecs - pastMsec);
		if( pastMid > 0. )
		{
			double pastRetRat = (mt.mid / pastMid - 1.) / volatAdj;
			if( (stopRat > 0. && fabs(pastRetRat) > stopRat) ||
			 (stopRat < 0. && fabs(pastRetRat) < -stopRat) )
			{
				int haltMsecs = mt.msecs;
				vStopMsecs_.push_back(haltMsecs);
				if( debug )
				{
					double ret = mt.mid / pastMid - 1.;
					printf("%s h: %d pred: %.1f, prcFrom: %.2f prcTo: %.2f ret: %.1f, retAdj: %.1f\n",
							ticker.c_str(), mt.msecs, mt.pred, pastMid, mt.price, ret, pastRetRat);
				}
			}
		}
		prevMsecs = mt.msecs;
	}
}
