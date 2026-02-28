#include <HOrders/HaltCondRetWgt.h>
#include <jl_lib/jlutil.h>
#include <jl_lib/GODBC.h>
using namespace std;
using namespace gtlib;

HaltCondRetWgt::HaltCondRetWgt(int haltLenSec, int pastMsec, double stopRat, bool debug)
	:debug_(debug),
	haltLenSec_(haltLenSec),
	pastMsec_(pastMsec),
	stopRat_(stopRat)
{
}

string HaltCondRetWgt::getDesc()
{
	char buf[200];
	sprintf(buf, "for %d s. | %d msecRetRat > %.1f",
			haltLenSec_, pastMsec_, stopRat_);
	return buf;
}

void HaltCondRetWgt::beginDay(int idate)
{
	tickersVolat_.read(idate);
}

void HaltCondRetWgt::updateSched(HaltSched& hSched, const string& ticker,
	   	const vector<MercatorTrade>& mts,
		const QuoteSample& qSample, int minMsecs)
{
	int NT = mts.size();
	double volat = tickersVolat_.getVolat(ticker);
	double volatAdj = volat * sqrt(pastMsec_ / 1000. / (6.5*60));
	for( int iT = 0; iT < NT; ++iT )
	{
		const MercatorTrade& mt = mts[iT];
		if( mt.msecs < minMsecs )
			continue;

		double pastMid = qSample.getMid(mt.msecs - pastMsec_);
		if( pastMid > 0. && !hSched.isHalted(mt.msecs) )
		{
			double pastRetRat = (mt.mid / pastMid - 1.) / volatAdj;
			if(stopRat_ > 0. && fabs(pastRetRat) > stopRat_)
			{
				int haltMsecs = mt.msecs;
				hSched.addHalt(haltMsecs);
				if( debug_ )
				{
					double ret = mt.mid / pastMid - 1.;
					printf("%s h: %d pred: %.1f, prcFrom: %.2f prcTo: %.2f ret: %.1f, retAdj: %.1f\n",
							ticker.c_str(), mt.msecs, mt.pred, pastMid, mt.price, ret, pastRetRat);
				}
			}
		}
	}
}

