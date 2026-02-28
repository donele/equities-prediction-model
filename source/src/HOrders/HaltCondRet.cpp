#include <HOrders/HaltCondRet.h>
#include <jl_lib/jlutil.h>
using namespace std;
using namespace gtlib;

HaltCondRet::HaltCondRet(int haltLenSec, int pastMsec, double stopBpt, bool debug)
	:debug_(debug),
	haltLenSec_(haltLenSec),
	pastMsec_(pastMsec),
	stopBpt_(stopBpt)
{
}

string HaltCondRet::getDesc()
{
	char buf[200];
	sprintf(buf, "for %d s. | %d msecRet > %.1f bpt",
			haltLenSec_, pastMsec_, stopBpt_);
	return buf;
}

void HaltCondRet::updateSched(HaltSched& hSched, const string& ticker,
		const std::vector<MercatorTrade>& mts,
		const QuoteSample& qSample, int minMsecs)
{
	int NT = mts.size();
	for( int iT = 0; iT < NT; ++iT )
	{
		const MercatorTrade& mt = mts[iT];
		if( mt.msecs < minMsecs )
			continue;

		double pastMid = qSample.getMid(mt.msecs - pastMsec_);
		if( pastMid > 0. && !hSched.isHalted(mt.msecs) )
		{
			double pastRet = (mt.mid / pastMid - 1.) * basis_pts_;
			if( fabs(pastRet) > stopBpt_ )
			{
				int haltMsecs = mt.msecs;
				hSched.addHalt(haltMsecs);
			}
		}
	}
}
