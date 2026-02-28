#include <HOrders/HaltSimRet.h>
using namespace std;
using namespace gtlib;

HaltSimRet::HaltSimRet(int id, double minPrice, int minMsecs, int pastMsec, int haltLenSec, double stopBpt)
	:HaltSim(id, minPrice, minMsecs),
	pastMsec_(pastMsec),
	stopBpt_(stopBpt),
	haltLenSec_(haltLenSec)
{
}

HaltCondition* HaltSimRet::getHaltCondition(const string& ticker, const vector<MercatorTrade>& mts, const QuoteSample& qSample)
{
	HaltCondition* hc = new HaltConditionRet(mts, qSample, minPrice_, minMsecs_, pastMsec_, haltLenSec_, stopBpt_);
	return hc;
}

string HaltSimRet::getDesc()
{
	char buf[200];
	sprintf(buf, "for %d s. | %d msecRet > %.1f bpt",
			haltLenSec_, pastMsec_, stopBpt_);
	return buf;
}

//
// HaltCondition
//

HaltConditionRet::HaltConditionRet()
{
}

HaltConditionRet::HaltConditionRet(const std::vector<MercatorTrade>& mts,
		const QuoteSample& qSample, double minPrice, int minMsecs,
		int pastMsec, int haltLenSec, double stopBpt)
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

		double pastMid = qSample.getMid(mt.msecs - pastMsec);
		if( pastMid > 0. )
		{
			double pastRet = (mt.mid / pastMid - 1.) * basis_pts_;
			if( fabs(pastRet) > stopBpt )
			{
				int haltMsecs = mt.msecs;
				vStopMsecs_.push_back(haltMsecs);
			}
		}
	}
}
