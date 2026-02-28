#include <gtlib_fitana/AnaAllDaysStat.h>
using namespace std;

namespace gtlib {

AnaAllDaysStat::AnaAllDaysStat()
	:nDays_(0)
{
}

AnaAllDaysStat::AnaAllDaysStat(const map<int, AnaDayStat>& mStat)
	:nDays_(mStat.size())
{
	for( auto& kv : mStat )
		addDayStat(kv.second);
}

void AnaAllDaysStat::addDayStat(const AnaDayStat& ds)
{
	ds_.merge(ds);
	accPnlbm_.add(ds.mevBm.mev * ds.mevBm.ntrds);
	accPnltb_.add(ds.mevTb.mev * ds.mevTb.ntrds);
}

float AnaAllDaysStat::getMbias()
{
	return ds_.mevTb.bias;
}

ostream& operator<<(ostream& os, const AnaAllDaysStat& rhs)
{
	double shrpRat = 0.;
	if( rhs.accPnltb_.stdev() > 0. )
		shrpRat = rhs.accPnltb_.mean() / rhs.accPnltb_.stdev();
	double thres = 0.;

	const AnaDayStat& ds = rhs.ds_;
	char buf[2000];
	sprintf(buf, "%.4f\t%.4f\t%.4f\t%.4f\t%.4f\t%.4f\t%.4f\t%.4f\t"
			"%.0f\t%.2f\t"
			"%.4f\t%.4f\t%.4f\t%.4f\t%.0f\t%.0f\t"
			"%.1f\t%.1f\t%.2f\n",
			ds.cbm, ds.ctb, ds.evBm.ev, ds.evTb.ev, ds.evBm.of, ds.evTb.of, ds.evBm.malpred, ds.evTb.malpred,
			(double)ds.npts / rhs.nDays_, thres,
			ds.mevBm.mev, ds.mevTb.mev, ds.mevBm.bias, ds.mevTb.bias,
			(double)ds.mevBm.ntrds / rhs.nDays_, (double)ds.mevTb.ntrds / rhs.nDays_,
			rhs.accPnlbm_.mean(), rhs.accPnltb_.mean(), shrpRat);
	os << buf;
	return os;
}

} // namespace gtlib
