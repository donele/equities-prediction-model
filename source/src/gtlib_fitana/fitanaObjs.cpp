#include <gtlib_fitana/fitanaObjs.h>
using namespace std;

namespace gtlib {

void EvStat::merge(const EvStat& rhs)
{
	if( rhs.npts > 0 )
	{
		double lhsWgt = (double)npts / (npts + rhs.npts);
		double rhsWgt = (double)rhs.npts / (npts + rhs.npts);
		npts += rhs.npts;
		ev = lhsWgt * ev + rhsWgt * rhs.ev;
		of = lhsWgt * of + rhsWgt * rhs.of;
		malpred = lhsWgt * malpred + rhsWgt * rhs.malpred;
	}
}

void MevStat::merge(const MevStat& rhs)
{
	if( rhs.ntrds > 0 )
	{
		double lhsWgt = (double)ntrds / (ntrds + rhs.ntrds);
		double rhsWgt = (double)rhs.ntrds / (ntrds + rhs.ntrds);
		ntrds += rhs.ntrds;
		mev = lhsWgt * mev + rhsWgt * rhs.mev;
		bias = lhsWgt * bias + rhsWgt * rhs.bias;
	}
}

void AnaDayStat::merge(const AnaDayStat& rhs)
{
	if( rhs.npts > 0 )
	{
		double lhsWgt = (double)npts / (npts + rhs.npts);
		double rhsWgt = (double)rhs.npts / (npts + rhs.npts);

		npts += rhs.npts;

		cbm = lhsWgt * cbm + rhsWgt * rhs.cbm;
		ctb = lhsWgt * ctb + rhsWgt * rhs.ctb;

		evBm.merge(rhs.evBm);
		evTb.merge(rhs.evTb);

		mevBm.merge(rhs.mevBm);
		mevTb.merge(rhs.mevTb);
	}
}

ostream& operator <<(ostream& os, const AnaDayStat& rhs)
{
	char buf[2000];
	sprintf(buf, "%.4f\t%.4f\t%.4f\t%.4f\t%.4f\t%.4f\t%.4f\t%.4f\t"
			"%d\t%.4f\t%.4f\t%.4f\t%.4f\t%d\t%d\n",
			rhs.cbm, rhs.ctb, rhs.evBm.ev, rhs.evTb.ev, rhs.evBm.of, rhs.evTb.of, rhs.evBm.malpred, rhs.evTb.malpred,
			rhs.npts, rhs.mevBm.mev, rhs.mevTb.mev, rhs.mevBm.bias, rhs.mevTb.bias, rhs.mevBm.ntrds, rhs.mevTb.ntrds);
	os << buf;
	return os;
}

} // namespace gtlib
